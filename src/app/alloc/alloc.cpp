#include <array>
#include <iostream>
#include <array>

#include <map>

#include <vector>

#include <allocators/log_alloc.hpp>
#include <allocators/arena_alloc.hpp>

int main() {

  // std::vector<int, LogAllocator<int>> vec(5);
  // vec.push_back(1);
  // vec.push_back(2);
  // vec.push_back(3);
  // vec.push_back(4);
  // vec.push_back(5);
  // vec.push_back(6);


  // std::map<int, int, std::less<>, LogAllocator<std::pair<const int, int>>> m;
  // m[1] = 1;
  // m[2] = 2;
  // m[3] = 3;

  // Test MemoryArena
  std::cout << "\n=== Testing MemoryArena ===\n";
  MemoryArena<1024> arena;
  std::cout << "Capacity: " << arena.capacity() << " bytes\n";
  std::cout << "Used: " << arena.used() << " bytes\n";
  std::cout << "Available: " << arena.available() << " bytes\n";

  [[maybe_unused]] auto ptr1 = arena.allocate(100);
  std::cout << "\nAfter allocating 100 bytes:\n";
  std::cout << "Used: " << arena.used() << " bytes\n";
  std::cout << "Available: " << arena.available() << " bytes\n";

  [[maybe_unused]] auto ptr2 = arena.allocate(256);
  std::cout << "\nAfter allocating 256 bytes:\n";
  std::cout << "Used: " << arena.used() << " bytes\n";
  std::cout << "Available: " << arena.available() << " bytes\n";

  return 0;
}