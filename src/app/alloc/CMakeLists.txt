add_executable(alloc alloc.cpp)

target_compile_features(alloc PUBLIC cxx_std_20)

target_link_libraries(alloc PRIVATE
                        allocators::log-alloc
                        allocators::arena-alloc
)

if (MSVC)
    target_compile_options(alloc PRIVATE /W4)
else()
    target_compile_options(alloc PRIVATE -Wall -Wextra -pedantic -Werror)
endif()


# Installation

install(TARGETS alloc RUNTIME DESTINATION bin)