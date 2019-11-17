#pragma once

#include <stdio.h>
#include <iostream>
#include <vector>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

void cartoonifyImage(cv::Mat srcColor, cv::Mat dst, bool sketchMode, bool alienMode, bool evilMode, int debugType);
void drawFaceStickFigure(cv::Mat dst);
void changeFacialSkinColor(cv::Mat smallImgBGR, cv::Mat bigEdges, int debugType);
void removePepperNoise(cv::Mat &mask);

