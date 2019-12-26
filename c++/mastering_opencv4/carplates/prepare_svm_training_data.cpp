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
        std::cout << "Usage: \n" << argv[0] << 
        " <num plates > <num non plates> <path to plate files> " <<
        " <path to not plate files>" << std::endl;
    }

    cv::Mat classes;
    cv::Mat training_data;
    cv::Mat training_images;
    std::vector<int> training_labels;

    for (int i = 0; i < num_plates; i++) {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << plate_path << i << ".jpg";
        cv::Mat img = cv::imread(ss.str(), 0);
        if (img.empty()) {
            std::cout << "Failed to read image from" << ss.str() << std::endl;
        }
        img = img.reshape(1, 1);
        training_images.push_back(img);
        training_labels.push_back(1);
    }

    for (int i = 0; i < num_notplates; i++) {
        std::stringstream ss(std::stringstream::in | std::stringstream::out);
        ss << notplate_path << i << ".jpg";
        cv::Mat img = cv::imread(ss.str(), 0);
        if (img.empty()) {
            std::cout << "Failed to read image from" << ss.str() << std::endl;
        }
        img = img.reshape(1, 1);
        training_images.push_back(img);
        training_labels.push_back(0);
    }

    cv::Mat(training_images).copyTo(training_data); 
    training_data.convertTo(training_data, CV_32FC1);
    cv::Mat(training_labels).copyTo(classes);

    cv::FileStorage fs("svm.xml", cv::FileStorage::WRITE);
    fs << "training_data" << training_data;
    fs << "training_labels" << classes;
    fs.release();

    return 0;
}


