cmake_minimum_required (VERSION 3.0)

PROJECT(ANPR)
set(PROJECT_VERSION "0.1")

set(CMAKE_CXX_STANDARD 11)

# requires OpenCV
# FIND_PACKAGE(OpenCV 4.0.0 REQUIRED)
FIND_PACKAGE(OpenCV REQUIRED)
MESSAGE("OpenCV version: ${OpenCV_VERSION}")
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

set(SRC
    main.cpp
    detect_regions.cpp
    # ocr.cpp
    plate.cpp
)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS})


# set(RESOURCES
#    README.txt
#    SVM.xml
#    OCR.xml
#    test
#)

#foreach(f ${RESOURCES})
#file(COPY ${f} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
#endforeach(f)

