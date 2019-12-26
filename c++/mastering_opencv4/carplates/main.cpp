#include <iostream>
#include <vector>

#include "detect_regions.hpp"
#include "ocr.hpp"

#include <opencv2/opencv.hpp>

std::string getFilename(std::string s)
{
    char sep = '/';
    char sep_ext = '.';
    
    size_t i = s.rfind(sep, s.length());
    if (i != std::string::npos) { // not no match
        std::string fn = (s.substr(i+1, s.length()-1));
        size_t j = fn.rfind(sep_ext, fn.length());
        if (i != std::string::npos) {
            return fn.substr(0, j);
        } else {
            return fn;
        }
    } else {
        return "";
    }
}

int main(int argc, char **argv)
{
    std::cout << "OpenCV Automatic Number Plate Recognition" << std::endl;
    char *filename;
    cv::Mat input_image;

    if (argc >=2) {
        filename = argv[1];
        input_image = cv::imread(filename, 1);
    } else {
        printf("Use:\n %s image \n", argv[0]);
        return 0;
    }

    std::string filename_no_ext = getFilename(filename);
    std::cout << "working with file: " << filename_no_ext << std::endl;

    return 0;

}
