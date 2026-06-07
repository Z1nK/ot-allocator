#include <iostream>
#include <vector>

#include <allocators/arena_alloc_v2.hpp>

int main() {
	std::cout << "=== ArenaAllocator v2 example ===\n";

	// Use an arena where capacity is expressed as number of T-sized slots.
	MemoryArena2<int, 64> int_arena;
	ArenaAllocator2<int, int, 64> int_alloc(int_arena);
	std::vector<int, ArenaAllocator2<int, int, 64>> vec(int_alloc);

	for (int i = 1; i <= 10; ++i) {
		vec.push_back(i * i);
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

	return 0;
}
