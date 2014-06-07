//
//  TargetExtractor.cpp
//  FlameDetection
//
//  Created by liberize on 14-4-11.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include "TargetExtractor.h"
#include "FlameDetector.h"

/**************** Rectangle ****************/

Rectangle::Rectangle()
{
}

Rectangle::Rectangle(const Rect& r)
: Rect(r)
{
}

inline bool Rectangle::near(const Rectangle& r)
{
    return abs((x + width / 2.0) - (r.x + r.width / 2.0)) - (width + r.width) / 2.0 <
                max(width, r.width) * 0.2 &&
           abs((y + height / 2.0) - (r.y + r.height / 2.0)) - (height + r.height) / 2.0 <
                max(height, r.height) * 0.2;
}

inline void Rectangle::merge(const Rectangle& r)
{
    int tx = min(x, r.x);
    int ty = min(y, r.y);
    width = max(x + width, r.x + r.width) - tx;
    height = max(y + height, r.y + r.height) - ty;
    x = tx;
    y = ty;
}

/**************** Region ****************/

Region::Region()
{
}

Region::Region(ContourInfo* contour, const Rectangle& rect)
: contours(1, contour)
, rect(rect)
{
}

Region::Region(const vector<ContourInfo*>& contours, const Rectangle& rect)
: contours(contours)
, rect(rect)
{
}

inline bool Region::near(const Region& r)
{
    return rect.near(r.rect);
}

void Region::merge(const Region& r)
{
    rect.merge(r.rect);
    for (vector<ContourInfo*>::const_iterator it = r.contours.begin(); it != r.contours.end(); it++) {
        contours.push_back(*it);
    }
}

/**************** TargetExtractor ****************/

TargetExtractor::TargetExtractor()
{
    mMOG.set("detectShadows", false);
}

#ifdef OLD_ALGO
void TargetExtractor::movementDetect2(int threshold, double learningRate)
{
    Mat gray, temp, background;
    
    cvtColor(mFrame, gray, CV_BGR2GRAY);
    if (mBackground.empty()) {
        gray.convertTo(mBackground, CV_64F);
    }
    mBackground.convertTo(background, CV_8U);
    absdiff(background, gray, mMask);
    cv::threshold(mMask, mMask, threshold, 255, THRESH_BINARY);
    
    bitwise_not(mMask, temp);
    accumulateWeighted(gray, mBackground, learningRate, temp);
}

void TargetExtractor::colorDetect2(int threshold)
{
    Mat temp;
    GaussianBlur(mFrame, temp, Size(3, 3), 0);
    cvtColor(temp, temp, CV_BGR2YCrCb);
    
    Vec4d mean = sum(temp) / (temp.rows * temp.cols);
    for (int i = 0; i < temp.rows; i++) {
        for (int j = 0; j < temp.cols; j++) {
            if (mMask.at<uchar>(i, j) == 255) {
                Vec3b& v = temp.at<Vec3b>(i, j);
                if (!(v[0] > v[2] && v[1] > v[2] && v[0] > mean[0] && v[2] < mean[2] &&
                    v[1] > mean[1] && abs(v[2] - v[1]) >= threshold)) {
                    mMask.at<uchar>(i, j) = 0;
                }
            }
        }
    }
}

void TargetExtractor::regionGrow2(int areaThreshold, int diffThreshold)
{
    Mat gray;
    cvtColor(mFrame, gray, CV_BGR2GRAY);
    
    Mat temp;
    mMask.copyTo(temp);
    
    vector<vector<Point> > contours;
    findContours(temp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    
    int maxStackSize = gray.rows * gray.cols / 4;
    static int direction[8][2] = {
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 },
        { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }
    };
    
    for (int i = 0; i < contours.size(); i++) {
        if (contourArea(contours[i]) < areaThreshold) {
            drawContours(mMask, contours, i, Scalar(0), CV_FILLED);
            continue;
        }
        
        // TODO: 修改种子选取方法
        Moments mu = moments(contours[i], false);
        Point seed(cvRound(mu.m10 / mu.m00), cvRound(mu.m01 / mu.m00));
        if (pointPolygonTest(contours[i], seed, false) < 0) {
            cout << "Seed not in contour!" << endl;
            continue;
        }
        
        stack<Point> pointStack;
        temp.at<uchar>(seed) = 255;
        pointStack.push(seed);
        
        Mat temp = Mat::zeros(mMask.size(), mMask.type());
        uchar seedPixel = gray.at<uchar>(seed);
        Point cur, pop;
        
        while (!pointStack.empty() && pointStack.size() < maxStackSize) {
            
            pop = pointStack.top();
            pointStack.pop();
            
            for (int k = 0; k < 8; k++) {
                cur.x = pop.x + direction[k][0];
                cur.y = pop.y + direction[k][1];
                
                if (cur.x < 0 || cur.x > gray.cols - 1 || cur.y < 0 || cur.y > gray.rows - 1) {
                    continue;
                }
                
                if (temp.at<uchar>(cur) != 255) {
                    uchar curPixel = gray.at<uchar>(cur);
                    
                    if (abs(curPixel - seedPixel) < diffThreshold) {
                        temp.at<uchar>(cur) = 255;
                        pointStack.push(cur);
                    }
                }
            }
        }
        if (pointStack.empty()) {
            bitwise_or(mMask, temp, mMask);
        }
    }
}
#endif

