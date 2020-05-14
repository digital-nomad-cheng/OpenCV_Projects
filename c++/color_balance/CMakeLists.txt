cmake_minimum_required(VERSION 2.8)

project(template_matching)

set(EXECUTABLE_OUTPUT_PATH "/Users/vincent/Documents/Repo/opencv_projects")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O3")
if(CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wextra -pedantic -Wno-long-long")
message(STATUS "GNUCXX Compiler")
endif()

# set opencv directory
set(OpenCV_DIR "/Users/vincent/Lib/opencv-4.1.2/build/lib")
set(OpenCV_INCLUDE_DIRS "/Users/vincent/Lib/opencv-4.1.2/include") 
find_package(OpenCV REQUIRED)

add_executable(color_balance color_balance.cpp)

target_link_libraries(color_balance ${OpenCV_LIBS})

