#pragma once
#include <array>
#include <concepts>
#include <cstddef>

template <std::size_t N>
concept MinimalArenaSize = N > 0;

template <std::size_t N>
  requires MinimalArenaSize<N>
class MemoryArena {
  std::array<std::byte, N> buffer;
  std::size_t offset = 0;

 public:

  [[nodiscard]] constexpr std::size_t capacity() const noexcept {
    return N;
  }

  [[nodiscard]] constexpr std::size_t used() const noexcept {
    return offset;
  }

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

  //TODO: implement comparations operators, for avoid compare backing storage
  auto operator<=>(const MemoryArena&) const = default;

};