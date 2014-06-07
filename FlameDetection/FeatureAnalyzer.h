//
//  FeatureAnalyzer.h
//  FlameDetection
//
//  Created by liberize on 14-4-14.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef __FlameDetection__FeatureAnalyzer__
#define __FlameDetection__FeatureAnalyzer__

#include "common.h"

struct Target;
class Region;


class Feature {
private:
    Mat mROI;
    Mat mGray;
    Mat mMask;
    double mArea;
    
    static const int MAX_AREA_VEC_SIZE = 32;
    
    // if we use list here, we need to convert it to vector each time. so use vector instead.
    vector<double> mAreaVec;
    
    void calcColorFeature();
    void calcGeometryFeature(const Region& region);
    void calcTexture(int levels = 16, int dx = 3, int dy = 3);
    void calcFrequency();
    void calcAreaVar();
    
public:
    static const int LEN = 20;
    
    // color features
    double red[4];
    double gray[4];
    double saturation[4];
    
    // geometric features
    double circularity;
    double squareness;
    double aspectRatio;
    double roughness;
    
    // structural features
    double texture[4];
    
    // dynamic features
    double frequency;
    double areaVar;
    
    void calc(const Region& region, const Mat& frame);
    static void merge(const vector<const Feature*>& src, Feature& feature);
    operator Mat() const;

#ifdef DEBUG_OUTPUT
    void printAreaVec() const;
#endif
};

ifstream& operator>>(ifstream& ifs, Feature& feature);
ofstream& operator<<(ofstream& ofs, const Feature& feature);


class FeatureAnalyzer {
private:
    Mat mFrame;
    
    void featureUpdate(Target& target);
    void featureMerge(Target& target, const map<int, Target>& targets, const vector<int>& keys);
    void targetUpdate(map<int, Target>& targets);

public:
    void analyze(const Mat& frame, map<int, Target>& targets);
};

#endif /* defined(__FlameDetection__FeatureAnalyzer__) */
