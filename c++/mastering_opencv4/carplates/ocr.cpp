#include "ocr.hpp"

const char OCR::str_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', 
                                    '9', 'B', 'C', 'D', 'F', 'G', 'H', 'J', 'K',
                                    'L', 'M', 'N', 'P', 'R', 'S', 'T', 'V', 'W',
                                    'X', 'Y', 'Z'};
const int OCR::num_chars = 30;

CharSegment::CharSegment(){};
CharSegment::CharSegment(cv::Mat i, cv::Rect p)
{
    img = i;
    pos = p;
}

OCR::OCR()
{
    DEBUG = false;
    trained = false;
    save_segments = false;
    char_size = 30;
}

OCR::OCR(std::string train_file)
{
    DEBUG = false;
    trained = false;
    save_segments = false;
    char_size = 20;

    cv::FileStorage fs;
    fs.open(train_file, cv::FileStorage::READ);
    cv::Mat training_data;
    cv::Mat labels;

    fs["training_data_f15"] >> training_data;
    fs["classes"] >> labels;

    train(training_data, labels, 10);

    dnn_net = cv::dnn::readNetFromTensorflow("model.pb");
}

cv::Mat OCR::preprocessChar(cv::Mat in)
{
    int h = in.rows;
    int w = in.cols;
    cv::Mat trans_mat = cv::Mat::eye(2, 3, CV_32F);
    int m = std::max(w, h);
    trans_mat.at<float>(0, 2) = m/2 - w/2;
    trans_mat.at<float>(1, 2) = m/2 - h/2;

    cv::Mat warp_image(m, m, in.type());
    cv::warpAffine(in, warp_image, trans_mat, warp_image.size(), cv::INTER_LINEAR,
                    cv::BORDER_CONSTANT, cv::Scalar(0));
    cv::Mat out;
    cv::resize(warp_image, out, cv::Size(char_size, char_size));

    return out;
}

bool OCR::verifySizes(cv::Mat r)
{
    float aspect = 45.0f/77.0f;
    float char_aspect = (float)r.cols / (float)r.rows;
    float error = 0.35;
    float min_height = 15;
    float max_height = 28;
    float min_aspect = 0.2;
    float max_aspect = aspect+aspect*error;
    float area = countNonZero(r);
    float bb_area = r.cols*r.rows;
    float perc_pixels = area/bb_area;
    float per_pixels = area/bb_area;
    
    if (DEBUG) {
        std::cout << "Aspect: " << aspect << " [" << min_aspect << "," << 
            max_aspect << "] " << "Area " << per_pixels << " Char aspect " <<
            char_aspect << " Height char " << r.rows << "\n";
    }
    if (per_pixels < 0.8 && char_aspect > min_aspect && char_aspect < max_aspect
        && r.rows >= min_height && r.rows < max_height) {
        return true;
    } else {
        return false;
    }

}

std::vector<CharSegment> OCR::segment(Plate plate)
{
    cv::Mat input = plate.plate_img;
    std::vector<CharSegment> output;
    cv::Mat img_threshold;
    cv::threshold(input, img_threshold, 60, 255, CV_THRESH_BINARY_INV);

    if (DEBUG) {
        cv::imshow("threshold plate", img_threshold);
    }

    cv::Mat img_contours;
    img_threshold.copyTo(img_contours);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(img_contours, contours, CV_RETR_EXTERNAL,
                                             CV_CHAIN_APPROX_NONE);
    cv::Mat result;
    img_threshold.copyTo(result);
    cv::cvtColor(result, result, cv::COLOR_BGR2RGB);
    cv::drawContours(result, contours, -1, cv::Scalar(255, 0, 0), 1);
    std::vector<std::vector<cv::Point>>::iterator it = contours.begin();
    while (it!=contours.end()) {
        cv::Rect mr = boundingRect(cv::Mat(*it));
        cv::rectangle(result, mr, cv::Scalar(0, 255, 0));
        cv::Mat aux_ROI(img_threshold, mr);
        if (verifySizes(aux_ROI)) {
            aux_ROI = preprocessChar(aux_ROI);
            output.push_back(CharSegment(aux_ROI, mr));
            cv::rectangle(result, mr, cv::Scalar(0, 125, 255));
        }
        ++it;
    }
    if (DEBUG) {
        std::cout << "Num chars: " <<output.size() << std::endl;
        cv::imshow("segmented chars", result);
    }
    return output;
}

cv::Mat OCR::projectHistogram(cv::Mat img, int t)
{
    int size = (t)? img.rows: img.cols;
    cv::Mat mhist = cv::Mat::zeros(1, size, CV_32F);
    for (int j = 0; j < size; j++) {
        cv::Mat data = (t)?img.row(j):img.col(j);
        mhist.at<float>(j) = countNonZero(data);
    }
    double min, max;
    cv::minMaxLoc(mhist, &min, &max);

    if (max > 0){
        mhist.convertTo(mhist, -1, 1.0f/max, 0);
    }

    return mhist;
}

cv::Mat OCR::getVisualHistogram(cv::Mat *hist, int type)
{
    int size = 100;
    cv::Mat im_hist;
    
    if (type==HORIZONTAL) {
        im_hist.create(cv::Size(size, hist->cols), CV_8UC3);
    } else {
        im_hist.create(cv::Size(hist->cols, size), CV_8UC3);
    }
    // ?
    im_hist = cv::Scalar(55, 55, 55);
    for (int i = 0; i < hist->cols; i++) {
        float value = hist->at<float>(i);
        int max_val = (int)(value*size);

        cv::Point p1, p2, p3, p4;

        if (type==HORIZONTAL) {
            p1.x = p3.x = 0;
            p2.x = p4.x = max_val;
            p1.y = p2.y = i;
            p3.y = p4.y = i+1;

            cv::line(im_hist, p1, p2, CV_RGB(220, 220, 220), 1, 8, 0);
            cv::line(im_hist, p3, p4, CV_RGB(34, 34, 34), 1, 8, 0);

            p3.y = p4.y = i+2;
            cv::line(im_hist, p3, p4, CV_RGB(44, 44, 44), 1, 8, 0);
            p3.y = p4.y = i+3;
            cv::line(im_hist, p3, p4, CV_RGB(50, 50, 50), 1, 8, 0);
        } else {
            p1.x = p2.x = i;
            p3.x = p4.x = i+1;
            p1.y = p3.y = 100;
            p2.y = p4.y = 100 - max_val;

            cv::line(im_hist, p1, p2, CV_RGB(220, 220, 220), 1, 8, 0);
            cv::line(im_hist, p3, p4, CV_RGB(24, 24, 24), 1, 8, 0);
            p3.x = p4.x = i+2;
            cv::line(im_hist, p3, p4, CV_RGB(44, 44, 44), 1, 8, 0);
            p3.x = p4.x = i+3;
            cv::line(im_hist, p3, p4, CV_RGB(50, 50, 50), 1, 8, 0);
        }
    }
    return im_hist;
}

    

                                        


