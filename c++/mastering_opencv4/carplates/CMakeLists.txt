cmake_minimum_required (VERSION 3.0)

PROJECT(ANPR)
set(PROJECT_VERSION "0.1")
set(CMAKE_CXX_STANDARD 11)
set(EXCUTABLE_OUTPUT_PATH "/Users/vincent/Documents/Repo/opencv_projects/bin/") 

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

ADD_EXECUTABLE( prepare_svm_data prepare_svm_training_data.cpp )
TARGET_LINK_LIBRARIES( prepare_svm_data  ${OpenCV_LIBS} )

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OpenCV_LIBS})


# set(RESOURCES
#    README.txt
#    svm_training_data.xml
#    OCR.xml
#    test
#)

#foreach(f ${RESOURCES})
#file(COPY ${f} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)
#endforeach(f)

