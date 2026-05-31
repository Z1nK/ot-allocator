add_library(log-alloc INTERFACE)

add_library(allocators::log-alloc ALIAS log-alloc)

target_include_directories(log-alloc INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
  )

# ----------------------------------------------------------------------------

add_library(arena-alloc INTERFACE)

add_library(allocators::arena-alloc ALIAS arena-alloc)

target_include_directories(arena-alloc INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
  )


