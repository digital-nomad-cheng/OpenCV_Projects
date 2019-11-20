cmake_minimum_required(VERSION 2.8)

project(optical_flow)

set(EXECUTABLE_OUTPUT_PATH "/home/idealabs/Work/tmp/opencv_projects/bin")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wextra -pedantic -Wno-long-long")
message(STATUS "GNUCXX Compiler")
endif()

# set opencv directory
set(OpenCV_DIR "/home/idealabs/Documents/LibModel/lib/opencv-3.4.8/build")
set(OpenCV_INCLUDE_DIRS "/home/idealabs/Documents/LibModel/lib/opencv-3.4.8/include") 
find_package(OpenCV REQUIRED)

add_executable(ball_tracking ball_tracking_with_kalman_filter.cpp)

target_link_libraries(ball_tracking ${OpenCV_LIBS})

