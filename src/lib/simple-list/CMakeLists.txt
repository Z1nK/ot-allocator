set(LOCAL_LIB_NAME simple-list)

add_library(${LOCAL_LIB_NAME} INTERFACE)

add_library(simple-list::simple-list ALIAS ${LOCAL_LIB_NAME})

target_include_directories(${LOCAL_LIB_NAME} INTERFACE
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
  )