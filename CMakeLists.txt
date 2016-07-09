cmake_minimum_required(VERSION 3.3)
project(checkers)

if(NOT MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(CPP_FILES main.cpp
			  checkers.cpp 
			  stdio_player.cpp
			  tcp_player.cpp
			  move.cpp
			  board.cpp
			  render.cpp)
set(HPP_FILES checkers.h)

add_executable(checkers ${CPP_FILES} ${HPP_FILES})
target_link_libraries(checkers SDL)
