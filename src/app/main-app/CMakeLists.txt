add_executable(otallocator otallocator.cpp)

target_compile_features(otallocator PUBLIC cxx_std_20)

target_link_libraries(otallocator PRIVATE
                        allocators::log-alloc
                        allocators::arena-alloc
                        allocators::heap-arena-alloc
                        allocators::chunk-heap-arena-alloc
                        simple-list::simple-list
)

if (MSVC)
    target_compile_options(otallocator PRIVATE /W4)
else()
    target_compile_options(otallocator PRIVATE -Wall -Wextra -pedantic -Werror)
endif()


# Installation

install(TARGETS otallocator RUNTIME DESTINATION bin)