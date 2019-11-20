#include "cartoon.hpp"

void cartoonifyImage(cv::Mat srcColor, cv::Mat dst, bool sketchMode, bool alienMode, bool evilMode, int debugType) {
    cv::Mat srcGray;
    cv::cvtColor(srcColor, srcGray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(srcGray, srcGray, 7);
    cv::Size size = srcColor.size();
    cv::Mat mask = cv::Mat(size, CV_8U);
    cv::Mat edges = cv::Mat(size, CV_8U);
    if(!evilMode) {
        cv::Laplacian(srcGray, edges, CV_8U, 5);
        cv::threshold(edges, mask, 80, 255, cv::THRESH_BINARY_INV);
        removePepperNoise(mask);
    } else {
        cv::Mat edges2;
        cv::Scharr(srcGray, edges, CV_8U, 1, 0);
        cv::Scharr(srcGray, edges2, CV_8U, 1, 0, -1);
        edges += edges2;
        cv::threshold(edges, mask, 12, 255, cv::THRESH_BINARY_INV);
        cv::medianBlur(mask, mask, 3);
    }

    if(sketchMode) {
        cv::cvtColor(mask, dst, cv::COLOR_GRAY2BGR);
        return;
    }

    cv::Size smallSize;
    smallSize.width = size.width / 2;
    smallSize.height = size.height / 2;
    cv::Mat smallImg = cv::Mat(smallSize, CV_8UC3);
    cv::resize(srcColor, smallImg, smallSize, 0, 0, cv::INTER_LINEAR);
    
    cv::Mat tmp = cv::Mat(smallSize, CV_8UC3);
    int repetitions = 7;
    for(int i = 0; i < repetitions; i++) {
        int filterSize = 9;
        double sigmaColor = 9;
        double sigmaSpace = 7;
        cv::bilateralFilter(smallImg, tmp, filterSize, sigmaColor, sigmaSpace);
        cv::bilateralFilter(tmp, smallImg, filterSize, sigmaColor, sigmaSpace);
    }
    if(alienMode) {
        changeFacialSkinColor(smallImg, edges, debugType);
    }
    cv::resize(smallImg, srcColor, size, 0, 0, cv::INTER_LINEAR);
    memset((char*)dst.data, 0, dst.step*dst.rows); // in c or c++
    srcColor.copyTo(dst, mask);
}

void removePepperNoise(cv::Mat &mask) {
    for(auto y=2; y<mask.rows-2; y++) {
        uchar *pThis = mask.ptr(y);
        uchar *pUp1 = mask.ptr(y-1);
        uchar *pUp2 = mask.ptr(y-2);
        uchar *pDown1 = mask.ptr(y+1);
        uchar *pDown2 = mask.ptr(y+2);

        pThis += 2;
        pUp1 += 2;
        pUp2 += 2;
        pDown1 += 2;
        pDown2 += 2;

        for(auto x=2; x<mask.rows-2; x++) {
            uchar v = *pThis;
            if(v == 0) {
                bool allAbove = *(pUp2-2) && *(pUp2-1) && *(pUp2) && *(pUp2+1) && *(pUp2+2);
                bool allLeft = *(pUp1 - 2) && *(pThis - 2) && *(pDown1 - 2);
                bool allBelow = *(pDown2 - 2) && *(pDown2 - 1) && *(pDown2) && *(pDown2 + 1) && *(pDown2 + 2);
                bool allRight = *(pUp1 + 2) && *(pThis + 2) && *(pDown1 + 2);
                bool surroundings = allAbove && allLeft && allBelow && allRight;
                if(surroundings == true) {
                    *(pUp1 - 1) = 255;
                    *(pUp1 + 0) = 255;
                    *(pUp1 + 1) = 255;
                    *(pThis - 1) = 255;
                    *(pThis + 0) = 255;
                    *(pThis + 1) = 255;
                    *(pDown1 - 1) = 255;
                    *(pDown1 + 0) = 255;
                    *(pDown1 + 1) = 255;
                }
                pThis += 2;
                pUp1 += 2;
                pUp2 += 2;
                pDown1 += 2;
                pDown2 += 2;
            }
            pThis++;
            pUp1++;
            pUp2++;
            pDown1++;
            pDown2++;
        }
                
    }     
}

void drawFaceStickFigure(cv::Mat dst) {
    cv::Size size = dst.size();
    int sw = size.width;
    int sh = size.height;

    cv::Mat faceOutline = cv::Mat::zeros(size, CV_8UC3);
    cv::Scalar color = CV_RGB(255, 255, 0);
    auto thickness = 4;
    int faceH = sh/2 * 70/100;
    int faceW = faceH * 72/100;
    cv::ellipse(faceOutline, cv::Point(sw/2, sh/2), cv::Size(faceW, faceH), 0, 0, 360, color, thickness, cv::LINE_AA);
    int eyeW = faceW * 23/100;
    int eyeH = faceH * 11/100;
    int eyeX = faceH * 48/100;
    int eyeY = faceH * 13/100;
    auto eyeA = 15;
    auto eyeYshift = 11;
    
    // draw top, bottom of right eye, draw top bottom of left eye
    cv::ellipse(faceOutline, cv::Point(sw/2 - eyeX, sh/2 - eyeY), cv::Size(eyeW, eyeH), 0, 
        180+eyeA, 360-eyeA, color, thickness, cv::LINE_AA);
    cv::ellipse(faceOutline, cv::Point(sw/2 - eyeX, sh/2 - eyeY - eyeYshift), cv::Size(eyeW, eyeH), 0, 0+eyeA, 180-eyeA, color, thickness, cv::LINE_AA);
        
    int mouthY = faceH * 53/100;
    int mouthW = faceW * 45/100;
    int mouthH = faceH * 6/100;
    cv::ellipse(faceOutline, cv::Point(sw/2, sh/2+mouthY), cv::Size(mouthW, mouthH), 0, 0, 180, color, thickness, cv::LINE_AA);
    
    auto fontFace = cv::FONT_HERSHEY_COMPLEX;
    auto fontScale = 1.0f;
    auto fontThickness = 2;
    cv::putText(faceOutline, "Put your face here", cv::Point(sw * 23/100, sh * 10/100), fontFace, fontScale, color, fontThickness, cv::LINE_AA);
    
    addWeighted(dst, 1.0, faceOutline, 0.7, 0, dst, CV_8UC3);
}
