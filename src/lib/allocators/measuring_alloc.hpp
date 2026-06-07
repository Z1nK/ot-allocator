#include <iostream>
#include <map>

template <typename T>
struct MeasuringAllocator {
  using value_type = T;
  MeasuringAllocator() = default;
  template <typename U>
  MeasuringAllocator(const MeasuringAllocator<U>&) {}

  static inline std::size_t detected_node_size = 0;

  T* allocate(std::size_t n) {
    // Save node size (it is equal to n * sizeof(T))
    detected_node_size = n * sizeof(T);

    return static_cast<T*>(::operator new(n * sizeof(T)));
  }

  void deallocate(T* p, std::size_t n) noexcept { ::operator delete(p); }
};