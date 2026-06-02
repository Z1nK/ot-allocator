#include <array>
#include <iostream>
#include <iterator>

#include <list>
#include <map>

#include <vector>

#include <allocators/log_alloc.hpp>
#include <allocators/arena_alloc.hpp>
#include <allocators/chunk_heap_arena_alloc.hpp>
#include <allocators/heap_arena_alloc.hpp>

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

  // Test iterators and size with ArenaAllocator
  std::cout << "\n=== Testing Iterators and Size ===\n";
  MemoryArena<2048> vec_arena;
  ArenaAllocator<int, 2048> vec_alloc(vec_arena);
  std::vector<int, ArenaAllocator<int, 2048>> vec(vec_alloc);
  
  vec.push_back(10);
  vec.push_back(20);
  vec.push_back(30);

  using Vec = std::vector<int, ArenaAllocator<int, 2048>>;
  Vec::pointer ptr = vec.data();
  Vec::size_type sz = vec.size();

  std::cout << "Vector size: " << sz << "\n";
  std::cout << "Vector data pointer: " << ptr << "\n";
  std::cout << "Vector contents via iterator:\n";
  for (auto it = vec.begin(); it != vec.end(); ++it) {
    std::cout << "  *it = " << *it << "\n";
  }

  // Test ArenaAllocator with std::list (exercises free-list reuse)
  std::cout << "\n=== Testing ArenaAllocator with std::list ===\n";
  MemoryArena<4096> list_arena;
  ArenaAllocator<int, 4096> list_alloc(list_arena);
  std::list<int, ArenaAllocator<int, 4096>> lst(list_alloc);

  lst.push_back(10);
  lst.push_back(20);
  lst.push_back(30);
  std::cout << "After 3 push_back: arena used = " << list_arena.used() << " bytes\n";

  auto mid = std::next(lst.begin());
  lst.erase(mid);  // frees node '20' onto the free list
  std::cout << "After erase(20):   arena used = " << list_arena.used()
            << " bytes (unchanged; node on free list)\n";

  const std::size_t used_before_reuse = list_arena.used();
  lst.push_back(40);  // reuses the freed node
  std::cout << "After push_back(40): arena used = " << list_arena.used()
            << " bytes (" << (list_arena.used() == used_before_reuse ? "reused" : "new alloc") << ")\n";

  std::cout << "List contents: ";
  for (const auto& v : lst) {
    std::cout << v << " ";
  }
  std::cout << "\n";



  // ------------------------------------------------

  // Test ChunkHeapMemoryArena with std::map
  std::cout << "\n=== Testing ChunkHeapArenaAllocator with std::map ===\n";
  ChunkHeapMemoryArena chunk_arena(64);  // intentionally small to trigger chunk extension
  ChunkHeapArenaAllocator<std::pair<const int, int>> chunk_alloc(chunk_arena);

  std::map<int, int, std::less<int>,
           ChunkHeapArenaAllocator<std::pair<const int, int>>> chunk_map(
      std::less<int>(), chunk_alloc);

  std::cout << "Initial capacity: " << chunk_arena.capacity() << " bytes\n";
  std::cout << "Initial available: " << chunk_arena.available() << " bytes\n";

  for (int i = 1; i <= 10; ++i) {
    chunk_map[i] = i * 100;
  }

  std::cout << "After inserting 10 entries:\n";
  std::cout << "  Total capacity: " << chunk_arena.capacity() << " bytes\n";
  std::cout << "  Total used:     " << chunk_arena.used() << " bytes\n";
  std::cout << "  Available:      " << chunk_arena.available() << " bytes\n";

  std::cout << "Map contents:\n";
  for (const auto& [key, value] : chunk_map) {
    std::cout << "  [" << key << "] = " << value << "\n";
  }

  // Verify erase + lookup still work correctly after extension
  chunk_map.erase(5);
  std::cout << "After erasing key 5, find(5) "
            << (chunk_map.find(5) == chunk_map.end() ? "not found (ok)" : "ERROR: still present")
            << "\n";
  std::cout << "find(6) = " << chunk_map.at(6) << "\n";

  return 0;
}