cmake_minimum_required(VERSION 3.0)

project(landmarks_demo)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)

set(SRC
    demo.cpp
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

