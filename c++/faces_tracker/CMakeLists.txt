cmake_minimum_required(VERSION 2.8)

project(faces_tracker)

set(EXECUTABLE_OUTPUT_PATH "~/Work/tmp/opencv_projects/bin")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wextra -pedantic -Wno-long-long -pthread")
message(STATUS "GNUCXX Compiler")
endif()

# set opencv directory
set(OpenCV_DIR "~/Documents/LibModel/lib/opencv-3.4.8/build")
set(OpenCV_INCLUDE_DIRS "~/Documents/LibModel/lib/opencv-3.4.8/include") 
find_package(OpenCV REQUIRED)

add_executable(faces_tracker faces_tracker.cpp)

target_link_libraries(faces_tracker ${OpenCV_LIBS})

