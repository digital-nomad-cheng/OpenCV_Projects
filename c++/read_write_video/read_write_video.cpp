/*
 * https://docs.opencv.org/master/d7/d9e/tutorial_video_write.html
 *
 */


#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

int main() {
    cv::VideoCapture cap("./resources/face_tracking_test_video.mp4");
    
    int ex = static_cast<int>(cap.get(cv::CAP_PROP_FOURCC));
    if(!cap.isOpened()) {
        std::cout << "Failed to open video stream from camera or file" << std::endl;
        return -1;
    }
    cv::Mat frame;
    if(!cap.read(frame)) {
        std::cout << "Read frame error!" << std::endl;
    }

    int width = frame.cols;
    int height = frame.rows;
    
    cv::VideoWriter video("resources/face_tracking_write_video.mp4", ex, cap.get(cv::CAP_PROP_FPS), cv::Size(width, height));

    if(!video.isOpened()) {
        std::cout << "Output video could not be opened!" << std::endl;
        return -1;
    }

    video.write(frame);
    while(1) {
        if(!cap.read(frame)) {
            std::cout << "Read frame error!" << std::endl;
            break;
        }
        if(frame.empty()) {
            break;
        }
        int q = cv::waitKey(1); // wait for 1 millisecond for pressing key
        if(q == 27) break; // press ESC key to break
        cv::imshow("frame", frame);
        video.write(frame);
    }
    video.release();
    cap.release();
    cv::destroyAllWindows();
    return 0;
}
