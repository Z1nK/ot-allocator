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
  [[nodiscard]] constexpr std::size_t capacity() const noexcept { return N; }

  [[nodiscard]] constexpr std::size_t used() const noexcept { return offset; }

  [[nodiscard]] constexpr std::size_t available() const noexcept {
    return N - offset;
  }

  std::byte* allocate(std::size_t bytes) {
    if (offset + bytes > N) {
      throw std::bad_alloc();
    }

    std::byte* current_ptr = buffer.data() + offset;

    offset += bytes;

    return current_ptr;
  }

  // TODO: implement deallocate, for now we can only reset the arena.
  //! could be problem if data types not simple.
  void reset() noexcept { offset = 0; }

  // TODO: implement comparations operators, for avoid compare backing storage
  auto operator<=>(const MemoryArena&) const = default;
};

// Allocator itself
template <typename T, std::size_t N>
class ArenaAllocator {
  MemoryArena<N>* arena;

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
    if (n == 0) return nullptr;
    if (n > (std::numeric_limits<std::size_t>::max() / sizeof(T))) {
      throw std::bad_alloc();
    }
    return reinterpret_cast<T*>(arena->allocate(n * sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept {
    // Nothing to do, as the arena will be reset all at once after.
    (void)p;
    (void)n;
  }

  template <typename U>
  struct rebind {
    using other = ArenaAllocator<U, N>;
  };

  template <typename U>
  bool operator==(const ArenaAllocator<U, N>& other) const noexcept {
    return arena == other.arena;
  }

  template <typename U>
  bool operator!=(const ArenaAllocator<U, N>& other) const noexcept {
    return arena != other.arena;
  }
};
