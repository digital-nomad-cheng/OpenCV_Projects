#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/video/video.hpp>

int main(int argc, char **argv) {
    /*
    const std::string about = "This is a demo which demonstrates the usage of Optical Flow for object tracking";
    const std::string keys = 
        "{ h help |         | print this help message}"
        "{@image  | test.avi| path to img file}";
    cv::CommandLineParser parser(argc, argv, keys);
    parser.about(about);
    */

    const std::string file_path = "resources/face_tracking_test_video_1.mp4";
    cv::VideoCapture cap(file_path);
    if(!cap.isOpened()) {
        std::cerr << "Unable to open file!" << std::endl;
        return EXIT_FAILURE;
    }
    
    cv::RNG rng;
    std::vector<cv::Scalar> colors;
    for(int i = 0; i < 100; i++) {
        int r = rng.uniform(0, 256);
        int g = rng.uniform(0, 256);
        int b = rng.uniform(0, 256);
        colors.push_back(cv::Scalar(r, g, b));
    }

    int frame_idx = 0;   
    int detect_frame_interval = 30;
    cv::Mat old_frame, old_gray;
    cv::Mat frame, frame_gray;
    std::vector<cv::Point2f> p0, p1;
    cap >> old_frame;
    if(old_frame.empty()) return -1;
    frame_idx += 1;
    cv::cvtColor(old_frame, old_gray, cv::COLOR_BGR2GRAY);
    cv::goodFeaturesToTrack(old_gray, p0, 100, 0.3, 7, cv::Mat(), 7, false, 0.04);
    cv::Mat mask = cv::Mat::zeros(old_frame.size(), old_frame.type());
    while(true) {
        cap >> frame;
        if(frame.empty()) break;
        frame_idx += 1;
        cv::cvtColor(frame, frame_gray, cv::COLOR_BGR2GRAY);
        
        // calculate optical flow
        std::vector<uchar> status;
        std::vector<float> err;
        cv::TermCriteria criteria = cv::TermCriteria((cv::TermCriteria::COUNT) + (cv::TermCriteria::EPS), 10, 0.03);
        cv::calcOpticalFlowPyrLK(old_gray, frame_gray, p0, p1, status, err, cv::Size(11, 11), 2, criteria);

        std::vector<cv::Point2f> good_points;
        for(uint i = 0; i < p0.size(); i++) {
            if(status[i] == 1) {
                good_points.push_back(p1[i]);
                cv::line(mask, p1[i], p0[i], colors[i], 2);
                cv::circle(frame, p1[i], 5, colors[i], -1);
            }
        }

        cv::Mat img;
        cv::add(frame, mask, img);
        cv::imshow("Frame", img);
        int k = cv::waitKey(30);
        if(k == 'q' || k == 27) {
            break;
        }
        old_gray = frame_gray.clone();
        p0 = good_points;
    
        if(frame_idx % detect_frame_interval == 0) {
            cv::goodFeaturesToTrack(old_gray, p0, 100, 0.3, 7, cv::Mat(), 7, false, 0.04);
            mask.setTo(0);
        }

    }
}
