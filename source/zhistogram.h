#ifndef ZHISTOGRAM_H
#define ZHISTOGRAM_H


#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>

class ZHistogram
{
private:
    int histSize[3];        //直方图中箱子的数量
    float hranges[2];    //值范围
    const float * ranges[3];        //值范围的指针
    int channels[3];        //要检查的通道数量

public:
    ZHistogram();
    cv::Mat getHistogram(const cv::Mat & image);
    std::vector<cv::Mat> getHistogramImage(const cv::Mat & image, int zoom = 1);
    static std::vector<cv::Mat> getImageOfHistogram(const cv::Mat & hist, int zoom);
};

#endif // ZHISTOGRAM_H
