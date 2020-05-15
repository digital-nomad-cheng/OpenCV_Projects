#include <iostream>
#include <opencv2/opencv.hpp>

void balance_color(cv::Mat &img)
{
    /* Idea is from here: https://web.stanford.edu/~sujason/ColorBalancing/simplestcb.html
     * A well balanced photo, the brightest color should be white and the darkest
     * black.Thus, we can remove the color cast from an image by scaling the 
     * histograms of each of the R, G, B channels so that they span the complete
     * 0-255 scale.
     */

	// calculate image histogram
	int num_pixels = img.cols * img.rows;
	int channels = 0;
	int hist_size = 256;
	float pranges[2] = {0,255};
    const float* ranges[1] = {pranges};
	cv::Mat hist;
	cv::calcHist(&img, 1, &channels, cv::Mat(), hist, 1, &hist_size, ranges);
	// hist: rows: 256, cols: 1
    // std::cout << hist << std::endl;	
	
    // calculate cumulative distribute function
    for (size_t i = 1; i < hist.rows; i++) {
		hist.at<float>(i, 0) += hist.at<float>(i-1, 0);
	}

	double s = 0.0265; //此参数可以调整，最好在0.1以下(0=<s<=1)

	int min_index = 0;
    
	while (hist.at<float>(min_index, 0) < num_pixels * s/2) {
		min_index += 1;
	}

	int max_index = 255;
    
	while (hist.at<float>(max_index, 0) > num_pixels * (1 - s/2)) {
		max_index -= 1;
	}
    
	// min_index and max_index represents the over dark and over light pixels in img
	for (size_t x = 0; x < img.rows; x++) {
		uchar *ptr = img.ptr(x);
		for (size_t y = 0; y < img.cols; y++) {
			if (*ptr > max_index)
				*ptr = max_index;
			if (*ptr < min_index)
				*ptr = min_index;
            ptr += 1;
		}
	}

	// normalzie to 0 ~ 255 (scale)
    cv::normalize(img, img, 0, 255, cv::NORM_MINMAX);
    /*
	for (size_t x = 0; x < img.rows; x++) {
		uchar *ptr = img.ptr(x);
		for (size_t y = 0; y < img.cols; y++) {
			// img[x][y] = (img[x][y] - min_index) * 255 / (max_index - min_index);
			*ptr = (*ptr - min_index) * 255 / (max_index - min_index);
		    ptr += 1;
        }
	}
    */
}

cv::Mat gray_world(cv::Mat &img)
{
    /* Idea is from here https://web.stanford.edu/~sujason/ColorBalancing/grayworld.html
     * The main premise behind this idea is that in a normal well color balanced
     * photo, the avarage of all the colors is a neutral gray.
     */

    assert (img.channels() == 3);
    int num_pixels = img.rows * img.cols;
    std::vector<cv::Mat> bgr_channels;
    cv::split(img, bgr_channels);
    double B = 0;
    double G = 0;
    double R = 0;
    cv::Mat result = img.clone();

    for (int x = 0; x < img.rows; x++) {
        for (int y = 0; y < img.cols; y++) {
            B += img.at<cv::Vec3b>(x, y)[0];
            G += img.at<cv::Vec3b>(x, y)[1];
            R += img.at<cv::Vec3b>(x, y)[2];
        }
    }

    B /= num_pixels;
    G /= num_pixels;
    R /= num_pixels;
    
    double gray_value = ( B + G + R ) / 3;
    double kb = gray_value / B;
    double kg = gray_value / G;
    double kr = gray_value / R;

    for (int x = 0; x < img.rows; x++) {
        for (int y = 0; y < img.cols; y++) {
            result.at<cv::Vec3b>(x, y)[0] = (uchar)(kb*img.at<cv::Vec3b>(x, y)[0]) > 255 
                    ? 255 : (kb*img.at<cv::Vec3b>(x, y)[0]);
            result.at<cv::Vec3b>(x, y)[1] = (uchar)(kg*img.at<cv::Vec3b>(x, y)[1]) > 255
                    ? 255 : (kg*img.at<cv::Vec3b>(x, y)[1]);
            result.at<cv::Vec3b>(x, y)[2] = (uchar)(kr*img.at<cv::Vec3b>(x, y)[2]) > 255
                    ? 255 : (kr*img.at<cv::Vec3b>(x, y)[2]);
        }
    }
    
    return result;
}


int main()
{
	std::string image_path = "/home/idealabs/Work/tmp/opencv_projects/resources/color_balance_lily.png";

	cv::Mat img = cv::imread(image_path, 1);
    cv::Mat result;
	cv::imshow("img", img);
    cv::waitKey(0);	
    std::vector<cv::Mat> bgr_channels;
	cv::split(img, bgr_channels);

	balance_color(bgr_channels[0]);
    balance_color(bgr_channels[1]);
    balance_color(bgr_channels[2]);

    cv::merge(bgr_channels, result);
    
    cv::imshow("balance_color", result);
    cv::waitKey(0);

    result = gray_world(img);
    cv::imshow("gray_world", result);
    cv::waitKey(0);
}
