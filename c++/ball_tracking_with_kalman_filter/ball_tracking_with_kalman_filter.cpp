#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#define MIN_H_BLUE 200
#define MAX_H_BLUE 300

int main() {
    cv::Mat frame;
    int stateSize = 6;
    int measureSize = 4;
    int contrSize = 0;

    unsigned int type = CV_32F;
    cv::KalmanFilter kf(stateSize, measureSize, contrSize, type);
    cv::Mat state(stateSize, 1, type);
    cv::Mat measure(measureSize, 1, type);
    
    // set transition state matrix A
    cv::setIdentity(kf.transitionMatrix);
    
    // set measure matrix
    kf.measurementMatrix.at<float>(0) = 1.0f;
    kf.measurementMatrix.at<float>(7) = 1.0f;
    kf.measurementMatrix.at<float>(16) = 1.0f;
    kf.measurementMatrix.at<float>(23) = 1.0f;
    
    // set noise covariance matrix
    // cv::setIdentity(kf.processNoiseCov, cv::Scalar(1e-2)
    kf.processNoiseCov.at<float>(0) = 1e-2;
    kf.processNoiseCov.at<float>(7) = 1e-2;
    kf.processNoiseCov.at<float>(14) = 5.0f;
    kf.processNoiseCov.at<float>(21) = 5.0f;
    kf.processNoiseCov.at<float>(28) = 1e-2;
    kf.processNoiseCov.at<float>(35) = 1e-2;

    // set measure noise matrix
    cv::setIdentity(kf.measurementNoiseCov, cv::Scalar(1e-1));

    cv::VideoCapture cap;
    if(!cap.open("resources/ball.mp4")) {
        std::cout << "Cannot open Webcam" << std::endl;
        return EXIT_FAILURE;
    }

    // cap.set(CV_CAP_PROP_FRAME_WIDTH, 1024);
    // cap.set(CV_CAP_PROP_FRAME_HEIGHT, 768);

    std::cout << "Enter 'Esc' to quit" << std::endl;

    char ch = 0;
    bool found = false;
    double ticks = 0;
    int notFoundCount = 0; 

    while (ch != 27) {
        double preTick = ticks;
        ticks = (double) cv::getTickCount();
        double dT = (ticks - preTick) / cv::getTickFrequency();
        cap >> frame;
        cv::Mat res;
        frame.copyTo(res);
        if (found) {
            kf.transitionMatrix.at<float>(2) = dT;
            kf.transitionMatrix.at<float>(9) = dT;

            state = kf.predict();
            std::cout << "State post:" << state << std::endl;
            cv::Rect predRect;
            predRect.width = state.at<float>(4);
            predRect.height = state.at<float>(5);
            predRect.x = state.at<float>(0) - predRect.width/2;
            predRect.y = state.at<float>(1) - predRect.height/2;
            cv::Point center;
            center.x = state.at<float>(0);
            center.y = state.at<float>(1);
            cv::circle(res, center, 2, CV_RGB(255, 0, 0), -1);
            cv::rectangle(res, predRect, CV_RGB(255, 0, 0), 2);
            
        }

        cv::Mat blur;
        cv::medianBlur(frame, blur, 5);
        cv::Mat frmHSV;
        cv::cvtColor(blur, frmHSV, CV_BGR2HSV);
        cv::Mat rangeRes = cv::Mat::zeros(frame.size(), CV_8UC1);
        // color threshold
        cv::inRange(frmHSV, cv::Scalar(MIN_H_BLUE/2, 100, 80), cv::Scalar(MAX_H_BLUE/2, 255, 255), rangeRes);
        cv::erode(rangeRes, rangeRes, cv::Mat(), cv::Point(-1, -1), 2);
        cv::dilate(rangeRes, rangeRes, cv::Mat(), cv::Point(-1, -1), 2);
        cv::imshow("threshold", rangeRes);

        std::vector<std::vector<cv::Point> > contours;
        cv::findContours(rangeRes, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

        std::vector<std::vector<cv::Point> > balls;
        std::vector<cv::Rect> ballsBox;
        
        // fiter detection results
        for (size_t i = 0; i < contours.size(); i++ ) {
            cv::Rect bbox;
            bbox  = cv::boundingRect(contours[i]);
            float ratio = (float) bbox.width / (float) bbox.height;
            if (ratio > 1.0f)
                ratio = 1.0f / ratio;
            if (ratio > 0.9 && bbox.area() >= 400) {
                balls.push_back(contours[i]);
                ballsBox.push_back(bbox);
            }
        }
        
        std::cout << "Number of balls found:" << ballsBox.size() << std::endl;
        
        for (size_t i = 0; i < balls.size(); i++) {
            cv::drawContours(res, balls, i, CV_RGB(20, 150, 20), 1);
            cv::rectangle(res, ballsBox[i], CV_RGB(0, 255, 0), 2);
            cv::Point center;
            center.x = ballsBox[i].x + ballsBox[i].width / 2;
            center.y = ballsBox[i].y + ballsBox[i].height / 2;
            std::stringstream sstr;
            sstr << "(" <<  center.x << "," << center.y << ")";
            cv::putText(res, sstr.str(), cv::Point(center.x + 3, center.y + 3),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, CV_RGB(20, 150, 20), 2);
        }


        // update kalman filter
        if (balls.size() == 0) {
            notFoundCount++;
            if (notFoundCount >= 100) {
                found = false;
            }
        }
        else {
            notFoundCount = 0;
            measure.at<float>(0) = ballsBox[0].x + ballsBox[0].width / 2;
            measure.at<float>(1) = ballsBox[0].y + ballsBox[0].height / 2;
            measure.at<float>(2) = (float)ballsBox[0].width;
            measure.at<float>(3) = (float)ballsBox[0].height;
            
            if (!found) { // first detection  
                kf.errorCovPre.at<float>(0) = 1;
                kf.errorCovPre.at<float>(7) = 1;
                kf.errorCovPre.at<float>(14) = 1;
                kf.errorCovPre.at<float>(21) = 1;
                kf.errorCovPre.at<float>(28) = 1;
                kf.errorCovPre.at<float>(35) = 1;
                
                state.at<float>(0) = measure.at<float>(0);
                state.at<float>(1) = measure.at<float>(1);
                state.at<float>(2) = 0;
                state.at<float>(3) = 0;
                state.at<float>(4) = measure.at<float>(2);
                state.at<float>(5) = measure.at<float>(3);
                
                kf.statePost = state;
                found = true;
            }
            else {
                kf.correct(measure); // Kalman Correction
            }
            std::cout << "Measure Matrix: " << measure << std::endl;
        } 
            
        cv::imshow("tracking", res);
        ch = cv::waitKey();
    }
    return EXIT_SUCCESS;
}
