//
//  FeatureAnalyzer.cpp
//  FlameDetection
//
//  Created by liberize on 14-4-14.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include "FeatureAnalyzer.h"
#include "VideoHandler.h"
#include "FlameDetector.h"
#include "TargetExtractor.h"

extern VideoHandler* videoHandler;

/**************** Feature ****************/

void Feature::calcColorFeature()
{
    // TODO: optimize this part, reduce extra work
    
    Mat hsv;
    cvtColor(mROI, hsv, CV_BGR2HSV_FULL);
    
    Mat temp(mROI.size(), CV_8UC3), mixed;
    Mat src[] = { mROI, mGray, hsv };
    int fromTo[] = { 2,0, 3,1, 5,2 };
    mixChannels(src, 3, &temp, 1, fromTo, 3);
    temp.convertTo(mixed, CV_64F);
    
    Scalar avg, stdDev;
    meanStdDev(mixed, avg, stdDev, mMask);
    Scalar var = stdDev.mul(stdDev);
    Mat temp1 = mixed - avg;
    Mat temp2 = temp1.mul(temp1);
    Scalar sk = mean(temp1.mul(temp2), mMask) / (var.mul(stdDev));
    Scalar ku = mean(temp2.mul(temp2), mMask) / (var.mul(var));
    
    Scalar stat[] = { avg, stdDev, sk, ku };
    for (int i = 0; i < 4; i++) {
        red[i] = stat[i][0];
        gray[i] = stat[i][1];
        saturation[i] = stat[i][2];
    }
}

void Feature::calcGeometryFeature(const Region& region)
{
    circularity = 0;
    squareness = 0;
    aspectRatio = 0;
    roughness = 0;
    
    const vector<ContourInfo*>& contours = region.contours;
    for (vector<ContourInfo*>::const_iterator it = contours.begin(); it != contours.end(); it++) {
        const vector<Point>& contour = (*it)->contour;
        double area = (*it)->area;
        
        double perimeter = arcLength(contour, true);
        RotatedRect minRect = minAreaRect(Mat(contour));
        vector<Point> hull;
        convexHull(contour, hull);
        double perimeterHull = arcLength(hull, true);
        double width = minRect.size.width, height = minRect.size.height;
        
        circularity += area * (4 * M_PI * area / (perimeter * perimeter));
        squareness  += area * (area / (width * height));
        aspectRatio += area * (1.0 * min(width, height) / max(width, height));
        roughness   += area * (perimeterHull / perimeter);
    }
    
    circularity /= mArea;
    squareness  /= mArea;
    aspectRatio /= mArea;
    roughness   /= mArea;
}

void Feature::calcTexture(int levels, int dx, int dy)
{
    assert(levels >= 2 && levels <= 256 && (levels & (levels - 1)) == 0);
    assert(dx >= 0 && dy >= 0 && dx + dy > 0);
    
    Mat temp;
    mGray.copyTo(temp);
    
    // TODO: implement my own version of 'equalizeHist' which accepts mask as an argument
    double minVal;
    minMaxLoc(temp, &minVal, NULL, NULL, NULL, mMask);
    uchar min = cvRound(minVal);
    for (int i = 0; i < temp.rows; i++) {
        for (int j = 0; j < temp.cols; j++) {
            if (mMask.at<uchar>(i, j) == 0) {
                temp.at<uchar>(i, j) = min;
            }
        }
    }
    equalizeHist(temp, temp);

#ifdef DEBUG_OUTPUT
    imshow("hist", temp);
#endif
    
    for (int i = 0; i < temp.rows; i++) {
        for (int j = 0; j < temp.cols; j++) {
            if (mMask.at<uchar>(i, j) == 255) {
                temp.at<uchar>(i, j) /= 256 / levels;
            }
        }
    }
    
    Mat glcm = Mat::zeros(Size(levels, levels), CV_64FC1);
    for (int i = 0; i < temp.rows; i++) {
        for (int j = 0; j < temp.cols; j++) {
            if (mMask.at<uchar>(i, j) == 255) {
                uchar l = temp.at<uchar>(i, j);
                int x1 = j + dx, y1 = i + dy;
                if (x1 < temp.cols && y1 < temp.rows && mMask.at<uchar>(y1, x1) == 255) {
                    uchar m = temp.at<uchar>(y1, x1);
                    glcm.at<double>(l, m) += 1;
                }
                int x2 = j - dx, y2 = i - dy;
                if (x2 >= 0 && y2 >= 0 && mMask.at<uchar>(y2, x2) == 255) {
                    uchar m = temp.at<uchar>(y2, x2);
                    glcm.at<double>(l, m) += 1;
                }
            }
        }
    }
    
    double sum = cv::sum(glcm)[0];
    if (sum == 0) {
        memset(texture, 0, sizeof(texture));
        return;
    }
    
    glcm *= 1.0 / sum;
    
    // in fact, the third one is not contrast...
    double entropy = 0, energy = 0, contrast = 0, homogenity = 0;
    for (int i = 0; i < levels; i++) {
        for (int j = 0; j < levels; j++) {
            double gij = glcm.at<double>(i, j);
            if(gij > 0) {
                entropy -= gij * log10(gij);
            }
            energy += gij * gij;
            contrast += (i - j) * (i - j) * gij;
            homogenity += 1.0 / (1 + (i - j) * (i - j)) * gij;
        }
    }
    
    texture[0] = entropy;
    texture[1] = energy;
    texture[2] = contrast;
    texture[3] = homogenity;
}

