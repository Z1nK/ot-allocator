add_executable(arena2 arena_v2.cpp)

target_compile_features(arena2 PUBLIC cxx_std_20)

target_link_libraries(arena2 PRIVATE
                        allocators::arena-alloc_v2
                        
)

if (MSVC)
    target_compile_options(arena2 PRIVATE /W4)
else()
    target_compile_options(arena2 PRIVATE -Wall -Wextra -pedantic -Werror)
endif()


# Installation

# install(TARGETS arena2 RUNTIME DESTINATION bin)