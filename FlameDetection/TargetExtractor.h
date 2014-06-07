//
//  TargetExtractor.h
//  FlameDetection
//
//  Created by liberize on 14-4-11.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef __FlameDetection__TargetExtractor__
#define __FlameDetection__TargetExtractor__

#include "common.h"
#include "utils.h"

struct Target;


struct ContourInfo {
    vector<Point> contour;
    double area;
    Rect boundRect;
};


class Rectangle : public Rect {
public:
    Rectangle();
    Rectangle(const Rect& r);
    
    bool near(const Rectangle& r);
    void merge(const Rectangle& r);
};


class Region {
public:
    vector<ContourInfo*> contours;
    Rectangle rect;
    
    Region();
    Region(ContourInfo* contour, const Rectangle& rect);
    Region(const vector<ContourInfo*>& contours, const Rectangle& rect);
    bool near(const Region& r);
    void merge(const Region& r);
};


class TargetExtractor {
private:
    static const int MAX_MASK_QUEUE_SIZE = 10;

    Mat mFrame;
    Mat mMask;
    queue<Mat> mMaskQueue;
    Mat mMaskSum;
    vector<ContourInfo> mContours;
    
    Mat mBackground;
    BackgroundSubtractorMOG2 mMOG;

#ifdef OLD_ALGO
    void movementDetect2(int threshold = 30, double learningRate = 0.01);
    void colorDetect2(int threshold = 20);
    void regionGrow2(int areaThreshold, int diffThreshold);
#endif
    
    void movementDetect(double learningRate = -1);
    void colorDetect(int redThreshold = 150, double saturationThreshold = 0.4);
    void denoise(int ksize = 7, int threshold = 6);
    void fill(int ksize = 7, int threshold = 6);
    void regionGrow(int threshold = 20);
    void smallAreaFilter(int threshold = 10, int keep = 5);
    void accumulate(int threshold = 5);
    void blobTrack(map<int, Target>& targets);
    
public:
    TargetExtractor();
    const Mat& getMask() const { return mMask; }
    void extract(const Mat& frame, map<int, Target>& targets, bool track);
};

#endif /* defined(__FlameDetection__TargetExtractor__) */
