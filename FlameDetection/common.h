//
//  common.h
//  FlameDetection
//
//  Created by liberize on 14-4-5.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef FlameDetection_common_h
#define FlameDetection_common_h

#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <stack>
#include <queue>
#include <vector>
#include <list>
#include <map>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/ml/ml.hpp>

using namespace std;
using namespace cv;

//#define OLD_ALGO
#define TRAIN_MODE
//#define DEBUG_OUTPUT

#ifdef TRAIN_MODE
extern bool trainComplete;
#endif

#endif
