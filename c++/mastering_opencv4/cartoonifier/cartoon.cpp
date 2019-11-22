#include "cartoon.hpp"

void cartoonifyImage(cv::Mat srcColor, cv::Mat dst, bool sketchMode, bool alienMode, bool evilMode, int debugType) 
{
    cv::Mat srcGray;
    cv::cvtColor(srcColor, srcGray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(srcGray, srcGray, 7);
    cv::Size size = srcColor.size();
    cv::Mat mask = cv::Mat(size, CV_8U);
    cv::Mat edges = cv::Mat(size, CV_8U);
    if(!evilMode) {
        cv::Laplacian(srcGray, edges, CV_8U, 5);
        // cv::imshow("Laplacian", edges);
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

void changeFacialSkinColor(cv::Mat smallImgBGR, cv::Mat bigEdges, int debugType)
{
        cv::Mat yuv = cv::Mat(smallImgBGR.size(), CV_8UC3);
        cv::cvtColor(smallImgBGR, yuv, cv::COLOR_BGR2YCrCb);

        // The floodFill mask has to be 2 pixels wider and 2 pixels taller than the small image.
        // The edge mask is the full src image size, so we will shrink it to the small size,
        // storing into the floodFill mask data.
        int sw = smallImgBGR.cols;
        int sh = smallImgBGR.rows;
        cv::Mat maskPlusBorder = cv::Mat::zeros(sh+2, sw+2, CV_8U);
        cv::Mat mask = maskPlusBorder(cv::Rect(1,1,sw,sh));  // mask is a ROI in maskPlusBorder.
        cv::resize(bigEdges, mask, smallImgBGR.size());

        cv::threshold(mask, mask, 80, 255, cv::THRESH_BINARY);
        cv::dilate(mask, mask, cv::Mat());
        cv::erode(mask, mask, cv::Mat());
        // cv::imshow("constraints for floodFill", mask);

        // YCrCb Skin detector and color changer using multiple flood fills into a mask.
        // Apply flood fill on many points around the face, to cover different shades & colors of the face.
        // Note that these values are dependent on the face outline, drawn in drawFaceStickFigure().
        int const NUM_SKIN_POINTS = 6;
        cv::Point skinPts[NUM_SKIN_POINTS];
        skinPts[0] = cv::Point(sw/2,          sh/2 - sh/6);
        skinPts[1] = cv::Point(sw/2 - sw/11,  sh/2 - sh/6);
        skinPts[2] = cv::Point(sw/2 + sw/11,  sh/2 - sh/6);
        skinPts[3] = cv::Point(sw/2,          sh/2 + sh/16);
        skinPts[4] = cv::Point(sw/2 - sw/9,   sh/2 + sh/16);
        skinPts[5] = cv::Point(sw/2 + sw/9,   sh/2 + sh/16);
        // Skin might be fairly dark, or slightly less colorful.
        // Skin might be very bright, or slightly more colorful but not much more blue.
        const int LOWER_Y = 60;
        const int UPPER_Y = 80;
        const int LOWER_Cr = 25;
        const int UPPER_Cr = 15;
        const int LOWER_Cb = 20;
        const int UPPER_Cb = 15;
        cv::Scalar lowerDiff = cv::Scalar(LOWER_Y, LOWER_Cr, LOWER_Cb);
        cv::Scalar upperDiff = cv::Scalar(UPPER_Y, UPPER_Cr, UPPER_Cb);
        // Instead of drawing into the "yuv" image, just draw 1's into the "maskPlusBorder" image, so we can apply it later.
        // The "maskPlusBorder" is initialized with the edges, because floodFill() will not go across non-zero mask pixels.
        cv::Mat edgeMask = mask.clone();    // Keep an duplicate copy of the edge mask.
        for (auto i=0; i<NUM_SKIN_POINTS; i++) {
            const int flags = 4 | cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY;
            cv::floodFill(yuv, maskPlusBorder, skinPts[i], cv::Scalar(), NULL, lowerDiff, upperDiff, flags);
            if (debugType >= 1)
                cv::circle(smallImgBGR, skinPts[i], 5, CV_RGB(0, 0, 255), 1, cv::LINE_AA);
        }

        if (debugType >= 1)
            cv::imshow("flood mask", mask*120); 

        // After the flood fill, "mask" contains both edges and skin pixels, whereas
        // "edgeMask" just contains edges. So to get just the skin pixels, we can remove the edges from it.
        mask -= edgeMask;
        // "mask" now just contains 1's in the skin pixels and 0's for non-skin pixels.

        auto Red = 0;
        auto Green = 70;
        auto Blue = 0;
        cv::add(smallImgBGR, cv::Scalar(Blue, Green, Red), smallImgBGR, mask);
}


void removePepperNoise(cv::Mat &mask) 
{
    for (auto y = 2; y < mask.rows-2; y++) {
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

        for(auto x = 2; x < mask.rows-2; x++) {
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

void drawFaceStickFigure(cv::Mat dst) 
{
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
