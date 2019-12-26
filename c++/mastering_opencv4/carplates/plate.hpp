#ifndef Plate_hpp
#define Plate_hpp

#include <string.h>
#include <vector.h>

#include <opencv2/opencv.hpp>

class Plate
{
public:
    Plate();
    Plate(cv::Mat img, cv::Rect pos);
    std::string str();
    cv::Rect position;
    cv::Mat plate_img;
    std::vector<char> chars;
    std::vector<cv::Rect> chars_pos;
}

#endif
