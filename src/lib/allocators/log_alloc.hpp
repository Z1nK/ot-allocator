#pragma once

template <typename T> struct LogAllocator {
  using value_type = T;

  LogAllocator() noexcept = default;

  template <typename U> LogAllocator(const LogAllocator<U> &) noexcept {}

  [[nodiscard]] T *allocate(std::size_t n) {
    std::cout << "Allocating " << n * sizeof(T) << " bytes for "
              << typeid(T).name() << "\n";
    return static_cast<T *>(::operator new(n * sizeof(T)));
  }

  void deallocate(T *p, std::size_t n) noexcept {
    std::cout << "Deallocating " << n * sizeof(T) << " bytes for "
              << typeid(T).name() << "\n";
    ::operator delete(p);
  }
};

template <typename T, typename U>
constexpr bool operator==(const LogAllocator<T> &,
                          const LogAllocator<U> &) noexcept {
  return true;
}

template <typename T, typename U>
constexpr bool operator!=(const LogAllocator<T> &,
                          const LogAllocator<U> &) noexcept {
  return false;
}