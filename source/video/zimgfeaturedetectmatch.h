#ifndef ZIMGFEATUREDETECTMATCH_H
#define ZIMGFEATUREDETECTMATCH_H

#include <vector>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d.hpp>
//#include <opencv2/xfeatures2d.hpp>
//#include <opencv2/xfeatures2d/nonfree.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/ocl.hpp>
#include <QString>
using std::cout;
using std::cerr;
using std::vector;
using std::string;
using cv::Mat;
using cv::Point2f;
using cv::KeyPoint;
using cv::Scalar;
using cv::Ptr;
using cv::FastFeatureDetector;
using cv::SimpleBlobDetector;
using cv::DMatch;
using cv::BFMatcher;
using cv::DrawMatchesFlags;
using cv::Feature2D;
using cv::ORB;
using cv::BRISK;
using cv::AKAZE;
using cv::KAZE;
//using cv::xfeatures2d::BriefDescriptorExtractor;
//using cv::xfeatures2d::SURF;
//using cv::xfeatures2d::SIFT;
//using cv::xfeatures2d::DAISY;
//using cv::xfeatures2d::FREAK;

const double kDistanceCoef = 4.0;
const int kMaxMatchingSize = 50;
class ZImgFeatureDetectMatch
{
public:
    ZImgFeatureDetectMatch();

    //surf/sink/orb/brisk/kaze/akaze
    //blobbrief/fastbrief
    //blobfreak/fastbreak
    //blobdaisy/fastdaisy
    void detect_and_compute(string type, const Mat& img, vector<KeyPoint>& kpts, Mat& desc);
    //match algorithm (bf/knn).
    void match(string type, Mat& desc1, Mat& desc2, vector<DMatch>& matches);
    void findKeyPointsHomography(vector<KeyPoint>& kpts1, vector<KeyPoint>& kpts2,vector<DMatch>& matches, vector<char>& match_mask);

};

#endif // ZIMGFEATUREDETECTMATCH_H
