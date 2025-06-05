cmake_minimum_required(VERSION 3.14)

project(slapdb_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG    release-1.12.1 # Используйте актуальный тег/коммит
)
FetchContent_MakeAvailable(googletest)

add_library(slapdb_lib STATIC
  src/slapdb.cpp
)

add_executable(run_tests
  tests/test_slapdb.cpp
)

target_link_libraries(run_tests PRIVATE
  slapdb_lib
  gtest_main
)

target_include_directories(run_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(GoogleTest)
gtest_discover_tests(run_tests)

