#include <iostream>
#include <opencv2/opencv.hpp>


int main(int argc, char **argv)
{
    cv::Mat img = cv::imread("resources/gull.jpg", -1);
    cv::resize(img, img, cv::Size(500, 500));
    cv::Mat mask = cv::Mat::zeros(cv::Size(img.cols+2, img.rows+2), CV_8UC1);
    cv::Point center;
    center.x = img.cols/2;
    center.y = img.rows/2;
    std::cout << center << std::endl;
    int flags = 4 | (255 << 8) | CV_FLOODFILL_MASK_ONLY; // | CV_FLOODFILL_FIXED_RANGE;
    cv::Scalar low_diff = cv::Scalar(10, 10, 10);
    cv::Scalar up_diff = cv::Scalar(10, 10, 10);
    cv::Canny(img, mask ,100, 200);
    cv::copyMakeBorder(mask, mask, 1, 1, 1, 1, cv::BORDER_REPLICATE);
    cv::floodFill(img, mask, center, cv::Scalar(255, 0, 0), 0, low_diff, up_diff,
        flags);
    cv::imshow("img", img);
    cv::imshow("mask", mask);
    cv::waitKey(0);
}

