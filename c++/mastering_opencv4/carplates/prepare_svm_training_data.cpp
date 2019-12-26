#include <iostream>
#include <vector>

#include <opencv2/opencv.hpp>

int main (int argc, char **argv) {
    std::cout << "Train SVM for Number Plate Recognition with OpenCV" << std::endl;
    char *plate_path;
    char *notplate_path;
    int num_plates;
    int num_notplates;
    const int image_width = 144;
    const int image_height = 33;

    if (argc >= 5) {
        num_plates = atoi(argv[1]);
        num_notplates = atoi(argv[2]);
        plate_path = argv[3];
        notplate_path = argv[4];
    } else {
        std::cout << "Usage: \n << argv[0] << 
        " <num plates > <num non plates> <path to plate files> " <<
        " <path to non plate files" << std::endl;
    }

    cv::Mat classes;
    cv::Mat training_data;
    cv::Mat training_images;
    std::vector<int> training_labels;

    for (int i = 0; i < num_plates; i++) {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << plate_path << i << ".jpg";
        cv::Mat img = cv::imread(ss.str(), 0);
        im = img.reshape(1, 1);
        training_imgs.push_back(img);
        training_labels.push_back(1);
    }

    for (int i = 0; i < num_noplates; i++) {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << noplate_path << i << ".jpg";
        cv::Mat img = imread(ss.str(), 0);
        img = img.reshape(1, 1);
        training_imgs.push_back(img);
        training_labels.push_back(0);
    }

    cv::Mat(training_images).copyTo(training_data); 
    training_data.convertTo(training_data, CV_32FC1);
    cv::Mat(traning_labels).copyTo(classes);

    cv::FileStorage fs("svm.xml", cv::FileStorage::WRITE);
    fs << "training_data" << training_data;
    fs << "classes" << classes;
    fs.release();

    return 0;
}


