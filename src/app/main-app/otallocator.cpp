#include <array>
#include <iostream>
#include <array>

#include <map>

#include <vector>

#include <allocators/log_alloc.hpp>
#include <allocators/arena_alloc.hpp>
#include <allocators/heap_arena_alloc.hpp>

int factorial(int n) {
  if (n <= 1) return 1;
  return n * factorial(n - 1);
}

int main() {

  std::map<int, int> standart_map;
  auto hint = standart_map.end();
  for (int i = 0; i <= 9; ++i) {    
    hint = standart_map.insert(hint, {i, factorial(i)});
  }
    
  return 0;
}