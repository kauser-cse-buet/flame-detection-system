//
//  FlameDecider.cpp
//  FlameDetection
//
//  Created by liberize on 14-5-20.
//  Copyright (c) 2014年 liberize. All rights reserved.
//

#include "FlameDecider.h"
#include "FlameDetector.h"
#include "FeatureAnalyzer.h"

const string FlameDecider::SVM_DATA_FILE("svmdata.xml");

#ifdef TRAIN_MODE
const string FlameDecider::SAMPLE_FILE("sample.txt");
#endif

#ifdef TRAIN_MODE
FlameDecider::FlameDecider()
: mSampleEnough(false)
, mFlameCount(0)
, mNonFlameCount(0)
, mFrameCount(0)
{
    Feature feature;
    bool isFlame;
    
    ifstream ifs(SAMPLE_FILE);
    while (ifs >> feature >> isFlame) {
        mFeatureVec.push_back(feature);
        mResultVec.push_back(isFlame);
        if (isFlame) {
            mFlameCount++;
        } else {
            mNonFlameCount++;
        }
    }
    ifs.close();
    
    if (mFlameCount >= MIN_SAMPLE_COUNT && mNonFlameCount >= MIN_SAMPLE_COUNT) {
        mSampleEnough = true;
        cout << "Flame count: " << mFlameCount << ", non-flame count: " << mNonFlameCount << "." << endl;
    }
}
#else
FlameDecider::FlameDecider()
{
    mSVM.load(SVM_DATA_FILE.c_str());
}
#endif

#ifdef TRAIN_MODE
void FlameDecider::userInput(const map<int, Target>& targets)
{
    ofstream ofs(SAMPLE_FILE, ios::app);
    for (map<int, Target>::const_iterator it = targets.begin(); it != targets.end(); it++) {
        if (it->second.lostTimes > 0) {
            continue;
        }
        
        const Feature& feature = it->second.feature;
        const Rectangle& rect = it->second.region.rect;
        
        Mat temp;
        mFrame.copyTo(temp);
        bool flag = true;
        
        while (true) {
            int key = waitKey(200);
            switch (key) {
                case -1:    // no key pressed
                    rectangle(temp, rect, flag ? Scalar(0, 0, 255) : Scalar(0, 255, 0));
                    namedWindow("temp");
                    moveWindow("temp", 350, 400);
                    imshow("temp", temp);
                    flag = !flag;
                    break;
                case 'y':   // press 'y' to add a positive record to sample
                    ofs << feature << true << endl;
#ifdef DEBUG_OUTPUT
                    cout << "freq: " << feature.frequency << endl;
                    feature.printAreaVec();
#endif
                    mFeatureVec.push_back(feature);
                    mResultVec.push_back(true);
                    mFlameCount++;
                    goto next;
                case 'n':   // press 'n' to add a negative record to sample
                    ofs << feature << false << endl;
                    mFeatureVec.push_back(feature);
                    mResultVec.push_back(false);
                    mNonFlameCount++;
                    goto next;
                case ' ':   // press SPACE to skip current target
                    goto next;
                case 's':   // press 's' to skip current frame
                    goto end;
                case 27:    // press ESC to stop training and exit program
                    trainComplete = true;
                    goto end;
                case 'o':   // press 'o' to stop input and start studying
                    mSampleEnough = true;
                    goto end;
                default:
                    break;
            }
        }

    next:
        if (mFlameCount >= MIN_SAMPLE_COUNT && mNonFlameCount >= MIN_SAMPLE_COUNT) {
            mSampleEnough = true;
            goto end;
        }
    }
    
end:
    ofs.close();
    cout << "Flame count: " << mFlameCount << ", non-flame count: " << mNonFlameCount << "." << endl;
}

void FlameDecider::svmStudy()
{
    assert(mFeatureVec.size() == mResultVec.size());
    
    int size = int(mFeatureVec.size());
	Mat data(size, Feature::LEN, CV_32FC1);
	Mat label(size, 1, CV_32FC1);
    
	for (int i = 0; i < size; i++) {
		Mat(mFeatureVec[i]).copyTo(data.row(i));
        label.at<float>(i, 0) = mResultVec[i] ? 1.0 : 0.0;
	}
    
	CvSVMParams params;
    params.svm_type = CvSVM::C_SVC;
    params.kernel_type = CvSVM::LINEAR;
    params.term_crit = cvTermCriteria(CV_TERMCRIT_ITER, 100, 1e-6);
    
    mSVM.train(data, label, Mat(), Mat(), params);
	mSVM.save(SVM_DATA_FILE.c_str());
}

void FlameDecider::train(const map<int, Target>& targets)
{
    if (!mSampleEnough) {
        if (mFrameCount++ % FRAME_GAP == 0) {
            userInput(targets);
        }
    } else {
        svmStudy();
        trainComplete = true;
    }
}
#else
inline bool FlameDecider::svmPredict(const Feature& feature)
{
    float result = mSVM.predict(Mat(feature));
    cout << "result: " << result << endl;
	return result == 1.0;
}

bool FlameDecider::judge(map<int, Target>& targets)
{
    bool flameDetected = false;
    
    Mat temp;
    mFrame.copyTo(temp);
    
    for (map<int, Target>::iterator it = targets.begin(); it != targets.end(); it++) {
        bool isFlame = svmPredict(it->second.feature);
        it->second.isFlame = isFlame;
        if (isFlame) {
            flameDetected = true;
            rectangle(temp, it->second.region.rect, Scalar(0, 255, 0));
        }
    }
    
    namedWindow("result");
    moveWindow("result", 350, 400);
    imshow("result", temp);
    return flameDetected;
}
#endif

bool FlameDecider::decide(const Mat& frame, map<int, Target>& targets)
{
    mFrame = frame;
    
#ifdef TRAIN_MODE
    train(targets);
    return false;
#else
    return judge(targets);
#endif
}
