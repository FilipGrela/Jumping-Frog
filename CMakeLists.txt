cmake_minimum_required(VERSION 3.30)
project(Test)

set(CMAKE_CXX_STANDARD 20)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(Test main.cpp)
target_link_libraries(Test ${CURSES_LIBRARIES})