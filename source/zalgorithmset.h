#ifndef ZALGORITHMSET_H
#define ZALGORITHMSET_H

#include <QObject>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
using namespace std;
class ZAlgorithmSet : public QObject
{
    Q_OBJECT
public:
    explicit ZAlgorithmSet(QObject *parent = nullptr);

    //图像相似度评估PSNR SSIM
    double ZGetPSNR(const cv::Mat &mat1,const cv::Mat &mat2);
    cv::Scalar ZGetSSIM(const cv::Mat &mat1,const cv::Mat &mat2);

    //图像相似度评估 感知哈希算法
    qint32 ZPerHash(const cv::Mat &mat1,const cv::Mat &mat2);

signals:

public slots:
public:

};
#endif // ZALGORITHMSET_H
