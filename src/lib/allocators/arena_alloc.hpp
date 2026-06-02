#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>

template <std::size_t N>
concept MinimalArenaSize = N > 0;

template <std::size_t N>
  requires MinimalArenaSize<N>
class MemoryArena {
  std::array<std::byte, N> buffer;
  std::size_t offset = 0;

 public:
  MemoryArena() = default;
  // Copying or moving MemoryArena would create a new buffer at a
  // different address, leaving every pointer previously returned by
  // allocate() dangling. Both operations are therefore deleted.
  MemoryArena(const MemoryArena&) = delete;
  MemoryArena& operator=(const MemoryArena&) = delete;
  MemoryArena(MemoryArena&&) = delete;
  MemoryArena& operator=(MemoryArena&&) = delete;
  [[nodiscard]] constexpr std::size_t capacity() const noexcept { return N; }

  [[nodiscard]] constexpr std::size_t used() const noexcept { return offset; }

  [[nodiscard]] constexpr std::size_t available() const noexcept {
    return N - offset;
  }

  std::byte* allocate(std::size_t bytes) {
    std::size_t space = N - offset;
    void* ptr = buffer.data() + offset;

    // std::align adjusts ptr to the next aligned address within space and
    // reduces space by the padding consumed. size=1 is the minimum valid
    // argument, so zero-byte requests use it to obtain a properly aligned
    // sentinel pointer without advancing offset.
    const std::size_t alloc_size = (bytes == 0) ? 1 : bytes;
    if (std::align(alignof(std::max_align_t), alloc_size, ptr, space)) {
      if (bytes > 0) {
        offset = N - space + bytes;
      }
      return static_cast<std::byte*>(ptr);
    }
    throw std::bad_alloc();
  }

  // Best-effort LIFO reclaim: if the freed block is the most-recently-allocated
  // one, retreat the offset so the space can be reused by the next allocate().
  // Non-LIFO frees are silently ignored; bulk reclaim remains available via
  // reset().
  void deallocate(std::byte* p, std::size_t bytes) noexcept {
    if (bytes > 0 && p + bytes == buffer.data() + offset) {
      offset = static_cast<std::size_t>(p - buffer.data());
    }
  }

  void reset() noexcept { offset = 0; }

  // Arenas are identified by address; value-based comparison is intentionally
  // disabled to prevent comparing uninitialized buffer bytes (UB).
  bool operator==(const MemoryArena&) const = delete;
  auto operator<=>(const MemoryArena&) const = delete;
};

// Allocator itself
template <typename T, std::size_t N>
class ArenaAllocator {
  MemoryArena<N>* arena;

  // Intrusive free list: freed blocks of exactly sizeof(T) bytes store a
  // next-pointer inside their own memory, so there is zero metadata overhead.
  struct FreeNode {
    FreeNode* next;
  };

  FreeNode* free_list_head = nullptr;

  template <typename U, std::size_t M>
  friend class ArenaAllocator;

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using is_always_equal = std::false_type;

  explicit ArenaAllocator(MemoryArena<N>& arena_ref) noexcept
      : arena(std::addressof(arena_ref)) {}

  template <typename U>
  ArenaAllocator(const ArenaAllocator<U, N>& other) noexcept
      : arena(other.arena) {}

  [[nodiscard]] T* allocate(std::size_t n) {
    if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
      throw std::bad_alloc();
    }
    // For single-element requests check the free list first.
    // Alignment is already guaranteed: MemoryArena always aligns to
    // alignof(std::max_align_t), which is >= alignof(FreeNode) on all
    // platforms. (noo need for sizeof(T) % alignof(FreeNode). The only
    // requirement is that the block is large enough to hold FreeNode::next. 
    // (P.S could be problem if use it for std::vector)
    if constexpr (sizeof(T) >= sizeof(FreeNode)) {
      if (n == 1 && free_list_head != nullptr) {
        FreeNode* node = free_list_head;
        free_list_head = node->next;
        std::destroy_at(
            node);  // end FreeNode lifetime; storage is now available for T
        return reinterpret_cast<T*>(node);
      }
    }
    return reinterpret_cast<T*>(arena->allocate(n * sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept {
    // Only single-element frees are pushed onto the free list: allocate() only
    // reuses n==1 blocks, so pushing n>1 blocks would pay O(n) cost with no
    // benefit. Multi-element frees are silently ignored; bulk reclaim via
    // reset() remains. Alignment is guaranteed by the arena (see allocate).
    // Size guard ensures the block has room for FreeNode::next. Placement new
    // starts FreeNode's lifetime, making the pointer write well-defined.
    if constexpr (sizeof(T) >= sizeof(FreeNode)) {
      if (n == 1) {
        FreeNode* node = ::new (static_cast<void*>(p)) FreeNode{free_list_head};
        free_list_head = node;
      }
    }
  }

  // Must be called instead of arena.reset() directly: resets both the free list
  // and the arena bump pointer atomically. Calling arena.reset() alone leaves
  // free_list_head pointing into bytes that the bump allocator will reuse,
  // causing two live allocations to share the same storage (silent corruption).
  void reset() noexcept {
    free_list_head = nullptr;
    arena->reset();
  }

  template <typename U>
  struct rebind {
    using other = ArenaAllocator<U, N>;
  };

  template <typename U>
  bool operator==(const ArenaAllocator<U, N>& other) const noexcept {
    return arena == other.arena;
  }

  // operator!= is synthesised from operator== (C++20). should be ...
};
