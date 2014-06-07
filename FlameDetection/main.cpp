//
//  main.cpp
//  FlameDetection
//
//  Created by liberize on 14-4-4.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include "common.h"
#include "VideoHandler.h"
#include "FlameDetector.h"

#ifdef TRAIN_MODE
bool trainComplete = false;
#endif

VideoHandler* videoHandler = NULL;

int main(int argc, const char* argv[])
{
    VideoHandler handler("clips/6.avi");
    videoHandler = &handler;
    
    int ret = handler.handle();
    
    switch (ret) {
    case VideoHandler::STATUS_FLAME_DETECTED:
        cout << "Flame detected." << endl;
        break;
    case VideoHandler::STATUS_OPEN_CAP_FAILED:
        cout << "Open capture failed." << endl;
        break;
    case VideoHandler::STATUS_NO_FLAME_DETECTED:
        cout << "No flame detected." << endl;
        break;
    default:
        break;
    }

    return 0;
}
