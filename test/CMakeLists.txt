if(EXISTS ${CMAKE_SOURCE_DIR}/third_party/googletest/CMakeLists.txt)
  add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/googletest
                   ${CMAKE_BINARY_DIR}/googletest)
else()
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(relay_controller_tests
  main.cpp
  relay_controller_test.cpp
  relay_instance_test.cpp
  scheduler_test.cpp
)

target_include_directories(relay_controller_tests
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(relay_controller_tests
  PRIVATE
  relay_controller
  GTest::gtest
)

add_test(NAME relay_controller_tests COMMAND relay_controller_tests)
