cmake_minimum_required(VERSION 3.8)
project(BrlCV CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(Boost REQUIRED)

add_subdirectory(lib)

add_executable(stats stats.cpp)
target_link_libraries(stats IO Boost::boost)
add_executable(brltest brltest.cpp)
target_link_libraries(brltest IO)
add_executable(cv2midiclock cv2midiclock.cpp)
target_link_libraries(cv2midiclock IO Boost::boost)
add_executable(MIDILatency MIDILatency.cpp)
target_link_libraries(MIDILatency IO)
