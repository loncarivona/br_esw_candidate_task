cmake_minimum_required(VERSION 3.20)
project(br_esw_relay_controller C CXX)

set(RELAY_SOURCES
  src/relay_io/relay_io_sim.c
  src/relay_control/relay_instance.c
  src/relay_control/relay_controller.c
  src/scheduler/scheduler.c
)

add_library(relay_controller STATIC ${RELAY_SOURCES})
target_include_directories(relay_controller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(BUILD_DEMO "Build host demo executable" ON)
if(BUILD_DEMO)
  add_executable(relay_controller_demo src/main.c)
  target_link_libraries(relay_controller_demo PRIVATE relay_controller)
endif()

option(BUILD_TESTS "Build Google Test unit tests" ON)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()
