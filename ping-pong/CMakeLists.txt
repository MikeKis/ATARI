cmake_minimum_required(VERSION 3.14)

project(ping-pong)

set(CMAKE_CXX_STANDARD 17)

include_directories(../../ArNI)
include_directories(.)
link_directories(/usr/local/lib) #it is - to find boost!
link_directories(../../ArNI/lib)

add_subdirectory(environment)
add_subdirectory(WorldModelTest)
if(ArNI_graphics)
	add_subdirectory(ping-pong-visualizer)
endif()

