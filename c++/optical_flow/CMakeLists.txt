cmake_minimum_required(VERSION 2.8)

project(optical_flow)

set(EXECUTABLE_OUTPUT_PATH "/home/ubuntu/ihandy/temp/opencv_projects/")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wextra -pedantic -Wno-long-long")
message(STATUS "GNUCXX Compiler")
endif()

# set opencv directory
set(OpenCV_DIR "/usr/local/lib")
set(OpenCV_INCLUDE_DIRS "/usr/local/include") 
find_package(OpenCV REQUIRED)

add_executable(optical_flow optical_flow.cpp)

target_link_libraries(optical_flow ${OpenCV_LIBS})

