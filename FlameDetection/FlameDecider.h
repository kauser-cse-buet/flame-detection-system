//
//  FlameDecider.h
//  FlameDetection
//
//  Created by liberize on 14-5-20.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef __FlameDetection__FlameDecider__
#define __FlameDetection__FlameDecider__

#include "common.h"

struct Target;
class Feature;


class FlameDecider {
private:
    static const string SVM_DATA_FILE;
    
#ifdef TRAIN_MODE
    static const string SAMPLE_FILE;
    static const int MIN_SAMPLE_COUNT = 1000;
    static const int FRAME_GAP = 5;
#endif
    
    Mat mFrame;
    CvSVM mSVM;
    
#ifdef TRAIN_MODE
    bool mSampleEnough;
    int mFlameCount;
    int mNonFlameCount;
    vector<Feature> mFeatureVec;
    vector<bool> mResultVec;
    int mFrameCount;
#endif
    
#ifdef TRAIN_MODE
    void userInput(const map<int, Target>& targets);
    void svmStudy();
    void train(const map<int, Target>& targets);
#else
    bool svmPredict(const Feature& feature);
    bool judge(map<int, Target>& targets);
#endif
    
public:
    FlameDecider();
    bool decide(const Mat& frame, map<int, Target>& targets);
};

#endif /* defined(__FlameDetection__FlameDecider__) */
