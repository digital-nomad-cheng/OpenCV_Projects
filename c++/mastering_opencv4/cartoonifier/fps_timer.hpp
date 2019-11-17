#pragma once

#include <iostream>
#include <stdio.h>

class fps_timer {
    // frames/second timer for tracking 
public:
    int64 t_start;
    int64 t_end;
    float fps;
    int fnum;
    
    fps_timer() {
        this->reset();
    }

    void increment() {
        if(fnum >= 29) {
            t_end = cv::getTickCount();
            fps = 30.0 / (float(t_end - t_start)/cv::getTickFrequency());
            t_start = t_end;
            fnum = 0;    
        } else 
            fnum += 1;
    }

    void reset() {
        t_start = cv::getTickCount();
        fps = 0;
        fnum = 0;
    }

    void display_fps(cv::Mat& im, cv::Point2d p = cv::Point2f(-1, -1)) {
        char str[256];
        cv::Point2d pt;
        if(p.y < 0) 
            pt = cv::Point2d(10, im.rows-20);        
        else
            pt = p;
        sprintf(str, "%d frames/sec", (int)cv::cvRound(fps));
        string text = str;
        cv::putText(im, text, pt, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar::all(255));
    }
};
