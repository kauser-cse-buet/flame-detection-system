# Flame Detection System

This project is an imcomplete implemention of a video-based multi-feature flame detection system, and it's my graduation design.

## Some Words to Say

When I began the study of this subject, I found a lot of articles and very little code on the Internet. So when I finally finished my graduation design, I decided to make it open source.

Note that, this project is not ready for practical application. Due to limited time spent on this project, the code is only partially optimized. However, the response time is totally acceptable when tesing on my Mac.

## Build Environment

This project is built with Xcode 5.0.2 on Mac OS X 10.9.3, and it's implemented with OpenCV 2.4.8.

Because it doesn't use any native interface of a specific platform, it can be easily ported to other platforms, such as windows, linux, android, etc.

If you want to use the save-video feature, you need to make sure your OpenCV is compiled with FFMPEG support, if not, compile it by yourself.

## The Result

The following two images show the result.

![Demo1][Demo1]
![Demo2][Demo2]

## The Process

First, segment the input image with color detection and motion detection, and extract candidate flame regions. 

1. Use Gaussian mixture model for background modeling to detect motion pixels.
2. Use a flame color model based on RGB and HSV color space to detect pixels which have flame-like color.
3. Take the intersection of the results of the above two steps to get candidate regions.
4. For the problem of noise and incomplete target, apply serveral denoising and target completion algorithms to the candidate regions.
5. Divide the candidate region into targets, and use multi-target tracking algorithm to identify the same target in different frames.

Then, extract some static and dynamic features from the flame.

1. Static features are described in three aspects: color, geometry and texture.
2. Dynamic features are described in two aspects: whole motion and random motion.

Finally, use a support vector machine for flame recognition, which consists of two parts: training and prediction.

## Files and Folders

* The `FlameDetection` folder contains all the C++ code.
* The `clips` folder contains some video clips for testing, which is mainly downloaded from [here][Video Source].
* The `samples` folder contains some data samples which can be used as SVM input. 
* The `test` folder contains only one script which plot the DFT results with matplotlib.

## Performance Testing

Tested on MacBook Pro MC721CH/A, with processor Intel Core i7 2630QM, clocked at 2GHz, Turbo 2.9GHz and 8GB memory size.

### Response Time

Use `clips/6.avi` to test the response time.

| Total Frames | Min Process Time | Max Process Time | Average Process Time |
| ------------ | ---------------- | ---------------- | -------------------- |
|    1181      |    0.016595      |    0.032096      |    0.019690          |

### Detection Rate

Use three clips to test the detection rate.

| Video Files | Total Frames | Detected Frames | Detection Rate |
| ----------- | ------------ | --------------- | -------------- |
|   6.avi     |     1181     |      1073       |    0.908552    |
|   2.avi     |     419      |      414        |    0.988067    |
|   4.avi     |     226      |      226        |    1.000000    |

## License

MIT.

## Contact Me

If you have any problem, please contact me at <liberize@gmail.com>.

[Demo1]:            https://raw.githubusercontent.com/liberize/flame-detection-system/master/images/2.gif
[Demo2]:            https://raw.githubusercontent.com/liberize/flame-detection-system/master/images/6.gif
[Video Source]:     http://signal.ee.bilkent.edu.tr/VisiFire/