void Feature::calcFrequency()
{
    // TODO: optimize this part
    
    if (mAreaVec.size() < MAX_AREA_VEC_SIZE) {
        frequency = -1;
        return;
    }
    
    // limit n to integer power of 2 for simplicity
    // in fact, you can use function 'getOptimalDFTSize' to pad the input array
    assert((MAX_AREA_VEC_SIZE & (MAX_AREA_VEC_SIZE - 1)) == 0);
    
    vector<double> spec(MAX_AREA_VEC_SIZE);
    dft(mAreaVec, spec);
    
    double maxAmpl = 0;
    int idx = 0;
    for (int i = 1; i < MAX_AREA_VEC_SIZE; i += 2) {
        double ampl = (i == MAX_AREA_VEC_SIZE - 1) ? spec[i] :
            sqrt(spec[i] * spec[i] + spec[i + 1] * spec[i + 1]);
        if (ampl > maxAmpl) {
            maxAmpl = ampl;
            idx = (i + 1) / 2;
        }
    }
    
    double fps = videoHandler->getVideoFPS();
    frequency = fps / MAX_AREA_VEC_SIZE * idx;

#ifdef DEBUG_OUTPUT
    cout << "fps: " << fps << ", frequency: " << frequency << endl;
#endif
}

void Feature::calcAreaVar()
{
    // TODO: optimize this part
    
    if (mAreaVec.size() < MAX_AREA_VEC_SIZE) {
        areaVar = -1;
        return;
    }
    
    Scalar m, s;
    meanStdDev(mAreaVec, m, s);
    areaVar = s[0] / m[0];

#ifdef DEBUG_OUTPUT
    cout << "areaVar: " << areaVar << endl;
#endif
}

void Feature::calc(const Region& region, const Mat& frame)
{
    mROI = frame(region.rect);
    cvtColor(mROI, mGray, CV_BGR2GRAY);
    const Mat& mask = videoHandler->getDetector().getExtractor().getMask();
    mMask = mask(region.rect);
    mArea = 0;
    
    const vector<ContourInfo*>& contours = region.contours;
    for (vector<ContourInfo*>::const_iterator it = contours.begin(); it != contours.end(); it++) {
        mArea += (*it)->area;
    }
    
    calcColorFeature();
    calcGeometryFeature(region);
    calcTexture();
    
    if (mAreaVec.size() >= MAX_AREA_VEC_SIZE) {
        mAreaVec.erase(mAreaVec.begin());
    }
    mAreaVec.push_back(mArea);
    
    calcFrequency();
    calcAreaVar();
}

void Feature::merge(const vector<const Feature*>& src, Feature& feature)
{
    vector<double>::size_type maxAreaVecSize = 0;
    for (vector<const Feature*>::const_iterator it = src.begin(); it != src.end(); it++) {
        vector<double>::size_type areaVecSize = (*it)->mAreaVec.size();
        if (areaVecSize > maxAreaVecSize) {
            maxAreaVecSize = areaVecSize;
        }
    }
    
    vector<double>(maxAreaVecSize, 0).swap(feature.mAreaVec);
    
    for (vector<const Feature*>::const_iterator it1 = src.begin(); it1 != src.end(); it1++) {
        const vector<double>& areaVec = (*it1)->mAreaVec;
        vector<double>::reverse_iterator it2 = feature.mAreaVec.rbegin();

        for (vector<double>::const_reverse_iterator it4 = areaVec.rbegin(); it4 != areaVec.rend(); it4++) {
            *(it2++) += *it4;
        }
    }
}

