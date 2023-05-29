cmake_minimum_required(VERSION 3.17)

project(TimeSeriesDB
    VERSION 1.0
    DESCRIPTION "Time Series Indexing Database"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

find_package(glog  REQUIRED)

add_subdirectory(lib_indexing)
add_subdirectory(test)
