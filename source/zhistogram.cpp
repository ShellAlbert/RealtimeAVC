#include "zhistogram.h"

ZHistogram::ZHistogram()
{
    histSize[0] = 256;
    histSize[1] = 256;
    histSize[2] = 256;
    hranges[0] = 0.0;
    hranges[1] = 256.0;
    ranges[0] = hranges;
    ranges[1] = hranges;
    ranges[2] = hranges;
    channels[0] = 0;
    channels[1] = 1;
    channels[2] = 2;
}

cv::Mat ZHistogram::getHistogram(const cv::Mat & image)
{
    cv::Mat hist;

    hranges[0] = 0.0;
    hranges[1] = 256.0;
    channels[0] = 0;
    channels[1] = 1;
    channels[2] = 2;

    cv::calcHist(&image, 1, channels, cv::Mat(), hist, 3, histSize, ranges);

    return hist;
}

std::vector<cv::Mat> ZHistogram::getHistogramImage(const cv::Mat & image, int zoom)
{
    cv::Mat hist = getHistogram(image);

    return ZHistogram::getImageOfHistogram(hist, zoom);
}

std::vector<cv::Mat> ZHistogram::getImageOfHistogram(const cv::Mat & hist, int zoom)
{
    int scale = 2;

    float hist_b[256];
    float hist_g[256];
    float hist_r[256];

    memset(hist_b, 0, 256 * sizeof(float));
    memset(hist_g, 0, 256 * sizeof(float));
    memset(hist_r, 0, 256 * sizeof(float));

    //计算三个通道的直方图
    for(int b = 0; b < 256; b ++ )
    {
        for(int g = 0; g < 256; g ++)
        {
            for(int r = 0; r < 256; r ++)
            {
                float binVal = hist.at<float>(b, g, r);
                hist_b[b] += binVal;
                hist_g[g] += binVal;
                hist_r[r] += binVal;
            }
        }
    }

    //获得三个通道直方图中的最大值
    double max_b = 0.0, max_g = 0.0,max_r = 0.0;
    for(int i = 0; i < 256; i ++)
    {
        if(hist_b[i] > max_b)
        {
            max_b = hist_b[i];
        }
    }
    for(int i = 0; i < 256; i ++)
    {
        if(hist_g[i] > max_g)
        {
            max_g = hist_g[i];
        }
    }
    for(int i = 0; i < 256; i ++)
    {
        if(hist_r[i] > max_r)
        {
            max_r = hist_r[i];
        }
    }

    //初始化空的图
    cv::Mat b_img = cv::Mat::zeros(256, 256 * scale, CV_8UC3);
    cv::Mat g_img = cv::Mat::zeros(256, 256 * scale, CV_8UC3);
    cv::Mat r_img = cv::Mat::zeros(256, 256 * scale, CV_8UC3);

    //绘制三个通道的直方图
    for(int i = 0; i < 256; i ++)
    {
        int intensity = cvRound(hist_b[i] * b_img.rows / max_b);
        cv::rectangle(b_img, cv::Point(i * scale, b_img.rows - intensity), cv::Point((i + 1) * scale - 1, b_img.rows - 1), cv::Scalar(255, 0 ,0), 1);
    }
    for(int i = 0; i < 256; i ++)
    {
        int intensity = cvRound(hist_g[i] * g_img.rows / max_g);
        cv::rectangle(g_img, cv::Point(i * scale, g_img.rows - intensity), cv::Point((i + 1) * scale - 1, g_img.rows - 1), cv::Scalar(0, 255, 0), 1);
    }
    for(int i = 0; i < 256; i ++)
    {
        int intensity = cvRound(hist_r[i] * r_img.rows / max_r);
        cv::rectangle(r_img, cv::Point(i * scale, r_img.rows - intensity), cv::Point((i + 1) * scale - 1, r_img.rows - 1), cv::Scalar(0, 0, 255), 1);
    }

    std::vector<cv::Mat> imgs;
    imgs.push_back(b_img);
    imgs.push_back(g_img);
    imgs.push_back(r_img);

    return imgs;
}
