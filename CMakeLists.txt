cmake_minimum_required(VERSION 3.7)
project(BrlCV CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Boost REQUIRED)

add_subdirectory(lib)

add_executable(stats stats.cpp)
target_link_libraries(stats IO Boost::boost)
