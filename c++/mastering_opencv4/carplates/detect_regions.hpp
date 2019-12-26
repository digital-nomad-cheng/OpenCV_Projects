#ifndef DetectRegions_hpp
#define DetectRegions_hpp

#include <string>
#include <vector>

#include "plate.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

class DetectRegions
{
public:
    DetectRegions();
    void setFilename(std::string f);
    std::vector<Plate> run(cv::Mat input);
    
    std::string filename;
    bool save_regions;
    bool show_steps;
    
private:    
   std::vector<Plate> segment(cv::Mat input);
   bool verifySizes(cv::RotatedRect mr);
   cv::Mat histEq(cv::Mat in);
};

#endif 
