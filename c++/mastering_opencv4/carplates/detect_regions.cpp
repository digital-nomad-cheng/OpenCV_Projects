#include "detect_regions.hpp"

void DetectRegions::setFilename(std::string s) 
{
    filename = s;
}

DetectRegions::DetectRegions() 
{
    show_steps = false;
    save_regions = false;
}

bool DetectRegions::verifySizes(cv::RotatedRect mr)
{
    float error = 0.4;
    float aspect = 4.7272; // car plate aspect, 52/11
    // min and max area
    int min = 15*aspect*15;
    int max = 125*aspect*125;
    // min and max aspect ratio
    float rmin = aspect - aspect*error;
    float rmax = aspect + aspect*error;

    int area = mr.size.height * mr.size.width;

    float r = (float)mr.size.width / (float)mr.size.height;
    if (r < 1) {
        r = (float)mr.size.height / (float)mr.size.width;
    }

    if ((area < min || area > max) || (r < min || r > max)) {
        return false;
    } else {
        return true;
    }
}

cv::Mat DetectRegions::histEq(cv::Mat in)
{
    cv::Mat out(in.size(), in.type());
    if (in.channels() == 3) {
        cv::Mat hsv;
        std::vector<cv::Mat> hsv_split;
        cv::cvtColor(in, hsv, cv::COLOR_BGR2HSV);
        cv::split(hsv, hsv_split);
        cv::equalizeHist(hsv_split[2], hsv_split[2]);
        cv::merge(hsv_split, hsv);
        cv::cvtColor(hsv, out, cv::COLOR_HSV2BGR);
    } else if (in.channels() == 1) {
        cv::equalizeHist(in, out);
    }
    return out;
}

std::vector<Plate> DetectRegions::segment(cv::Mat input)
{
    std::vector<Plate> output;
    cv::Mat img_gray;
    cv::cvtColor(input, img_gray, cv::COLOR_BGR2GRAY);
    cv::blur(img_gray, img_gray, cv::Size(5, 5));
    
    cv::Mat img_sobel;
    cv::Sobel(img_gray, img_sobel, CV_8U, 1, 0, 3, 1, 0, cv::BORDER_DEFAULT);
    if (show_steps) {
        cv::imshow("sobel", img_sobel);
    }

    cv::Mat img_threshold;
    cv::threshold(img_sobel, img_threshold, 0, 255, CV_THRESH_OTSU+
                                                    CV_THRESH_BINARY);
    
    if (show_steps) {
        cv::imshow("threshold", img_threshold);
    }

    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(17, 3));
    cv::morphologyEx(img_threshold, img_threshold, CV_MOP_CLOSE, element);
    if (show_steps) {
        cv::imshow("close", img_threshold);
    }

    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(img_threshold, contours, CV_RETR_EXTERNAL, 
                                              CV_CHAIN_APPROX_NONE);
    
    std::vector<std::vector<cv::Point>>::iterator it = contours.begin();
    std::vector<cv::RotatedRect> rects;
    while (it != contours.end()) {
        cv::RotatedRect mr = cv::minAreaRect(cv::Mat(*it));
        if (!verifySizes(mr)) {
            it = contours.erase(it);
        } else {
            ++it;
            rects.push_back(mr);
        }
    }

    cv::Mat result;
    input.copyTo(result);
    cv::drawContours(result, contours, -1, cv::Scalar(255, 0, 0), 1);
    for (int i = 0; i < rects.size(); i++) {
        cv::circle(result, rects[i].center, 3, cv::Scalar(0, 255, 0), -1);
        float min_size = (rects[i].size.width < rects[i].size.height) ? 
                        rects[i].size.width:rects[i].size.height;
        min_size = min_size - min_size*0.5;
        std::srand(time(NULL));
        cv::Mat mask;
        mask.create(input.rows+2, input.cols+2, CV_8UC1);
        mask = cv::Scalar::all(0);
        int low_diff = 30;
        int up_diff = 30;
        int connectivity = 4;
        int new_mask_val = 255;
        int num_seeds = 10;
        cv::Rect ccomp;
        int flags = connectivity + (new_mask_val << 8) + 
            CV_FLOODFILL_FIXED_RANGE + CV_FLOODFILL_MASK_ONLY;
        for (int j = 0; j < num_seeds; j++) {
            cv::Point seed;
            seed.x = rects[i].center.x + rand() % (int)min_size - (min_size/2);
            seed.y = rects[i].center.y + rand() % (int)min_size - (min_size/2);
            cv::circle(result, seed, 1, cv::Scalar(0, 255, 255), -1);
            int area = cv::floodFill(input, mask, seed, cv::Scalar(255, 0, 0),
                                     &ccomp,
                                     cv::Scalar(low_diff, low_diff, low_diff),
                                     cv::Scalar(up_diff, up_diff, up_diff));
        }
        if (show_steps) {
            cv::imshow("mask", mask);
        }
        
        std::vector<cv::Point> points_interest;
        cv::Mat_<uchar>::iterator it = mask.begin<uchar>();
        cv::Mat_<uchar>::iterator end = mask.end<uchar>();
        for (; it != end; ++it) {
            if (*it=255) {
                points_interest.push_back(it.pos());
            }
        }
        
        cv::RotatedRect min_rect = cv::minAreaRect(points_interest);
        if (verifySizes(min_rect)) {
            cv::Point2f rect_points[4];
            min_rect.points(rect_points);
            for (int j = 0; j < 4; j++) {
                cv::line(result, rect_points[j], rect_points[(j+1)%4], 
                         cv::Scalar(0, 0, 255), 1, 8);
            }
            float r = (float)min_rect.size.width / (float)min_rect.size.height;
            float angle = min_rect.angle;
            if ( r< 1) {
                angle = 90 + angle;
            }
            cv::Mat rot_mat = cv::getRotationMatrix2D(min_rect.center, angle, 1);
            cv::Mat img_rotated;
            cv::warpAffine(input, img_rotated, rot_mat, input.size(), CV_INTER_CUBIC);
            
            cv::Size rect_size = min_rect.size;
            if (r < 1) {
                cv::swap(rect_size.width, rect_size.height);
            }

            cv::Mat img_crop;
            cv::getRectSubPix(img_rotated, rect_size, min_rect.center, img_crop);

            cv::Mat result_resized;
            result_resized.create(33, 144, CV_8UC3);
            cv::resize(img_crop, result_resized, result_resized.size(), 0, 0, 
                                                            cv::INTER_CUBIC);
            cv::Mat gray_result;
            cv::cvtColor(result_resized, gray_result, cv::COLOR_BGR2GRAY);
            cv::blur(gray_result, gray_result, cv::Size(3, 3));
            gray_result = histEq(gray_result);
            if (save_regions) {
                std::stringstream ss(std::stringstream::in | std::stringstream::out);
                ss << "tmp/" << filename << "_" << i << ".jpg";
                cv::imwrite(ss.str(), gray_result);
            }
            output.push_back(Plate(gray_result, min_rect.boundingRect()));
        }   
    }
    if (show_steps) {
        cv::imshow("contours", result);
    }
    return output;
}

std::vector<Plate> DetectRegions::run(cv::Mat input)
{
    std::vector<Plate> tmp = segment(input);
    return tmp;
}


