#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <opencv2/opencv.hpp>
#include "cartoon.hpp"
#include "fps_timer.hpp"

const int DEFAULT_CAMERA_WIDTH = 640;
const int DEFAULT_CAMERA_HEIGHT = 480;
const char *DEFAULT_CAMERA_NUMBER = "0";
const int NUM_STICK_FIGURE_ITERATIONS = 40;
const char *windowName = "Cartoonifier";

auto m_sketchMode = true;
auto m_alienMode = false;
auto m_evilMode = false;
auto m_debugMode = false;

int m_stickFigureIterations = 0;

#if !defined VK_ESCAPE
    #define VK_ESCAPE 0x1B
#endif

void initCamera(cv::VideoCapture &videoCapture, char *cameraNumber) {
    try {
        if(isdigit(cameraNumber[0]) {
            videoCapture.open(atoi(cameraNumber));
        }
    } catch(cv::Exception &e) {}
    
    if(!videoCapture.isOpened()) {
        try {
            videoCapture.open(cameraNumber); // open as video file or URL
        } catch(cv::Exception &e) {}
    }
    
    if(!videoCapture.isOpened()) {
        std::cerr << "ERROR: could not access the camera " << cameraNumber << " !" << std::endl; 
        exit(1);
    }
    
   std::cout << "Loaded camera " << cameraNumber << std::endl;
}
 
void onKeyPress(char key) {
    swith(key) {
    case 's': 
        m_sketchMode = !m_sketchMode;
        std::cout << "Sketch / Paint Mode:" << m_sketchMode << std::endl;
        break;
    case 'a':
        m_alineMode = !m_alineMode;
        std::cout << "Alien / Human Mode:" << m_alienMode << std::endl;
        if(m_alienMode) {
            m_stickFigureIterations = NUM_STICK_FIGURE_ITERATIONS;
        }
        break;
    case 'e':
        m_evilMode = !m_evilMode;
        std::cout << "Evil / Good Mode:" << m_evilMode << std::endl;
        break;
    case 'd':
        m_debugMode = !m_debugMode;
        std::cout << "Debug Mode:" << m_debugMode << std::endl;
        break;
    }
}


int main(int argc, char *argv) {
    std::cout << "Converts real-life images to cartoon-like images." << std::endl;
    std::cout << "Compiled with OpenCV version " << CV_VERSION << std::endl;
    std::cout << "usage:   " << argv[0] << " [[camera_number] desired_width desired_height ]" << std::endl;
    std::cout << "default: " << argv[0] << " " << DEFAULT_CAMERA_NUMBER << " " << DEFAULT_CAMERA_WIDTH << " " << DEFAULT_CAMERA_HEIGHT << std::endl;
    std::cout << std::endl;

    std::cout << "Keyboard commands (press in the GUI window):" << std::endl;
    std::cout << "    Esc:  Quit the program." << std::endl;
    std::cout << "    s:    change Sketch / Paint mode." << std::endl;
    std::cout << "    a:    change Alien / Human mode." << std::endl;
    std::cout << "    e:    change Evil / Good character mode." << std::endl;
    std::cout << "    d:    change debug mode." << std::endl;
    std::cout << std::endl;

    char *cameraNumber = (char*)DEFAULT_CAMERA_NUMBER;
    int desiredCameraWidth = DEFAULT_CAMERA_WIDTH;
    int desiredCameraHeight = DEFAULT_CAMERA_HEIGHT;
    
    auto a = 1;
    if (argc > a) {
        cameraNumber = argv[a];
        a++;    // Next arg

        // Allow the user to specify camera resolution.
        if (argc > a) {
            desiredCameraWidth = atoi(argv[a]);
            a++;    // Next arg

            if (argc > a) {
                desiredCameraHeight = atoi(argv[a]);
                a++;    // Next arg
            }
        }
    }
    
    cv::VideoCapture camera;
    initCamera(camera, cameraNumber);
    
    camera.set(cv::CAP_PROP_FRAME_WIDTH, desiredCameraWidth);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, desiredCameraHeight);
    
    namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::setWindowProperty(windowName, cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    
    fps_timer timer;
    
    while(true) {
        cv::Mat cameraFrame;    
        camera >> cameraFrame;
        if(cameraFrame.empty()){
            std::cerr << "ERROR: Couldn't grab the next camera frame." << std::endl;
            exit(1);
        }
        cv::Mat displayedFrame = cv::Mat(cameraFrame.size, CV_8UC3);
        auto debugType = 0;
        if(m_debugMode) {
            debugType = 2;
        }
        cartoonifyImage(cameraFrame, displayedFrame, m_sketchMode, m_alienMode, m_evilMode, debugType);
        if (m_stickFigureIterations > 0) {
            drawFaceStickFigure(displayedFrame);
            m_stickFigureIterations--;
        }
        
        timer.increment();
        if (timer.fnum == 0) {
            double fps;
            if (timer.fps < 1.0f)
                fps = timer.fps;                // FPS is a fraction
            else
                fps = (int)(timer.fps + 0.5f);  // FPS is a large number
            std::cout << fps << " FPS" << std::endl;
        }

        cv::imshow(windowName, displayedFrame);
        auto keypress = cv::waitKey(20);  // This is needed if you want to see anything!
        if (keypress == VK_ESCAPE) {   // Escape Key
            // Quit the program!
            break;
        }
        // Process any other keypresses.
        if (keypress > 0) {
            onKeyPress(keypress);
        }
    } // end while
    return EXIT_SUCCESS;
}
