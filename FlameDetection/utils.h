//
//  utils.h
//  FlameDetection
//
//  Created by liberize on 14-5-19.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef __FlameDetection__utils__
#define __FlameDetection__utils__

#include "common.h"
#include <ctime>

void getCurTime(string& curTime);

void calcDensity(const Mat& mask, Mat& density, int ksize = 7);
void getMassCenter(const Mat& mask, Point& center);
void fixRect(const Mat& mask, Rect& rect);

#endif /* defined(__FlameDetection__utils__) */
