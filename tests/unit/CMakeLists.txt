add_executable(unit_tests unit_tests.cpp)

set_target_properties(unit_tests PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
)

target_link_libraries(unit_tests PRIVATE
	GTest::gtest
	GTest::gtest_main
	allocators::arena-alloc
	allocators::heap-arena-alloc
	allocators::log-alloc
	# log-reader::log-reader
)

include(GoogleTest)
gtest_discover_tests(unit_tests)