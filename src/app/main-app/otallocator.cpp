#include <array>
#include <iostream>
#include <map>
#include <vector>

#include <allocators/arena_alloc.hpp>
#include <allocators/heap_arena_alloc.hpp>
#include <allocators/log_alloc.hpp>
#include <simple-list/simple_list.hpp>

constexpr std::size_t MAP_ARENA_SIZE = 512;

int factorial(int n) {
  if (n <= 1)
    return 1;
  return n * factorial(n - 1);
}

int main() {

  using MapAlloc = ArenaAllocator<std::pair<const int, int>, MAP_ARENA_SIZE>;

  // 1. creating an instance of std::map<int, int> with standart allocator
  std::map<int, int> standart_map;
  auto hint = standart_map.end();
  for (int i = 0; i <= 9; ++i) {
    hint = standart_map.insert(hint, {i, factorial(i)});
  }

  for (const auto &[key, value] : standart_map) {
    std::cout << key << " " << value << "\n";
  }
  // 2. creating an instance of std::map<int, int> with custom allocator (arena
  // allocator on stack)

  MemoryArena<MAP_ARENA_SIZE> map_arena;
  MapAlloc alloc(map_arena);
  std::map<int, int, std::less<int>, MapAlloc> stack_map(std::less<int>(),
                                                         alloc);

  hint = stack_map.end();
  for (int i = 0; i <= 9; ++i) {
    hint = stack_map.insert(hint, {i, factorial(i)});
  }

  for (const auto &[key, value] : stack_map) {
    std::cout << key << " " << value << "\n";
  }

  // 3. creating an instance of simple list with standart allocator (arena
  // allocator on heap)
  SimpleList<int> slst;
  for (int i = 0; i <= 9; ++i) {
    slst.push_back(i);
  }

  for (int value : slst) {
    std::cout << value << "\n";
  }

  // 4. creating an instance of simple list with custom allocator (arena
  // allocator on heap)
  HeapMemoryArena heap_arena(1024);
  HeapArenaAllocator<int> list_alloc(heap_arena);
  SimpleList<int, HeapArenaAllocator<int>> mlst(list_alloc);

  for (int i = 0; i <= 9; ++i) {
    mlst.push_back(i);
  }

  for (int value : mlst) {
    std::cout << value << "\n";
  }

  return 0;
}