void TargetExtractor::movementDetect(double learningRate)
{
    mMOG(mFrame, mMask, learningRate);
    mMOG.getBackgroundImage(mBackground);
}

void TargetExtractor::colorDetect(int redThreshold, double saturationThreshold)
{
    Mat temp;
    GaussianBlur(mFrame, temp, Size(3, 3), 0);
    
    for (int i = 0; i < temp.rows; i++) {
        for (int j = 0; j < temp.cols; j++) {
            if (mMask.at<uchar>(i, j) == 255) {
                Vec3b& v = temp.at<Vec3b>(i, j);
                double s = 1 - 3.0 * min(v[0], min(v[1], v[2])) / (v[0] + v[1] + v[2]);
                if (!(v[2] > redThreshold && v[2] >= v[1] && v[1] > v[0] &&
                    s >= ((255 - v[2]) * saturationThreshold / redThreshold))) {
                    mMask.at<uchar>(i, j) = 0;
                }
            }
        }
    }
}

void TargetExtractor::denoise(int ksize, int threshold)
{
    int r = (ksize - 1) / 2;
    if (r <= 0) {
        return;
    }
    
    Mat density;
    calcDensity(mMask, density, ksize);
    
    for (int i = r; i < mMask.rows - r; i++) {
        for (int j = r; j < mMask.cols - r; j++) {
            int count = density.at<int>(i, j);
            if (count < threshold) {
                mMask.at<uchar>(i, j) = 0;
            }
        }
    }
}

void TargetExtractor::fill(int ksize, int threshold)
{
    int r = (ksize - 1) / 2;
    if (r <= 0) {
        return;
    }
    
    Mat density;
    calcDensity(mMask, density, ksize);
    
    double half = ksize / 2.0, dist = ksize / 5.0;
    int max = ksize * ksize * 9 / 10;
    
    for (int i = r; i < mMask.rows - r; i++) {
        for (int j = r; j < mMask.cols - r; j++) {
            int count = density.at<int>(i, j);
            if (count > max) {
                mMask.at<uchar>(i, j) = 255;
            } else if (count >= threshold) {
                // TODO: further optimize the mass-center calculation
                Point center;
                Rect rect(j - r, i - r, ksize, ksize);
                getMassCenter(mMask(rect), center);
                if (abs(center.x - half) < dist && abs(center.y - half) < dist) {
                    mMask.at<uchar>(i, j) = 255;
                }
            }
        }
    }
}

