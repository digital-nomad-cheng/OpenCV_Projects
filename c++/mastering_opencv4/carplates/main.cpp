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
    
    
    cv::FileStorage fs;
    fs.open("SVM.xml", FileStorage::READ);
    cv::Mat svm_training_data;
    cv::Mat svm_training_label;
    fs["training_data"] >> svm_training_data;
    fs["training_labels"] >> svm_training_label;
    cv::Ptr<cv::SVM> svm_classifier = cv::ml::SVM::create();
    svm_classifier->setType(cv::ml::SVM::C_SVC);
    svm_classifier->setKernel(cv::ml::SVM::LINEAR);
    svm_classifier->setDegree(0.0):
    svm_classifier->setGamma(1.0);
    svm_classifier->setCoef0(0):
    svm_classifier->setC(0);
    svm_classifier->setNu(0.0);
    svm_classifier->setP(0);
    svm_classifier->setTermCriteria(TermCriteria(TermCriteria::MAX_ITER, 1000, 0.01)):
    
    cv::Ptr<TrainData> train_data = TrainData::Create(svm_training_data,
                                                      row_sample,
                                                      svm_training_label);
    svm_classifier->train(train_data);


    return 0;

}
