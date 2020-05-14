#include <iostream>
#include <opencv2/opencv.hpp>

void balance_color(cv::Mat &img)
{
	// calculate image histogram
	int num_pixels = img.cols * img.rows;
	int channels = 0;
	int hist_size = 256;
	float pranges[2] = {0,255};
    const float* ranges[1] = {pranges};
	cv::Mat hist;
	cv::calcHist(&img, 1, &channels, cv::Mat(), hist, 1, &hist_size, ranges);
	// hist: rows: 256, cols: 1
	
	// calculate cumulative distribute function
	for (size_t i = 1; i < hist.rows; i++) {
		hist.ptr(i)[0] += hist.ptr(i-1)[0];
	}

	double s = 0.0265; //此参数可以调整，最好在0.1以下(0=<s<=1)

	int min_index = 0;

	while (hist.ptr(min_index)[0] < num_pixels*s / 2) {
		min_index += 1;
	}

	int max_index = 255;

	while (hist.ptr(max_index)[0] > num_pixels * (1 - s/2)) {
		max_index -= 1;
	}

	// min_index and max_index represents the over dark and over light pixels in img
	for (size_t x = 0; x < img.rows; x++) {
		uchar *ptr = img.ptr(x);
		for (size_t y = 0; y < img.cols; y++) {
			if (ptr[0] > max_index)
				ptr[0] = max_index;
			if (ptr[0] < min_index)
				ptr[0] = min_index;
		}
	}

	// equalization
	for (size_t x = 0; x < img.rows; x++) {
		uchar *ptr = img.ptr(x);
		for (size_t y = 0; y < img.cols; y++) {
			// img[x][y] = (img[x][y] - min_index) * 255 / (max_index - min_index);
			ptr[0] = (ptr[0] - min_index) * 255 / (max_index - min_index);
		}
	}
}

int main()
{
	std::string image_path = "/Users/vincent/Documents/Repo/opencv_projects/resources/source.jpg";

	cv::Mat img = cv::imread(image_path, 1);
	cv::imshow("img", img);
	cv::waitKey(0);
	std::vector<cv::Mat> bgr_channels;
	cv::split(img, bgr_channels);

	balance_color(bgr_channels[0]);
}