#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

int max_Trackbar = 5;
int match_method;
float threshold = 0.9;
bool useThreshold = true;
cv::Mat source_img, template_img, similarity;

void MatchingMethod(int, void*);

int main(int argc, char** argv) {
    
    source_img = cv::imread("resources/source.jpg");
    template_img = cv::imread("resources/template.jpg");
    if(source_img.empty() || template_img.empty()) {
        std::cout << "Failed to read one of the images" << std::endl;
        return EXIT_FAILURE;
    }

    cv::namedWindow("source window", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("result window", cv::WINDOW_AUTOSIZE);
    const char* method_label = "Method: \n 0: SQDIFF \n 1: SQDIFF NORMED \n 2: TM CCORR \n 3: TM CCORR NORMED \n 4: TM COEFF \n 5: TM COEFF NORMED";
    cv::createTrackbar(method_label, "source window", &match_method, max_Trackbar, MatchingMethod);
     
    MatchingMethod(0, 0);
    cv::waitKey(0);
    return EXIT_SUCCESS;
}

void MatchingMethod(int, void*) {
    cv::Mat img_display;
    source_img.copyTo(img_display);
    int result_cols = source_img.cols - template_img.cols + 1;
    int result_rows = source_img.rows - template_img.rows + 1;
    similarity.create(result_rows, result_cols, CV_32FC1);
    cv::matchTemplate(source_img, template_img, similarity, match_method);
    cv::normalize(similarity, similarity, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
    if(!useThreshold) {
        double minVal, maxVal;
        cv::Point minLoc, maxLoc, matchLoc;
        minMaxLoc(similarity, &minVal, &maxVal, &minLoc, &maxLoc, cv::Mat());
        if(match_method == cv::TM_SQDIFF || match_method == cv::TM_SQDIFF_NORMED) {
            matchLoc = minLoc;
        } else {
            matchLoc = maxLoc;
        }
        cv::rectangle(img_display, matchLoc, cv::Point(matchLoc.x + template_img.cols, matchLoc.y + template_img.rows), cv::Scalar(255, 0, 0), 2, 8, 0);
        cv::imshow("source window", img_display);
        cv::imshow("result window", similarity);
    } else {
        for(int i = 0; i < similarity.rows; i++) {
            // const uchar *ptr = similarity.ptr(i);
            for(int j = 0; j < similarity.cols; j++) {
                // const uchar *ptr_pixel = ptr;
                // double sim = ptr_pixel[0];
                double sim = similarity.at<float>(i, j);
                if(match_method == cv::TM_SQDIFF || match_method == cv::TM_SQDIFF_NORMED) {
                    sim = 1.0 - sim;
                }
                if(sim > threshold) {
                    std::cout << "sim:" << sim << std::endl;
                    cv::Point matchLoc(j, i);
                    cv::rectangle(img_display, matchLoc, cv::Point(matchLoc.x + template_img.cols, matchLoc.y + template_img.rows), cv::Scalar(255, 0, 0), 2, 8, 0);
                    cv::imshow("source window", img_display);
                    cv::imshow("result window", similarity);
                }
                // ptr += 1;
                
            }
        }


    }

    return;
}


