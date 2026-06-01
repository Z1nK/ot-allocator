#pragma once
#include <cstddef>
#include <limits>
#include <memory>
#include <new>

class HeapMemoryArena {
  std::unique_ptr<std::byte[]> buffer;
  std::size_t capacity_;
  std::size_t offset = 0;

 public:
  explicit HeapMemoryArena(std::size_t size)
      : buffer(std::make_unique<std::byte[]>(size)), capacity_(size) {}

  // Copying or moving HeapMemoryArena would leave every pointer previously
  // returned by allocate() dangling. Both operations are therefore deleted.
  HeapMemoryArena(const HeapMemoryArena&) = delete;
  HeapMemoryArena& operator=(const HeapMemoryArena&) = delete;
  HeapMemoryArena(HeapMemoryArena&&) = delete;
  HeapMemoryArena& operator=(HeapMemoryArena&&) = delete;

  [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
  [[nodiscard]] std::size_t used() const noexcept { return offset; }
  [[nodiscard]] std::size_t available() const noexcept { return capacity_ - offset; }

  std::byte* allocate(std::size_t bytes) {
    std::size_t space = capacity_ - offset;
    void* ptr = buffer.get() + offset;

    const std::size_t alloc_size = (bytes == 0) ? 1 : bytes;
    if (std::align(alignof(std::max_align_t), alloc_size, ptr, space)) {
      if (bytes > 0) {
        offset = capacity_ - space + bytes;
      }
      return static_cast<std::byte*>(ptr);
    }
    throw std::bad_alloc();
  }

  // Best-effort LIFO reclaim: if the freed block is the most-recently-allocated
  // one, retreat the offset so the space can be reused by the next allocate().
  // Non-LIFO frees are silently ignored; bulk reclaim remains available via reset().
  void deallocate(std::byte* p, std::size_t bytes) noexcept {
    if (bytes > 0 && p + bytes == buffer.get() + offset) {
      offset = static_cast<std::size_t>(p - buffer.get());
    }
  }

  void reset() noexcept { offset = 0; }

  bool operator==(const HeapMemoryArena&) const = delete;
  auto operator<=>(const HeapMemoryArena&) const = delete;
};

template <typename T>
class HeapArenaAllocator {
  HeapMemoryArena* arena;

  // Intrusive free list: freed blocks store a next-pointer inside their own
  // memory (zero metadata overhead). Alignment is guaranteed by the arena which
  // always aligns to alignof(std::max_align_t) >= alignof(FreeNode).
  struct FreeNode {
    FreeNode* next;
  };

  FreeNode* free_list_head = nullptr;

  template <typename U>
  friend class HeapArenaAllocator;

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using is_always_equal = std::false_type;

  explicit HeapArenaAllocator(HeapMemoryArena& arena_ref) noexcept
      : arena(std::addressof(arena_ref)) {}

  template <typename U>
  HeapArenaAllocator(const HeapArenaAllocator<U>& other) noexcept
      : arena(other.arena) {}

  [[nodiscard]] T* allocate(std::size_t n) {
    if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
      throw std::bad_alloc();
    }
    if constexpr (sizeof(T) >= sizeof(FreeNode)) {
      if (n == 1 && free_list_head != nullptr) {
        FreeNode* node = free_list_head;
        free_list_head = node->next;
        std::destroy_at(node);  // end FreeNode lifetime; storage available for T
        return reinterpret_cast<T*>(node);
      }
    }
    return reinterpret_cast<T*>(arena->allocate(n * sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept {
    // Only n==1 pushed: allocate() only reuses single-element blocks.
    // Placement new starts FreeNode lifetime for well-defined pointer write.
    if constexpr (sizeof(T) >= sizeof(FreeNode)) {
      if (n == 1) {
        FreeNode* node = ::new (static_cast<void*>(p)) FreeNode{free_list_head};
        free_list_head = node;
      }
    }
  }

  // Must be called instead of arena.reset() directly: resets both the free list
  // and the arena bump pointer atomically to prevent stale free-list pointers
  // aliasing freshly bump-allocated blocks (silent corruption).
  void reset() noexcept {
    free_list_head = nullptr;
    arena->reset();
  }

  template <typename U>
  struct rebind {
    using other = HeapArenaAllocator<U>;
  };

  template <typename U>
  bool operator==(const HeapArenaAllocator<U>& other) const noexcept {
    return arena == other.arena;
  }
};
