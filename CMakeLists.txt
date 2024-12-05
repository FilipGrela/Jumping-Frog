cmake_minimum_required(VERSION 3.30)
project(Jumping-Frog)

set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc++ -nodefaultlibs -lc") # Disable STL

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

add_executable(Jumping-Frog.exe main.cpp)
target_link_libraries(Jumping-Frog.exe ${CURSES_LIBRARIES} c)