Feature::operator Mat() const
{
    return (Mat_<float>(1, LEN) <<
            red[0], red[1], red[2], red[3],
            gray[0], gray[1], gray[2], gray[3],
            saturation[0], saturation[1], saturation[2], saturation[3],
            circularity, squareness, aspectRatio, roughness,
            texture[0], texture[1], texture[2], texture[3]);
}

ifstream& operator>>(ifstream& ifs, Feature& feature)
{
    ifs >> feature.red[0] >> feature.red[1]
        >> feature.red[2] >> feature.red[3]
        >> feature.gray[0] >> feature.gray[1]
        >> feature.gray[2] >> feature.gray[3]
        >> feature.saturation[0] >> feature.saturation[1]
        >> feature.saturation[2] >> feature.saturation[3]
        >> feature.circularity >> feature.squareness
        >> feature.aspectRatio >> feature.roughness
        >> feature.texture[0] >> feature.texture[1]
        >> feature.texture[2] >> feature.texture[3];
    return ifs;
}

ofstream& operator<<(ofstream& ofs, const Feature& feature)
{
    ofs << feature.red[0] << " " << feature.red[1] << " "
        << feature.red[2] << " " << feature.red[3] << " "
        << feature.gray[0] << " " << feature.gray[1] << " "
        << feature.gray[2] << " " << feature.gray[3] << " "
        << feature.saturation[0] << " " << feature.saturation[1] << " "
        << feature.saturation[2] << " " << feature.saturation[3] << " "
        << feature.circularity << " " << feature.squareness << " "
        << feature.aspectRatio << " " << feature.roughness << " "
        << feature.texture[0] << " " << feature.texture[1] << " "
        << feature.texture[2] << " " << feature.texture[3] << " ";
    return ofs;
}

#ifdef DEBUG_OUTPUT
void Feature::printAreaVec() const
{
    vector<double>::size_type size = mAreaVec.size();
    
    for (int i = 0; i < size; i++) {
        cout << mAreaVec[i];
        if (i != size - 1) {
            cout << ", ";
        } else {
            cout << endl;
        }
    }
}
#endif

/**************** FeatureAnalyzer ****************/

void FeatureAnalyzer::featureMerge(Target& target, const map<int, Target>& targets, const vector<int>& keys)
{
    vector<const Feature*> featureVec;
    for (vector<int>::const_iterator it = keys.begin(); it != keys.end(); it++) {
        // const map can't be accessed by operator '[]', so use function 'find' instead
        map<int, Target>::const_iterator iter = targets.find(*it);
        featureVec.push_back(&(iter->second.feature));
    }
    Feature::merge(featureVec, target.feature);
}

void FeatureAnalyzer::targetUpdate(map<int, Target>& targets)
{
    for (map<int, Target>::iterator it = targets.begin(); it != targets.end(); ) {
        Target& target = it->second;
        
        if (target.type == Target::TARGET_LOST) {
            int maxTimes = min(target.times * 2, 10);
            if (target.lostTimes >= maxTimes) {
                targets.erase(it++);
                continue;
            }
        } else {
            if (target.lostTimes != 0) {
                target.lostTimes = 0;
            }
            if (target.type == Target::TARGET_MERGED) {
                vector<int>& keys = target.mergeSrc;
                featureMerge(target, targets, keys);
                for (vector<int>::const_iterator it2 = keys.begin(); it2 != keys.end(); it2++) {
                    targets.erase(targets.find(*it2));
                }
                vector<int>().swap(keys);
            }
        }
        it++;
    }
    
    for (map<int, Target>::iterator it = targets.begin(); it != targets.end(); it++) {
        Target& target = it->second;
        if (target.type != Target::TARGET_LOST) {
            target.feature.calc(target.region, mFrame);
        }
    }
}

void FeatureAnalyzer::analyze(const Mat& frame, map<int, Target>& targets)
{
    mFrame = frame;
    
    targetUpdate(targets);
    
    Mat temp;
    mFrame.copyTo(temp);
    for (map<int, Target>::iterator it = targets.begin(); it != targets.end(); it++) {
        rectangle(temp, it->second.region.rect, Scalar(0, 255, 0));
    }
    namedWindow("frame");
    moveWindow("frame", 10, 400);
    imshow("frame", temp);
}
