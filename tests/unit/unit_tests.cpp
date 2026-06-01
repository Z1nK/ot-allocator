#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "allocators/arena_alloc.hpp"
#include "allocators/log_alloc.hpp"

namespace {

TEST(MemoryArenaTests, InitialStateIsEmpty) {
	MemoryArena<64> arena;

	EXPECT_EQ(arena.capacity(), 64U);
	EXPECT_EQ(arena.used(), 0U);
	EXPECT_EQ(arena.available(), 64U);
}

TEST(MemoryArenaTests, AllocateAdvancesOffsetAndReturnsContiguousMemory) {
	MemoryArena<64> arena;

	std::byte* first = arena.allocate(16);
	std::byte* second = arena.allocate(8);

	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);
	EXPECT_EQ(second, first + 16);
	EXPECT_EQ(arena.used(), 24U);
	EXPECT_EQ(arena.available(), 40U);
}

TEST(MemoryArenaTests, AllocateZeroReturnsValidPtrAndDoesNotConsumeArena) {
	MemoryArena<128> arena;

	// Misalign offset by allocating 1 byte first, so the next arena position
	// is not naturally aligned to std::max_align_t.
	(void)arena.allocate(1);
	const std::size_t used_before = arena.used();

	std::byte* ptr = arena.allocate(0);

	EXPECT_NE(ptr, nullptr);
	EXPECT_EQ(arena.used(), used_before);  // offset must not change
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % alignof(std::max_align_t), 0U);
}

TEST(MemoryArenaTests, AllocateAccountsForAlignmentPaddingInUsedBytes) {
	MemoryArena<128> arena;

	std::byte* first = arena.allocate(1);
	ASSERT_NE(first, nullptr);
	const std::size_t used_after_first = arena.used();

	std::byte* second = arena.allocate(1);
	ASSERT_NE(second, nullptr);

	const std::size_t consumed_by_second = arena.used() - used_after_first;
	const std::size_t alignment_padding =
				static_cast<std::size_t>(second - (first + 1));

	EXPECT_EQ(consumed_by_second, alignment_padding + 1U);
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(second) %
							alignof(std::max_align_t),
						0U);
}

TEST(MemoryArenaTests, AllocateThrowsWhenAlignmentLeavesInsufficientSpace) {
	MemoryArena<17> arena;

	(void)arena.allocate(1);
	const std::size_t used_before_failed_allocation = arena.used();

	EXPECT_THROW((void)arena.allocate(2), std::bad_alloc);
	EXPECT_EQ(arena.used(), used_before_failed_allocation);
}

TEST(MemoryArenaTests, AllocateThrowsWhenCapacityExceeded) {
	MemoryArena<32> arena;
	(void)arena.allocate(24);

	EXPECT_THROW((void)arena.allocate(16), std::bad_alloc);
	EXPECT_EQ(arena.used(), 24U);
	EXPECT_EQ(arena.available(), 8U);
}

TEST(MemoryArenaTests, IsNeitherCopyableNorMovable) {
	// The arena's identity IS its buffer address. Copying or moving would
	// produce a new buffer at a different address, making every pointer
	// previously returned by allocate() dangle.
	static_assert(!std::is_copy_constructible_v<MemoryArena<64>>);
	static_assert(!std::is_copy_assignable_v<MemoryArena<64>>);
	static_assert(!std::is_move_constructible_v<MemoryArena<64>>);
	static_assert(!std::is_move_assignable_v<MemoryArena<64>>);
}

TEST(MemoryArenaTests, ResetClearsUsedBytes) {
	MemoryArena<32> arena;
	(void)arena.allocate(12);
	(void)arena.allocate(8);

	arena.reset();

	EXPECT_EQ(arena.used(), 0U);
	EXPECT_EQ(arena.available(), 32U);
}

TEST(ArenaAllocatorTests, AllocateZeroReturnsAlignedPtrAndDoesNotConsumeArena) {
	MemoryArena<64> arena;
	ArenaAllocator<int, 64> allocator(arena);

	int* ptr = allocator.allocate(0);

	EXPECT_NE(ptr, nullptr);
	EXPECT_EQ(arena.used(), 0U);
	EXPECT_EQ(reinterpret_cast<std::uintptr_t>(ptr) % alignof(std::max_align_t), 0U);
}

TEST(ArenaAllocatorTests, AllocateConsumesExpectedBytes) {
	MemoryArena<128> arena;
	ArenaAllocator<int, 128> allocator(arena);

	int* ptr = allocator.allocate(3);

	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(arena.used(), 3U * sizeof(int));
	EXPECT_EQ(arena.available(), 128U - (3U * sizeof(int)));
}

TEST(ArenaAllocatorTests, CopyFromOtherTypeUsesSameArena) {
	MemoryArena<128> arena;
	ArenaAllocator<int, 128> int_allocator(arena);
	ArenaAllocator<double, 128> double_allocator(int_allocator);

	double* ptr = double_allocator.allocate(1);

	ASSERT_NE(ptr, nullptr);
	EXPECT_EQ(arena.used(), sizeof(double));
}

TEST(ArenaAllocatorTests, EqualityDependsOnArenaAddress) {
	MemoryArena<64> arena_1;
	MemoryArena<64> arena_2;

	ArenaAllocator<int, 64> a(arena_1);
	ArenaAllocator<int, 64> b(arena_1);
	ArenaAllocator<int, 64> c(arena_2);

	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
	EXPECT_FALSE(a == c);
	EXPECT_TRUE(a != c);
}

TEST(ArenaAllocatorTests, AllocateThrowsWhenCountWouldOverflowSizeCalculation) {
	MemoryArena<128> arena;
	ArenaAllocator<int, 128> allocator(arena);

	constexpr std::size_t too_large_count =
			(std::numeric_limits<std::size_t>::max() / sizeof(int)) + 1U;

	EXPECT_THROW((void)allocator.allocate(too_large_count), std::bad_alloc);
}

TEST(ArenaAllocatorTests, DeallocateIsNoOp) {
	MemoryArena<64> arena;
	ArenaAllocator<int, 64> allocator(arena);

	int* ptr = allocator.allocate(2);
	ASSERT_NE(ptr, nullptr);
	const std::size_t used_before = arena.used();

	allocator.deallocate(ptr, 2);

	EXPECT_EQ(arena.used(), used_before);
}

TEST(LogAllocatorTests, AllocateAndDeallocateRawStorage) {
	LogAllocator<int> allocator;

	int* ptr = allocator.allocate(4);

	ASSERT_NE(ptr, nullptr);
	allocator.deallocate(ptr, 4);
}

TEST(LogAllocatorTests, EqualityOperatorsAreTypeAgnostic) {
	LogAllocator<int> int_allocator;
	LogAllocator<double> double_allocator;

	EXPECT_TRUE(int_allocator == double_allocator);
	EXPECT_FALSE(int_allocator != double_allocator);
}

TEST(LogAllocatorTests, WorksWithStdVector) {
	std::vector<int, LogAllocator<int>> values;

	values.push_back(1);
	values.push_back(2);
	values.push_back(3);

	ASSERT_EQ(values.size(), 3U);
	EXPECT_EQ(values[0], 1);
	EXPECT_EQ(values[1], 2);
	EXPECT_EQ(values[2], 3);
}

}  // namespace
