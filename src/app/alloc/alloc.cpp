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

  // Test ArenaAllocator with std::map
  std::cout << "\n=== Testing ArenaAllocator with std::map ===\n";
  MemoryArena<4096> map_arena;
  ArenaAllocator<std::pair<const int, int>, 4096> alloc(map_arena);
  
  std::map<int, int, std::less<int>, 
           ArenaAllocator<std::pair<const int, int>, 4096>> map(
      std::less<int>(), alloc);

  std::cout << "Map arena capacity: " << map_arena.capacity() << " bytes\n";
  std::cout << "Map arena used before insertion: " << map_arena.used() << " bytes\n";

  map[1] = 100;
  map[2] = 200;
  map[3] = 300;
  map[5] = 500;

  std::cout << "Map arena used after insertion: " << map_arena.used() << " bytes\n";
  std::cout << "Map arena available: " << map_arena.available() << " bytes\n";

  std::cout << "\nMap contents:\n";
  for (const auto& [key, value] : map) {
    std::cout << "  [" << key << "] = " << value << "\n";
  }

  return 0;
}