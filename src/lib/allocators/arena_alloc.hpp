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
  std::byte* allocate(std::size_t bytes) {
    if (offset + bytes > N) {
      throw std::bad_alloc();
    }

    std::byte* current_ptr = buffer.data() + offset;

    offset += bytes;

    return current_ptr;
  }
};