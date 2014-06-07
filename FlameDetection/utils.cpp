//
//  utils.cpp
//  FlameDetection
//
//  Created by liberize on 14-5-19.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include "utils.h"

void getCurTime(string& curTime)
{
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    char buffer[128];
    strftime(buffer, 128, "%m-%d_%H-%M-%S", timeinfo);
    
    curTime = buffer;
}

// this function is an alternative to the old one which is implemented with 'sum'
void calcDensity(const Mat& mask, Mat& density, int ksize)
{
    int r = (ksize - 1) / 2;
    if (r <= 0) {
        return;
    }
    
    density = Mat::zeros(mask.size(), CV_32SC1);
    
    int rowBound = density.rows - r, colBound = density.cols - r;
    
    Rect rect(0, 0, ksize, ksize);
    density.at<int>(r, r) = cvRound(sum(mask(rect))[0] / 255);
    
    for (int j = r + 1; j < colBound; j++) {
        int col1 = j - r - 1, col2 = j + r;
        int delta = 0;
        for (int k = 0; k < ksize; k++) {
            delta += mask.at<uchar>(k, col2) - mask.at<uchar>(k, col1);
        }
        density.at<int>(r, j) = density.at<int>(r, j - 1) + delta / 255;
    }
    
    for (int i = r + 1; i < rowBound; i++) {
        int row1 = i - r - 1, row2 = i + r;
        int delta = 0;
        for (int k = 0; k < ksize; k++) {
            delta += mask.at<uchar>(row2, k) - mask.at<uchar>(row1, k);
        }
        density.at<int>(i, r) = density.at<int>(i - 1, r) + delta / 255;
    }
    
    for (int i = r + 1; i < rowBound; i++) {
        for (int j = r + 1; j < colBound; j++) {
            int delta = (mask.at<uchar>(i + r, j + r) - mask.at<uchar>(i - r - 1, j + r) -
                mask.at<uchar>(i + r, j - r - 1) + mask.at<uchar>(i - r - 1, j - r - 1)) / 255;
            density.at<int>(i, j) = density.at<int>(i - 1, j) + density.at<int>(i, j - 1) -
                density.at<int>(i - 1, j - 1) + delta;
        }
    }
}

// this function is also an alternative to the old one which is implemented with 'moments'
void getMassCenter(const Mat& mask, Point& center)
{
    int sumX = 0, sumY = 0, count = 0;
    for (int i = 0; i < mask.rows; i++) {
        for (int j = 0; j < mask.cols; j++) {
            if (mask.at<uchar>(i, j) == 255) {
                sumX += j;
                sumY += i;
                count++;
            }
        }
    }
    center.x = sumX / count;
    center.y = sumY / count;
}

void fixRect(const Mat& mask, Rect& rect)
{
    Moments mu = moments(mask(rect), true);
    double x = mu.m10 / mu.m00;
    double y = mu.m01 / mu.m00;
    double w = 4 * sqrt(mu.m20 / mu.m00 - x * x);
    double h = 4 * sqrt(mu.m02 / mu.m00 - y * y);
    rect.x     += cvRound(x - w / 2);
    rect.y     += cvRound(y - h / 2);
    rect.width  = min(cvRound(w), mask.cols - rect.x);
    rect.height = min(cvRound(h), mask.rows - rect.y);
}
