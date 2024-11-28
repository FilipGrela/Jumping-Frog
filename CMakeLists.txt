cmake_minimum_required(VERSION 3.30)
project(Jumping-Frog)

set(CMAKE_CXX_STANDARD 20)
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(Jumping-Frog.exe main.cpp)
target_link_libraries(Jumping-Frog.exe ${CURSES_LIBRARIES})