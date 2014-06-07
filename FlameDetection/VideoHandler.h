//
//  VideoHandler.h
//  FlameDetection
//
//  Created by liberize on 14-4-6.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#ifndef __FlameDetection__VideoHandler__
#define __FlameDetection__VideoHandler__

#include "common.h"
#include "utils.h"
#include "FlameDetector.h"


class VideoHandler {
private:
    static const int WAIT_INTERVAL = 30;
    static const int MAX_EXTRA_FRAME_COUNT = 80;

    VideoCapture mCapture;
    FlameDetector mDetector;
    Mat mFrame;
    bool mFromCam;
    bool mSaveVideo;
    bool mSaveKeyFrame;
    double mVideoFPS;
    string mSaveVideoFile;
    VideoWriter mWriter;
    
    bool saveFrame();
    bool saveVideo();
    
public:
    static const int STATUS_FLAME_DETECTED = 0;
    static const int STATUS_OPEN_CAP_FAILED = 1;
    static const int STATUS_NO_FLAME_DETECTED = 2;

    VideoHandler(int device, bool saveKeyFrame = false, bool saveVideo = false);
    VideoHandler(const string& file, bool saveKeyFrame = false);
    
    const FlameDetector& getDetector() const { return mDetector; }
    double getVideoFPS() const { return mVideoFPS; }
    int handle();
};

#endif /* defined(__FlameDetection__VideoHandler__) */
