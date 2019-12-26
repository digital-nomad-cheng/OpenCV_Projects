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
    
    cv::FileStorage fs;
    fs.open("svm.xml", cv::FileStorage::READ);
    cv::Mat svm_training_data;
    cv::Mat svm_training_label;
    fs["training_data"] >> svm_training_data;
    fs["training_labels"] >> svm_training_label;
    std::cout << "Successfully load SVM training data" << std::endl;
    cv::Ptr<cv::ml::SVM> svm_classifier = cv::ml::SVM::create();
    svm_classifier->setType(cv::ml::SVM::C_SVC);
    svm_classifier->setKernel(cv::ml::SVM::LINEAR);
    svm_classifier->setDegree(0.0);
    svm_classifier->setGamma(1.0);
    svm_classifier->setCoef0(0);
    svm_classifier->setC(1);
    svm_classifier->setNu(0.0);
    svm_classifier->setP(0);
    svm_classifier->setTermCriteria(
            cv::TermCriteria(cv::TermCriteria::MAX_ITER, 1000, 0.01));
    
    cv::Ptr<cv::ml::TrainData> train_data = cv::ml::TrainData::create(svm_training_data,
                                                      cv::ml::ROW_SAMPLE,
                                                      svm_training_label);
    svm_classifier->train(train_data);
    std::cout <<"Finished training SVM classifier" << std::endl;
    

    std::string filename_no_ext = getFilename(filename);
    std::cout << "working with file: " << filename_no_ext << std::endl;
    DetectRegions detectRegions;
    detectRegions.setFilename(filename_no_ext);
    detectRegions.save_regions = false;
    detectRegions.show_steps = true;
    std::vector<Plate> possible_regions = detectRegions.run(input_image);
    std::cout << "Num possible regions: " << possible_regions.size() << std::endl;
    std::vector<Plate> plates;
    for (int i = 0; i < possible_regions.size(); i++) {
        cv::Mat img = possible_regions[i].plate_img;
        cv::imshow("candicate plate", img);
        cv::waitKey(0);
        cv::Mat p = img.reshape(1, 1);
        p.convertTo(p, CV_32FC1);
        int response = (int)svm_classifier->predict(p);
        if (response == 1) {
            plates.push_back(possible_regions[i]);
        }
    }
    std::cout << "Num plates detected: " << plates.size() << std::endl;
    cv::waitKey(0);
    return 0;

}
