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

# ----------------------------------------------------------------------------

add_library(heap-arena-alloc INTERFACE)

add_library(allocators::heap-arena-alloc ALIAS heap-arena-alloc)

target_include_directories(heap-arena-alloc INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
  )


# ----------------------------------------------------------------------------
  
add_library(chunk-heap-arena-alloc INTERFACE)

add_library(allocators::chunk-heap-arena-alloc ALIAS chunk-heap-arena-alloc)

target_include_directories(chunk-heap-arena-alloc INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
  )

# ----------------------------------------------------------------------------

add_library(arena-alloc_v2 INTERFACE)

add_library(allocators::arena-alloc_v2 ALIAS arena-alloc_v2)

target_include_directories(arena-alloc_v2 INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
  )