void TargetExtractor::regionGrow(int threshold)
{
    Mat gray;
    cvtColor(mFrame, gray, CV_BGR2GRAY);
    
    Mat temp;
    mMask.copyTo(temp);
    
    vector<vector<Point> > contours;
    findContours(temp, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
    
    int maxQueueSize = mFrame.rows * mFrame.cols / 4;
    static int direction[8][2] = {
        { 0, 1 }, { 1, 1 }, { 1, 0 }, { 1, -1 },
        { 0, -1 }, { -1, -1 }, { -1, 0 }, { -1, 1 }
    };
    
    for (int i = 0; i < contours.size(); i++) {
        Rect rect = boundingRect(Mat(contours[i]));
        Mat mask = Mat::zeros(gray.size(), CV_8U);
        drawContours(mask, contours, i, Scalar::all(255), CV_FILLED);
        int size = sum(mask(rect))[0] / 255;
        Scalar m, s;
        meanStdDev(gray(rect), m, s, mask(rect));
        double mean = m[0], stdDev = s[0];
        
        Mat temp;
        mMask.copyTo(temp);
        int origSize = size;
        
        queue<Point> pointQueue;
        for (int j = 0; j < contours[i].size(); j++) {
            uchar pixel = gray.at<uchar>(contours[i][j]);
            if (abs(pixel - mean) < 1.0 * stdDev) {
                pointQueue.push(contours[i][j]);
            }
        }
        
        Point cur, pop;
        while (!pointQueue.empty() && pointQueue.size() < maxQueueSize) {
            
            pop = pointQueue.front();
            pointQueue.pop();
            uchar pixel = gray.at<uchar>(pop);
            
            for (int k = 0; k < 8; k++) {
                cur.x = pop.x + direction[k][0];
                cur.y = pop.y + direction[k][1];
                
                if (cur.x < 0 || cur.x > gray.cols - 1 || cur.y < 0 || cur.y > gray.rows - 1) {
                    continue;
                }
                
                if (temp.at<uchar>(cur) != 255) {
                    uchar curPixel = gray.at<uchar>(cur);
                    
                    if (abs(curPixel - pixel) < threshold &&
                        abs(curPixel - mean) < 1.0 * stdDev) {
                        
                        temp.at<uchar>(cur) = 255;
                        
                        double diff = curPixel - mean;
                        double learningRate = 1.0 / (++size);
                        mean = (1 - learningRate) * mean + learningRate * curPixel;
                        stdDev = sqrt((1 - learningRate) * stdDev * stdDev + learningRate * diff * diff);
                        
                        pointQueue.push(cur);
                    }
                }
            }
        }
        
        if (pointQueue.empty()) {
            int incSize = size - origSize;
            if (incSize < mFrame.rows * mFrame.cols / 6 && incSize / origSize < 5) {
                mMask = temp;
            }
        }
    }
}

void TargetExtractor::smallAreaFilter(int threshold, int keep)
{
    vector<vector<Point> > contours;
    // this will change mMask, but it doesn't matter
    findContours(mMask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
    
    vector<int> indexes;
    vector<double> areas;
    vector<Rect> boundRects;
    
    for (int i = 0; i < contours.size(); i++) {
        double area = contourArea(contours[i]);
        if (area < threshold) {
            continue;
        }
        
        Rect rect = boundingRect(Mat(contours[i]));
        if (rect.width < 0.01 * mMask.cols && rect.height < 0.01 * mMask.rows) {
            continue;
        }
        
        indexes.push_back(i);
        areas.push_back(area);
        boundRects.push_back(rect);
    }
    
    mMask = Mat::zeros(mMask.size(), mMask.type());
    vector<ContourInfo>().swap(mContours);
    
    if (areas.size() == 0) {
        return;
    }
    
    while (keep > 0) {
        vector<double>::iterator it = max_element(areas.begin(), areas.end());
        if (*it == 0) {
            break;
        }
        
        vector<double>::difference_type offset = it - areas.begin();
        int index = indexes[offset];
        drawContours(mMask, contours, index, Scalar::all(255), CV_FILLED);
        
        // use 'resize' and 'swap' to avoid copy of contours
        vector<ContourInfo>::size_type size = mContours.size();
        mContours.resize(size + 1);
        mContours[size].contour.swap(contours[index]);
        mContours[size].area = areas[offset];
        mContours[size].boundRect = boundRects[offset];
        
        *it = 0;
        keep--;
    }
}

void TargetExtractor::accumulate(int threshold)
{
    if (mMaskSum.empty()) {
        mMaskSum = Mat::zeros(mMask.size(), CV_8U);
    }
    
    for (int i = 0; i < mMask.rows; i++) {
        for (int j = 0; j < mMask.cols; j++) {
            if (mMask.at<uchar>(i, j) == 255) {
                mMaskSum.at<uchar>(i, j)++;
            }
        }
    }

    Mat temp;
    mMask.copyTo(temp);
    
    mMaskQueue.push(temp);
    if (mMaskQueue.size() > MAX_MASK_QUEUE_SIZE) {
        Mat pop = mMaskQueue.front();
        mMaskQueue.pop();
        for (int i = 0; i < mMask.rows; i++) {
            for (int j = 0; j < mMask.cols; j++) {
                if (pop.at<uchar>(i, j) == 255) {
                    assert(mMaskSum.at<uchar>(i, j) != 0);
                    mMaskSum.at<uchar>(i, j)--;
                }
            }
        }
    }
    
    if (mMaskQueue.size() == MAX_MASK_QUEUE_SIZE) {
        Mat result = Mat::zeros(mMask.size(), mMask.type());
        for (int i = 0; i < mMask.rows; i++) {
            for (int j = 0; j < mMask.cols; j++) {
                if (mMaskSum.at<uchar>(i, j) >= threshold) {
                    result.at<uchar>(i, j) = 255;
                }
            }
        }
        imshow("accumulated", result);
    }
}

void TargetExtractor::blobTrack(map<int, Target>& targets)
{
    list<Region> regions;
    for (vector<ContourInfo>::iterator it = mContours.begin(); it != mContours.end(); it++) {
        regions.push_back(Region(&(*it), it->boundRect));
    }
    
    list<Region>::size_type lastRegionsSize;
    do {
        lastRegionsSize = regions.size();
        for (list<Region>::iterator it1 = regions.begin(); it1 != regions.end(); it1++) {
            list<Region>::iterator it2 = it1;
            for (it2++; it2 != regions.end(); ) {
                if (it1->near(*it2)) {
                    it1->merge(*it2);
                    regions.erase(it2++);
                } else {
                    it2++;
                }
            }
        }
    } while (regions.size() != lastRegionsSize);
    
    srand((unsigned)clock());
    
    if (targets.empty()) {
        int id;
        for (list<Region>::iterator it = regions.begin(); it != regions.end(); it++) {
            while (id = rand(), targets.find(id) != targets.end());
            targets[id] = Target();
            targets[id].type = Target::TARGET_NEW;
            targets[id].region = *it;
            targets[id].times++;
        }
        return;
    }
    
    list<Rectangle> rects;
    map<int, Rectangle> targetRects;
    for (list<Region>::iterator it = regions.begin(); it != regions.end(); it++) {
        rects.push_back(it->rect);
    }
    for (map<int, Target>::iterator it = targets.begin(); it != targets.end(); it++) {
        rects.push_back(it->second.region.rect);
        targetRects[it->first] = it->second.region.rect;
    }
    
    list<Rectangle>::size_type lastRectsSize;
    do {
        lastRectsSize = rects.size();
        for (list<Rectangle>::iterator it1 = rects.begin(); it1 != rects.end(); it1++) {
            list<Rectangle>::iterator it2 = it1;
            for (it2++; it2 != rects.end(); ) {
                if (it1->near(*it2)) {
                    it1->merge(*it2);
                    rects.erase(it2++);
                } else {
                    it2++;
                }
            }
        }
    } while (rects.size() != lastRectsSize);
        
    for (list<Rectangle>::iterator it1 = rects.begin(); it1 != rects.end(); it1++) {
        vector<int> vi;
        vector<list<Region>::iterator> vlit;
        for (map<int, Rectangle>::iterator it2 = targetRects.begin(); it2 != targetRects.end(); it2++) {
            if (it1->contains(it2->second.tl())) {
                vi.push_back(it2->first);
            }
        }
        for (list<Region>::iterator it2 = regions.begin(); it2 != regions.end(); it2++) {
            if (it1->contains(it2->rect.tl())) {
                vlit.push_back(it2);
            }
        }
        int id;
        if (vlit.size() == 0) {
            assert(vi.size() == 1);
            id = vi[0];
            targets[id].type = Target::TARGET_LOST;
            targets[id].lostTimes++;
        } else if (vi.size() == 0) {
            assert(vlit.size() == 1);
            while (id = rand(), targets.find(id) != targets.end());
            targets[id] = Target();
            targets[id].type = Target::TARGET_NEW;
            targets[id].region = *(vlit[0]);
            targets[id].times++;
        } else {
            Region r(*(vlit[0]));
            vector<list<Region>::iterator>::iterator it3 = vlit.begin();
            for (it3++; it3 != vlit.end(); it3++) {
                r.merge(**it3);
            }
            if (vi.size() == 1) {
                id = vi[0];
                targets[id].type = Target::TARGET_EXISTING;
                targets[id].region = r;
                targets[id].times++;
            } else {
                while (id = rand(), targets.find(id) != targets.end());
                targets[id] = Target();
                targets[id].type = Target::TARGET_MERGED;
                targets[id].region = r;
                int times = 0;
                for (vector<int>::iterator it4 = vi.begin(); it4 != vi.end(); it4++) {
                    targets[id].mergeSrc.push_back(*it4);
                    if (targets[*it4].times > times) {
                        times = targets[*it4].times;
                    }
                }
                targets[id].times = times;
            }
        }
    }
}

void TargetExtractor::extract(const Mat& frame, map<int, Target>& targets, bool track)
{
    mFrame = frame;
    
    /* for 2.avi:
     *     movement:   0.008;
     *     color:      120, 0.2;
     *     regionGrow: enable;
     * for 6.avi:
     *     movement:   0.012;
     *     color:      150, 0.4;
     *     regionGrow: disable;
     */
    
    movementDetect(0.012);
    colorDetect(150, 0.4);
    
    denoise(7, 5);
    fill(7, 5);
    medianBlur(mMask, mMask, 3);
    
    // TODO: make use of accumulate result
    
    //regionGrow();
    //fill(7, 6);
    //medianBlur(mMask, mMask, 3);
    
    //Mat element = getStructuringElement(MORPH_CROSS, Size(3, 3));
    //erode(mMask, mMask, element);
    //dilate(mMask, mMask, element);
    
    smallAreaFilter(12, 8);
    
    namedWindow("mask");
    moveWindow("mask", 350, 120);
    imshow("mask", mMask);
    
    if (track) {
        blobTrack(targets);
    }
}
