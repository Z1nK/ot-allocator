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

  void reset() noexcept { offset = 0; }

  bool operator==(const HeapMemoryArena&) const = delete;
  auto operator<=>(const HeapMemoryArena&) const = delete;
};

template <typename T>
class HeapArenaAllocator {
  HeapMemoryArena* arena;

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
    return reinterpret_cast<T*>(arena->allocate(n * sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept {
    (void)p;
    (void)n;
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
