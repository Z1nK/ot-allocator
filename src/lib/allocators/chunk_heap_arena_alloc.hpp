#pragma once
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <vector>
class ChunkHeapMemoryArena {
  // Each chunk is an independent heap buffer. Storing chunks in a vector and
  // only ever appending means every raw pointer previously returned by
  // allocate() stays valid: vector reallocation moves the Chunk structs (which
  // hold a unique_ptr), not the underlying byte buffers.
  struct Chunk {
    std::unique_ptr<std::byte[]> buffer;
    std::size_t capacity;
    std::size_t offset = 0;

    explicit Chunk(std::size_t size)
        : buffer(std::make_unique<std::byte[]>(size)), capacity(size) {}

    Chunk(Chunk &&) = default;
    Chunk &operator=(Chunk &&) = default;
  };

  std::vector<Chunk> chunks_;

public:
  explicit ChunkHeapMemoryArena(std::size_t initial_size) {
    chunks_.emplace_back(initial_size);
  }

  // Copying or moving ChunkHeapMemoryArena would leave every pointer previously
  // returned by allocate() dangling. Both operations are therefore deleted.
  ChunkHeapMemoryArena(const ChunkHeapMemoryArena &) = delete;
  ChunkHeapMemoryArena &operator=(const ChunkHeapMemoryArena &) = delete;
  ChunkHeapMemoryArena(ChunkHeapMemoryArena &&) = delete;
  ChunkHeapMemoryArena &operator=(ChunkHeapMemoryArena &&) = delete;

  // Total capacity across all chunks (initial + any extension chunks).
  [[nodiscard]] std::size_t capacity() const noexcept {
    std::size_t total = 0;
    for (const auto &c : chunks_)
      total += c.capacity;
    return total;
  }

  // Total bytes consumed across all chunks.
  [[nodiscard]] std::size_t used() const noexcept {
    std::size_t total = 0;
    for (const auto &c : chunks_)
      total += c.offset;
    return total;
  }

  // Bytes still available in the current (last) chunk before the next
  // extension.
  [[nodiscard]] std::size_t available() const noexcept {
    return chunks_.back().capacity - chunks_.back().offset;
  }

  // alignment must be a power of two and must not exceed
  // alignof(std::max_align_t): new std::byte[] only guarantees that alignment
  // for the buffer start, so std::align cannot satisfy a larger alignment
  // request.
  std::byte *allocate(std::size_t bytes,
                      std::size_t alignment = alignof(std::max_align_t)) {
    const std::size_t alloc_size = (bytes == 0) ? 1 : bytes;

    // Fast path: try the current chunk.
    {
      Chunk &cur = chunks_.back();
      std::size_t space = cur.capacity - cur.offset;
      void *ptr = cur.buffer.get() + cur.offset;
      if (std::align(alignment, alloc_size, ptr, space)) {
        // Always advance by alloc_size so repeated allocate(0) calls do not
        // return the same address and overlap later allocations.
        cur.offset = cur.capacity - space + alloc_size;
        return static_cast<std::byte *>(ptr);
      }
    }

    // Overflow: grow by doubling, but always large enough for the request.
    // The extra (alignment - 1) bytes cover worst-case alignment padding.
    const std::size_t new_cap =
        std::max(chunks_.back().capacity * 2, alloc_size + alignment - 1);
    chunks_.emplace_back(new_cap);

    Chunk &fresh = chunks_.back();
    std::size_t space = fresh.capacity;
    void *ptr = fresh.buffer.get();
    if (std::align(alignment, alloc_size, ptr, space)) {
      fresh.offset = fresh.capacity - space + alloc_size;
      return static_cast<std::byte *>(ptr);
    }
    throw std::bad_alloc();
  }

  // Best-effort LIFO reclaim: if the freed block is the most-recently-allocated
  // one in the current chunk, retreat the offset so the space can be reused.
  // Non-LIFO frees are silently ignored; bulk reclaim remains available via
  // reset().
  void deallocate(std::byte *p, std::size_t bytes) noexcept {
    if (bytes > 0) {
      Chunk &cur = chunks_.back();
      if (p + bytes == cur.buffer.get() + cur.offset) {
        cur.offset = static_cast<std::size_t>(p - cur.buffer.get());
      }
    }
  }

