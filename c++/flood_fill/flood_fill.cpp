


int main(int argc, char **argv)
{
    cv::Mat img = cv::imread("resources/gull.jpg", -1)
    cv::Mat mask = cv::Mat::zeros(cv::Size(img.cols+2, img.rows+2), CV_8UC1);
    cv::Point center;
    center.x = img.cols/2;
    center.y = img.rows/2;
    int flags = 4 | (255 << 8) | CV_FLOODFILL_MASK_ONLY;
    cv::Scalar low_diff = cv::Scalar(10, 10, 10);
    cv::Scalar up_diff = cv::Scalar(10, 10, 10);
    cv::floodFill(img, mask, center, cv::Scalar(255, 0, 0), 0, low_diff, up_diff,
        flags);
    cv::imshow("img", img);
    cv::imshow("mask", mask);

}

