#include "zalgorithmset.h"

ZAlgorithmSet::ZAlgorithmSet(QObject *parent) : QObject(parent)
{

}
//peak signal to noise ratio
//峰值信噪比
double ZAlgorithmSet::ZGetPSNR(const cv::Mat &mat1,const cv::Mat &mat2)
{
    cv::Mat matTmp;
    cv::absdiff(mat1,mat2,matTmp);
    matTmp.convertTo(matTmp,CV_32F);
    matTmp=matTmp.mul(matTmp);

    cv::Scalar s=cv::sum(matTmp); //sum elements per channel.

    double sse=s.val[0]+s.val[1]+s.val[2]; //sum channels.

    if(sse<1e-10) //for small values return zero.
    {
        return 0;
    }else{
        double mse=sse/(double)(mat1.channels()*mat1.total());
        double psnr=10.0*log10((255*255)/mse);
        return psnr;
    }
}
cv::Scalar ZAlgorithmSet::ZGetSSIM(const cv::Mat &mat1,const cv::Mat &mat2)
{
    const double C1 = 6.5025, C2 = 58.5225;
    /***************************** INITS **********************************/
    int d     = CV_32F;

    cv::Mat I1, I2;
    mat1.convertTo(I1, d);           // cannot calculate on one byte large values
    mat2.convertTo(I2, d);

    cv::Mat I2_2=I2.mul(I2);        // I2^2
    cv::Mat I1_2=I1.mul(I1);        // I1^2
    cv::Mat I1_I2=I1.mul(I2);        // I1 * I2

    /***********************PRELIMINARY COMPUTING ******************************/

    cv::Mat mu1,mu2;   //
    cv::GaussianBlur(I1, mu1,cv::Size(11, 11), 1.5);
    cv::GaussianBlur(I2, mu2,cv::Size(11, 11), 1.5);

    cv::Mat mu1_2=mu1.mul(mu1);
    cv::Mat mu2_2=mu2.mul(mu2);
    cv::Mat mu1_mu2=mu1.mul(mu2);

    cv::Mat sigma1_2, sigma2_2, sigma12;

    cv::GaussianBlur(I1_2, sigma1_2, cv::Size(11, 11), 1.5);
    sigma1_2 -= mu1_2;

    cv::GaussianBlur(I2_2, sigma2_2, cv::Size(11, 11), 1.5);
    sigma2_2 -= mu2_2;

    cv::GaussianBlur(I1_I2, sigma12, cv::Size(11, 11), 1.5);
    sigma12 -= mu1_mu2;

    ///////////////////////////////// FORMULA ////////////////////////////////
    cv::Mat t1, t2, t3;

    t1 = 2 * mu1_mu2 + C1;
    t2 = 2 * sigma12 + C2;
    t3 = t1.mul(t2);              // t3 = ((2*mu1_mu2 + C1).*(2*sigma12 + C2))

    t1 = mu1_2 + mu2_2 + C1;
    t2 = sigma1_2 + sigma2_2 + C2;
    t1 = t1.mul(t2);               // t1 =((mu1_2 + mu2_2 + C1).*(sigma1_2 + sigma2_2 + C2))

    cv::Mat ssim_map;
    cv::divide(t3, t1, ssim_map);      // ssim_map =  t3./t1;

    cv::Scalar mssim = cv::mean( ssim_map ); // mssim = average of ssim map
    return mssim;
}

qint32 ZAlgorithmSet::ZPerHash(const cv::Mat &mat1, const cv::Mat &mat2)
{
    cv::Mat matDst1, matDst2;
    cv::resize(mat1, matDst1, cv::Size(8, 8), 0, 0, cv::INTER_CUBIC);
    cv::resize(mat2, matDst2, cv::Size(8, 8), 0, 0, cv::INTER_CUBIC);
    cv::cvtColor(matDst1, matDst1, CV_BGR2GRAY);
    cv::cvtColor(matDst2, matDst2, CV_BGR2GRAY);

    int iAvg1 = 0, iAvg2 = 0;
    int arr1[64], arr2[64];

    for (int i = 0; i < 8; i++)
    {
        uchar* data1 = matDst1.ptr<uchar>(i);
        uchar* data2 = matDst2.ptr<uchar>(i);

        int tmp = i * 8;

        for (int j = 0; j < 8; j++)
        {
            int tmp1 = tmp + j;

            arr1[tmp1] = data1[j] / 4 * 4;
            arr2[tmp1] = data2[j] / 4 * 4;

            iAvg1 += arr1[tmp1];
            iAvg2 += arr2[tmp1];
        }
    }

    iAvg1 /= 64;
    iAvg2 /= 64;

    for (int i = 0; i < 64; i++)
    {
        arr1[i] = (arr1[i] >= iAvg1) ? 1 : 0;
        arr2[i] = (arr2[i] >= iAvg2) ? 1 : 0;
    }

    int iDiffNum = 0;

    for (int i = 0; i < 64; i++)
        if (arr1[i] != arr2[i])
            ++iDiffNum;

    cout<<"iDiffNum = "<<iDiffNum<<endl;

    if (iDiffNum <= 5)
        cout<<"two images are very similar!"<<endl;
    else if (iDiffNum > 10)
    {
        iDiffNum=10;
        cout<<"they are two different images!"<<endl;
    }
    else
        cout<<"two image are somewhat similar!"<<endl;

    return iDiffNum;
}
