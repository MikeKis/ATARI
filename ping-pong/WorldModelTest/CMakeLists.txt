cmake_minimum_required(VERSION 3.14)
# set(CMAKE_CXX_COMPILER "/usr/bin/g++")

project(WorldModelTest CXX)

add_library(${PROJECT_NAME} SHARED
  WorldModelTest.hpp
  WorldModelTestRea.cpp
  WorldModelTestRec.cpp
  ../ping-pong-environment.h
)

set(ROOT_NAMESPACE WorldModelTest)

################################################################################
# Output directory
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY    "${CMAKE_CURRENT_SOURCE_DIR}/../../Experiments"
        PREFIX ""
)

################################################################################
# Compile definitions
################################################################################
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "FOR_LINUX"
    "NDEBUG"
    "_USRDLL"
    "WORLDMODELTEST_EXPORTS"
)

target_compile_options(${PROJECT_NAME} PRIVATE
            -fPIC
            )

#target_link_options(${PROJECT_NAME} PRIVATE
#            -Wl,--no-undefined
#            )

target_link_libraries(${PROJECT_NAME} PUBLIC
        boost_system
        boost_filesystem
        boost_thread
	pthread
	rt
	ArNIUtilities
        )

                       
