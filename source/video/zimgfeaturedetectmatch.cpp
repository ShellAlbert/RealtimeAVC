#include "zimgfeaturedetectmatch.h"

ZImgFeatureDetectMatch::ZImgFeatureDetectMatch()
{

}
//surf/sink/orb/brisk/kaze/akaze
//blobbrief/fastbrief
//blobfreak/fastbreak
//blobdaisy/fastdaisy
void ZImgFeatureDetectMatch::detect_and_compute(string type, const Mat& img, vector<KeyPoint>& kpts, Mat& desc)
{
    if (type.find("fast")==0)
    {
        type = type.substr(4);
        Ptr<FastFeatureDetector> detector = FastFeatureDetector::create(10, true);
        detector->detect(img, kpts);
    }

    if (type.find("blob") == 0)
    {
        type = type.substr(4);
        Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create();
        detector->detect(img, kpts);

    }

//    if (type == "surf")
//    {
//        Ptr<Feature2D> surf = SURF::create(800.0);
//        surf->detectAndCompute(img, Mat(), kpts, desc);
//    }

//    if (type == "sift")
//    {
//        Ptr<Feature2D> sift = SIFT::create();
//        sift->detectAndCompute(img, Mat(), kpts, desc);
//    }

    if (type == "orb")
    {
        Ptr<ORB> orb = ORB::create();
        orb->detectAndCompute(img, Mat(), kpts, desc);
    }

    if (type == "brisk")
    {
        Ptr<BRISK> brisk = BRISK::create();
        brisk->detectAndCompute(img, Mat(), kpts, desc);
    }

    if (type == "kaze")
    {
        Ptr<KAZE> kaze = KAZE::create();
        kaze->detectAndCompute(img, Mat(), kpts, desc);
    }

    if (type == "akaze")
    {
        Ptr<AKAZE> akaze = AKAZE::create();
        akaze->detectAndCompute(img, Mat(), kpts, desc);
    }

//    if (type == "freak")
//    {
//        Ptr<FREAK> freak = FREAK::create();
//        freak->compute(img, kpts, desc);
//    }

//    if (type == "daisy")
//    {
//        Ptr<DAISY> daisy = DAISY::create();
//        daisy->compute(img, kpts, desc);
//    }

//    if (type == "brief")
//    {
//        Ptr<BriefDescriptorExtractor> brief = BriefDescriptorExtractor::create(64);
//        brief->compute(img, kpts, desc);
//    }
    return;
}

void ZImgFeatureDetectMatch::match(string type, Mat& desc1, Mat& desc2, vector<DMatch>& matches)
{
    matches.clear();
    if (type == "bf")
    {
        BFMatcher desc_matcher(cv::NORM_L2, true);
        desc_matcher.match(desc1, desc2, matches, Mat());
    }

    if (type == "knn")
    {
        BFMatcher desc_matcher(cv::NORM_L2, true);
        vector< vector<DMatch> > vmatches;
        desc_matcher.knnMatch(desc1, desc2, vmatches, 1);
        for (int i = 0; i < static_cast<int>(vmatches.size()); ++i)
        {
            if (!vmatches[i].size())
            {
                continue;
            }
            matches.push_back(vmatches[i][0]);
        }
    }

    std::sort(matches.begin(), matches.end());
    while (matches.front().distance * kDistanceCoef < matches.back().distance)
    {
        matches.pop_back();
    }

    while (matches.size() > kMaxMatchingSize)
    {
        matches.pop_back();
    }
    return;
}
void ZImgFeatureDetectMatch::findKeyPointsHomography(vector<KeyPoint>& kpts1, vector<KeyPoint>& kpts2,vector<DMatch>& matches, vector<char>& match_mask)
{
    if (static_cast<int>(match_mask.size()) < 3)
    {
        return;
    }

    vector<Point2f> pts1;
    vector<Point2f> pts2;

    for (int i = 0; i < static_cast<int>(matches.size()); ++i)
    {
        pts1.push_back(kpts1[matches[i].queryIdx].pt);
        pts2.push_back(kpts2[matches[i].trainIdx].pt);

    }

    findHomography(pts1, pts2, cv::RANSAC, 4, match_mask);
}
