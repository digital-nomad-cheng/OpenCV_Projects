#ifndef OCR_hpp
#define OCR_hpp

#include <string>
#include <vector>

#include "plate.hpp"

#include <opencv2/opencv.hpp>

#define HORIZONTAL 1
#define VERTICAL 0

class CharSegment
{
public:
    CharSegment();
    CharSegment(cv::Mat i, cv::Rect p);
    cv::Mat img;
    cv::Rect pos;
};

class OCR
{
public:
    OCR();
    OCR(std::string train_file);
    std::string run(Plate *input);
    cv::Mat preprocessChar(cv::Mat in);
    int classify(cv::Mat in);
    void train(cv::Mat train_data, cv::Mat train_label, int n_layers);
    int classifyKnn(cv::Mat in);
    void trainKnn(cv::Mat train_samples, cv::Mat train_labels, int k);
    cv::Mat features(cv::Mat intput, int size);

    bool DEBUG;
    bool save_segments;
    std::string filename;
    int char_size;
    static const int num_chars;
    static const char str_chars[];

private:
    bool trained;
    std::vector<CharSegment> segment(Plate in);
    cv::Mat preprocess(cv::Mat in, int new_size);
    cv::Mat getVisualHistogram(cv::Mat *hist, int type);
    void drawVisualFeatures(cv::Mat character, cv::Mat hhist, cv::Mat vhist,
                                                              cv::Mat low_data);
    cv::Mat projectHistogram(cv::Mat img, int t);
    bool verifySizes(cv::Mat in);
    cv::Ptr<cv::ml::ANN_MLP> ann;
    cv::Ptr<cv::ml::KNearest> knn;
    cv::dnn::Net dnn_net;
    int K;
};

#endif