  // Discards all extension chunks and resets the initial chunk's offset to
  // zero.
  void reset() noexcept {
    if (chunks_.size() > 1)
      chunks_.erase(chunks_.begin() + 1, chunks_.end());
    chunks_.front().offset = 0;
  }

  bool operator==(const ChunkHeapMemoryArena &) const = delete;
  auto operator<=>(const ChunkHeapMemoryArena &) const = delete;
};

template <typename T> class ChunkHeapArenaAllocator {
  // new std::byte[] only guarantees alignof(std::max_align_t) for the buffer
  // start, so std::align cannot satisfy a larger alignment requirement.
  // Use a dedicated aligned allocator for over-aligned types.
  static_assert(alignof(T) <= alignof(std::max_align_t),
                "ChunkHeapArenaAllocator does not support over-aligned types "
                "(alignof(T) > alignof(std::max_align_t))");

  ChunkHeapMemoryArena *arena;

  // Intrusive free list: freed blocks store a next-pointer inside their own
  // memory (zero metadata overhead). Alignment is guaranteed by the arena which
  // always aligns to alignof(std::max_align_t) >= alignof(FreeNode).
  struct FreeNode {
    FreeNode *next;
  };

  // Held via shared_ptr so all same-type copies of this allocator share the
  // same free list. Rebind copies (U -> T) receive a fresh list for T,
  // preventing aliasing of differently-typed storage. This ensures that
  // calling reset() on any one copy clears the free list for all copies,
  // preventing stale pointers into the reset arena.
  std::shared_ptr<FreeNode *> free_list_head_;

  template <typename U> friend class ChunkHeapArenaAllocator;

public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using is_always_equal = std::false_type;

  explicit ChunkHeapArenaAllocator(ChunkHeapMemoryArena &arena_ref)
      : arena(std::addressof(arena_ref)),
        free_list_head_(std::make_shared<FreeNode *>(nullptr)) {}

  // Same-type copy: shares arena and free list (default copy is correct).
  // std::shared_ptr copy constructor is noexcept, so this is noexcept.
  ChunkHeapArenaAllocator(const ChunkHeapArenaAllocator &) noexcept = default;
  ChunkHeapArenaAllocator &
  operator=(const ChunkHeapArenaAllocator &) noexcept = default;

  // Rebind copy (U -> T): share the arena but start a fresh free list for T.
  template <typename U>
  ChunkHeapArenaAllocator(const ChunkHeapArenaAllocator<U> &other)
      : arena(other.arena),
        free_list_head_(std::make_shared<FreeNode *>(nullptr)) {}

  [[nodiscard]] T *allocate(std::size_t n) {
    if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
      throw std::bad_alloc();
    }
    if constexpr (sizeof(T) >= sizeof(FreeNode)) {
      if (n == 1 && *free_list_head_ != nullptr) {
        FreeNode *node = *free_list_head_;
        *free_list_head_ = node->next;
        std::destroy_at(node); // end FreeNode lifetime; storage available for T
        return reinterpret_cast<T *>(node);
      }
    }
    return reinterpret_cast<T *>(arena->allocate(n * sizeof(T), alignof(T)));
  }

  void deallocate(T *p, std::size_t n) noexcept {
    // Only n==1 pushed: allocate() only reuses single-element blocks.
    // Placement new starts FreeNode lifetime for well-defined pointer write.
    if constexpr (sizeof(T) >= sizeof(FreeNode)) {
      if (n == 1) {
        FreeNode *node =
            ::new (static_cast<void *>(p)) FreeNode{*free_list_head_};
        *free_list_head_ = node;
      }
    }
  }

  // Resets both the shared free list and the arena bump pointer atomically.
  // Because free_list_head_ is shared with all same-type copies, every copy
  // sees a clean slate after this call; no copy can hold stale pointers into
  // the reset arena.
  void reset() noexcept {
    *free_list_head_ = nullptr;
    arena->reset();
  }

  template <typename U> struct rebind {
    using other = ChunkHeapArenaAllocator<U>;
  };

  template <typename U>
  bool operator==(const ChunkHeapArenaAllocator<U> &other) const noexcept {
    return arena == other.arena;
  }
};
