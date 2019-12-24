cmake_minimum_required(VERSION 3.0)

project(landmarks)

set(CMAKE_CXX_STANDARD 11)
set(EXECUTABLE_OUTPUT_PATH "/home/idealabs/Work/tmp/opencv_projects/bin/mastering_opencv4")

set(OpenCV_DIR "/home/idealabs/Documents/LibModel/lib/opencv-3.4.8/build")
set(OpenCV_INCLUDE_DIR "/home/idealabs/Documents/LibModel/lib/opencv-3.4.8/include")
find_package(OpenCV REQUIRED)

set(SRC
    # demo.cpp
    main
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

