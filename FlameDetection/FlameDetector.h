//
//  FlameDetector.h
//  FlameDetection
//
//  Created by liberize on 14-4-5.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef __FlameDetection__FlameDetector__
#define __FlameDetection__FlameDetector__

#include "common.h"
#include "TargetExtractor.h"
#include "FeatureAnalyzer.h"
#include "FlameDecider.h"


struct Target {
    static const int TARGET_EXISTING = 0;
    static const int TARGET_NEW = 1;
    static const int TARGET_LOST = 2;
    static const int TARGET_MERGED = 3;
    
    int type;
    int times;
    int lostTimes;
    vector<int> mergeSrc;
    Region region;
    Feature feature;
    bool isFlame;
};


class FlameDetector {
private:
    static const int SKIP_FRAME_COUNT = 20;
    
    Mat mFrame;
    TargetExtractor mExtractor;
    FeatureAnalyzer mAnalyzer;
    FlameDecider mDecider;
    map<int, Target> mTargetMap;
    int mFrameCount;
    int mFlameCount;
    bool mTrack;
    
public:
    FlameDetector();
    
    const TargetExtractor& getExtractor() const { return mExtractor; }
    const FeatureAnalyzer& getAnalyzer() const { return mAnalyzer; }
    const FlameDecider& getDecider() const { return mDecider; }
    bool detect(const Mat& frame);
};

#endif /* defined(__FlameDetection__FlameDetector__) */
