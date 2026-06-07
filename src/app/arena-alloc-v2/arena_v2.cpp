#include <allocators/arena_alloc_v2.hpp>
#include <iostream>
#include <map>
#include <simple-list/simple_list.hpp>
#include <vector>

struct FakeMapNode {
  std::pair<const int, int> value;
  void *left;
  void *right;
  void *parent;
  bool color;
};

int main() {
  std::cout << "=== ArenaAllocator v2 example ===\n";

  // Use an arena where capacity is expressed as number of T-sized slots.
  MemoryArena2<int, 64> int_arena;
  ArenaAllocator2<int, int, 64> int_alloc(int_arena);
  std::vector<int, ArenaAllocator2<int, int, 64>> vec(int_alloc);
  vec.reserve(64);

  for (int i = 0; i < 64; ++i) {
    vec.push_back(i);
  }

  std::cout << "vector size: " << vec.size() << "\n";
  std::cout << "int arena used/available: " << int_arena.used() << "/"
            << int_arena.available() << " bytes\n";
  std::cout << "vector values: ";
  for (int v : vec) {
    std::cout << v << " ";
  }
  std::cout << "\n\n";

  // Direct arena API usage for raw byte blocks.
  MemoryArena2<int, 32> raw_arena;
  std::byte *block = raw_arena.allocate(3 * sizeof(int));
  int *ints = reinterpret_cast<int *>(block);
  ints[0] = 111;
  ints[1] = 222;
  ints[2] = 333;

  std::cout << "raw arena values: " << ints[0] << " " << ints[1] << " "
            << ints[2] << "\n";
  std::cout << "raw arena used/available: " << raw_arena.used() << "/"
            << raw_arena.available() << " bytes\n";

  // SimpleList uses allocator rebinding to allocate Node<int> objects.
  MemoryArena2<Node<int>, 64> list_arena;
  ArenaAllocator2<int, Node<int>, 64> list_alloc(list_arena);
  SimpleList<int, ArenaAllocator2<int, Node<int>, 64>> slist(list_alloc);

    std::cout << "\nsimple list arena used/available: " << list_arena.used()
            << "/" << list_arena.available() << " bytes\n";

  for (int i = 0; i < 64; ++i) {
    slist.push_back(i);
  }

  std::cout << "simple list values: ";
  for (int value : slist) {
    std::cout << value << " ";
  }
  std::cout << "\n";
  std::cout << "simple list arena used/available: " << list_arena.used()
            << "/" << list_arena.available() << " bytes\n";

  // std::map usage with ArenaAllocator2

  using MapValue = std::pair<const int, int>;
  MemoryArena2<FakeMapNode, 256> map_arena;
  ArenaAllocator2<MapValue, FakeMapNode, 256> map_alloc(map_arena);
  std::map<int, int, std::less<int>, ArenaAllocator2<MapValue, FakeMapNode, 256>>
      m(std::less<int>{}, map_alloc);

  std::cout << "\nmap arena used before inserts: " << map_arena.used()
            << " bytes\n";
  m.emplace(1, 10);
  m.emplace(2, 20);
  m.emplace(3, 30);
  m.emplace(4, 40);

  std::cout << "map values: ";
  for (const auto &[k, v] : m) {
    std::cout << "(" << k << ":" << v << ") ";
  }
  std::cout << "\n";
  std::cout << "map arena used/available: " << map_arena.used() << "/"
            << map_arena.available() << " bytes\n";

  return 0;
}
