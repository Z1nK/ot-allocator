#include <array>
#include <iostream>
#include <array>

#include <map>

#include <vector>

#include <allocators/log_alloc.hpp>

int main() {

  std::vector<int, LogAllocator<int>> vec(5);
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(5);
  vec.push_back(6);


  std::map<int, int, std::less<>, LogAllocator<std::pair<const int, int>>> m;
  m[1] = 1;
  m[2] = 2;
  m[3] = 3;

  return 0;

}