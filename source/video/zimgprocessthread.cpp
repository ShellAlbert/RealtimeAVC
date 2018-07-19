#include "zimgprocessthread.h"
#include "zgblpara.h"
#include <QDateTime>
#include "zalgorithmset.h"
#include "zhistogram.h"
#include "zimgfeaturedetectmatch.h"
#include <QDebug>
#include <QApplication>
ZImgProcessThread::ZImgProcessThread()
{
    this->m_uart=NULL;
    this->m_timerProcess=NULL;
    this->m_bRunning=false;

    this->m_queueMain=NULL;
    this->m_semaMainUsed=NULL;
    this->m_semaMainFree=NULL;

    this->m_queueAux=NULL;
    this->m_semaAuxUsed=NULL;
    this->m_semaAuxFree=NULL;

    this->m_queueProcessedSet=NULL;
    this->m_semaProcessedSetUsed=NULL;
    this->m_semaProcessedSetFree=NULL;
}
ZImgProcessThread::~ZImgProcessThread()
{

}
qint32 ZImgProcessThread::ZBindMainQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueMain=queue;
    this->m_semaMainUsed=semaUsed;
    this->m_semaMainFree=semaFree;
    return 0;
}
qint32 ZImgProcessThread::ZBindAuxQueue(QQueue<QImage> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueAux=queue;
    this->m_semaAuxUsed=semaUsed;
    this->m_semaAuxFree=semaFree;
    return 0;
}
qint32 ZImgProcessThread::ZBindProcessedSetQueue(QQueue<ZImgProcessedSet> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueProcessedSet=queue;
    this->m_semaProcessedSetUsed=semaUsed;
    this->m_semaProcessedSetFree=semaFree;
    return 0;
}
qint32 ZImgProcessThread::ZStartThread()
{
    //check main queue.
    if(this->m_queueMain==NULL || this->m_semaMainUsed==NULL || this->m_semaMainFree==NULL)
    {
        qDebug()<<"<error>:no bind main queue,cannot start.";
        return -1;
    }

    //check aux queue.
    if(this->m_queueAux==NULL || this->m_semaAuxUsed==NULL || this->m_semaAuxFree==NULL)
    {
        qDebug()<<"<error>:no bind aux main,cannot start.";
        return -1;
    }

    this->start();
    return 0;
}
qint32 ZImgProcessThread::ZStopThread()
{
    this->exit(0);
    return 0;
}
bool ZImgProcessThread::ZIsRunning()
{
    return this->m_bRunning;
}
void ZImgProcessThread::ZDumpDiffXY2UART(qint32 nDiffX,qint32 nDiffY)
{
    char diffX[2];
    char diffY[2];
    gGblPara.int32_char8x2_low(nDiffX,diffX);
    gGblPara.int32_char8x2_low(nDiffY,diffY);

    char txBuffer[12];
    txBuffer[0]=0x5A;//frame head.
    txBuffer[1]=0xA5;
    txBuffer[2]=12;//frame length,12 bytes.
    txBuffer[3]=0x02;//command.
    txBuffer[4]=0x01;//endpoint.
    txBuffer[5]=0x23;//attribution id.
    txBuffer[6]=0x04;//data length.
    txBuffer[7]=diffX[1]; //diff x.
    txBuffer[8]=diffX[0];
    txBuffer[9]=diffY[1];//diff y.
    txBuffer[10]=diffY[0];
    txBuffer[11]=(char)gGblPara.ZDoCheckSum((uint8_t*)txBuffer,11);

    this->m_uart->write(txBuffer,sizeof(txBuffer));
    this->m_uart->waitForBytesWritten(1000);

    //    if(gGblPara.m_bDumpUART)
    //    {
    //        qint32 offset=0;
    //        char fmtBuffer[64];
    //        memset(fmtBuffer,0,sizeof(fmtBuffer));
    //        for(unsigned int i=0;i<sizeof(txBuffer);i++)
    //        {
    //            sprintf(&fmtBuffer[offset],"%02d ",txBuffer[i]);
    //            offset+=3;
    //        }
    //        qDebug()<<fmtBuffer;
    //    }
}
void ZImgProcessThread::ZSlotSerialPortErr(QSerialPort::SerialPortError err)
{
    qDebug()<<err;
    qApp->exit(-1);
}
void ZImgProcessThread::run()
{
    qDebug()<<"<MainLoop>:ImgProcessThread starts.";
    while(!gGblPara.m_bGblRst2Exit)
    {
        //1.fetch QImage from main queue.
        QImage imgMain;
        this->m_semaMainUsed->acquire();//已用信号量减1.
        imgMain=this->m_queueMain->dequeue();
        this->m_semaMainFree->release();//空闲信号量加1.

        //2.fetch QImage from aux queue.
        QImage imgAux;
        this->m_semaAuxUsed->acquire();//已用信号量减1.
        imgAux=this->m_queueAux->dequeue();
        this->m_semaAuxFree->release();//空闲信号量加1.

        //3.将QImage转换为cvMat以方便openCV处理.
        cv::Mat mat1=QImage2cvMat(imgMain);
        cv::Mat mat2=QImage2cvMat(imgAux);

        //这里优先启动fMode
        if(gGblPara.m_bFMode)
        {
            //牲征点匹配.
            cv::Mat mat1Gray,mat2Gray;
            cv::cvtColor(mat1,mat1Gray,CV_BGR2GRAY);
            cv::cvtColor(mat2,mat2Gray,CV_BGR2GRAY);

            //cut a 200x200 rectangle.
            cv::Rect rectObj((mat1Gray.cols-300)/2,(mat1Gray.rows-300)/2,300,300);
            cv::Mat mat1Cut(mat1Gray,rectObj);

            cv::Mat matObjKp,matSceneKp,matResult;
            //parameter1 : obj
            //parameter2: scene.
            qint32 nRetFDM=this->ZDoFeatureDetectMatch(mat1Cut,mat2Gray,matObjKp,matSceneKp,matResult,rectObj);

            if(gGblPara.m_bFMode && 0==nRetFDM)
            {
                //将图像复制到一个图像上
                //matObjKp.copyTo(mat1Gray(cv::Rect(0,0,300,300)));
                emit this->ZSigObjFeatureKeyPoints(cvMat2QImage(matObjKp));
                emit this->ZSigSceneFeatureKeyPoints(cvMat2QImage(matSceneKp));
            }
        }else if(gGblPara.m_bXMode)
        {
            //模板匹配有2种方法
            //普通模式作全局匹配，XMode将图像降分辨率后再匹配
            cv::Mat mat1Gray,mat2Gray;
            cv::cvtColor(mat1,mat1Gray,CV_BGR2GRAY);
            cv::cvtColor(mat2,mat2Gray,CV_BGR2GRAY);
            //resize image to 1/2.
            cv::Mat mat1GrayResize,mat2GrayResize;
            cv::resize(mat1Gray,mat1GrayResize,cv::Size(mat1.cols/2,mat1.rows/2));
            cv::resize(mat2Gray,mat2GrayResize,cv::Size(mat2.cols/2,mat2.rows/2));

            //如果命令行参数-x启用了XMode
            //则将图像做1/2缩小，然后比对，这样可以大大节省时间
            //同时校准中心点(x,y)分别也做(x/2,y/2）
            //同时裁剪尺寸(w,h)分别也做1/2处理（w/2,h/2)
            //这样做完matchTemplate后，再将最合适的匹配点坐标再乘以2,
            //然后在原图未绽放的图像上画矩形框
            this->ZDoTemplateMatchResize(mat1GrayResize,mat2GrayResize,true);

            //这里做PSNR/SSIM算法来评估图像相似度
            //SSIM适合2个内容完全一样，但模糊度，噪声水平等不同的图片进行相似度估计
            this->ZDoPSNRAndSSIM(mat1GrayResize,mat2GrayResize);
        }else{
            //直接从图像1中以校准中心点为中心，截取一块矩形区域
            //然后从图像2中全局匹配该区域
            this->ZDoTemplateMatchDirectly(mat1,mat2,true);

            //这里做PSNR/SSIM算法来评估图像相似度
            this->ZDoPSNRAndSSIM(mat1,mat2);
        }
        //qDebug()<<"processed 2 images.";
        this->usleep(VIDEO_THREAD_SCHEDULE_US);
    }
    qDebug()<<"<MainLoop>:ImgProcessThread ends.";
    //此处设置本线程退出标志.
    //同时设置全局请求退出标志，请求其他线程退出.
    gGblPara.m_bImgCmpThreadExitFlag=true;
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
    return;
#if 0
    do{
        if(gGblPara.m_bDumpUART)
        {
            this->m_uart=new QSerialPort;
            this->m_uart->setPortName(gGblPara.m_uartName);
            this->m_uart->setBaudRate(QSerialPort::Baud115200);
            this->m_uart->setDataBits(QSerialPort::Data8);
            this->m_uart->setParity(QSerialPort::NoParity);
            this->m_uart->setStopBits(QSerialPort::OneStop);
            this->m_uart->setFlowControl(QSerialPort::NoFlowControl);
            if(!this->m_uart->open(QIODevice::ReadWrite))
            {
                qDebug()<<"failed to open uart:"<<this->m_uart->portName();
                break;
            }
            QObject::connect(this->m_uart,SIGNAL(error(QSerialPort::SerialPortError)),this,SLOT(ZSlotSerialPortErr(QSerialPort::SerialPortError)),Qt::DirectConnection);
            QObject::connect(this->m_uart,SIGNAL(readyRead()),this,SLOT(ZSlotReadUARTData()),Qt::DirectConnection);
        }
        this->m_timerProcess=new QTimer;
        QObject::connect(this->m_timerProcess,SIGNAL(timeout()),this,SLOT(ZSlotScheduleTask()),Qt::DirectConnection);
        //do not start at default.
        //wait for uart command: start/stop.
        this->m_timerCtl=new QTimer;
        QObject::connect(this->m_timerCtl,SIGNAL(timeout()),this,SLOT(ZSlotCheckGblStartFlag()),Qt::DirectConnection);
        this->m_timerCtl->start(1000);//1s.

        //QTimer::singleShot(5000,this,SLOT(ZSlotDelayStart()));

        //set flag.
        this->m_bRunning=true;
        //enter event-loop until exit() called.
        this->exec();

        //do some clean.
        this->m_timerCtl->stop();
        delete this->m_timerCtl;

        this->m_timerProcess->stop();
        delete this->m_timerProcess;
    }while(0);

    //here we set global request exit flag to help other thread to exit.
    gGblPara.m_bGblRst2Exit=true;

    if(gGblPara.m_bDumpUART)
    {
        this->m_uart->close();
        delete this->m_uart;
    }
    qDebug()<<"thread ImgProcess exit okay.";
    //set flag.
    this->m_bRunning=false;
    emit this->ZSigThreadFinished();
#endif
}
#if 0
void ZImgProcessThread::ZSlotCheckGblStartFlag()
{
    if(gGblPara.m_bGblStartFlag)
    {
        if(!this->m_timerProcess->isActive())
        {
            qDebug()<<"detected start flag,start ImgProcessThread.";
            this->m_timerProcess->start(10);
        }
    }else{
        if(this->m_timerProcess->isActive())
        {
            qDebug()<<"detected stop flag,stop ImgProcessThread.";
            this->m_timerProcess->stop();
        }
    }
}
void ZImgProcessThread::ZSlotScheduleTask()
{
    //if the global request exit flag was set.
    //when we exit event-loop.
    if(gGblPara.m_bGblRst2Exit)
    {
        this->exit(0);
        return;
    }

    //step1. fetch two images from vector.
    QImage img1,img2;
    this->m_mux1.lock();
    if(this->m_imgVector1.size()>0)
    {
        img1=this->m_imgVector1.first();
    }
    this->m_mux2.lock();
    if(this->m_imgVector2.size()>0)
    {
        img2=this->m_imgVector2.first();
    }
    //missing frame,process next time.
    if(img1.isNull() || img2.isNull())
    {
        //emit this->ZSigMsg(tr("missing image1,retry %1 ms again!").arg(nSleepMS),Log_Msg_Warning);
        //emit this->ZSigMsg(tr("missing image2,retry %1 ms again!").arg(nSleepMS),Log_Msg_Warning);
        this->m_mux1.unlock();
        this->m_mux2.unlock();
        return;
    }
    //two frames are ready.
    this->m_imgVector1.removeFirst();
    this->m_imgVector2.removeFirst();
    this->m_mux1.unlock();
    this->m_mux2.unlock();

    //将QImage转换为cvMat以方便openCV处理.
    cv::Mat mat1=QImage2cvMat(img1);
    cv::Mat mat2=QImage2cvMat(img2);

    //emit this->ZSigImgProcessed(img1,img2);
    //对图像1和图像2进行感知哈希相似度匹配(RGB)
    //tStartMS=QDateTime::currentDateTime().toMSecsSinceEpoch();
    //        qint32 perHash=algorithm.ZPerHash(mat1,mat2);
    //        emit this->ZSigPerHashImgSimilarity(100-perHash*10);
    //tEndMS=QDateTime::currentDateTime().toMSecsSinceEpoch();
    //qDebug()<<"PerHash costs:"<<tEndMS-tStartMS<<"(ms)";


    //这里优先启动fMode
    if(gGblPara.m_bFMode)
    {
        //牲征点匹配.
        cv::Mat mat1Gray,mat2Gray;
        cv::cvtColor(mat1,mat1Gray,CV_BGR2GRAY);
        cv::cvtColor(mat2,mat2Gray,CV_BGR2GRAY);
        //cut a 200x200 rectangle.
        //we cut a width*height image centered on calibrate center.
        //         qint32 nCutX=gGblPara.m_calibrateX1-gGblPara.m_nCutTemplateWidth/2;
        //         qint32 nCutY=gGblPara.m_calibrateY1-gGblPara.m_nCutTemplateHeight/2;
        //         if(nCutX<0)
        //         {
        //             nCutX=0;
        //         }
        //         if(nCutY<0)
        //         {
        //             nCutY=0;
        //         }
        //         cv::Rect rectObj(nCutX,nCutY,gGblPara.m_nCutTemplateWidth,gGblPara.m_nCutTemplateHeight);
        //         cv::Mat mat1Cut(mat1Gray,rectObj);

        //cut a 200x200 rectangle.
        cv::Rect rectObj((mat1Gray.cols-300)/2,(mat1Gray.rows-300)/2,300,300);
        cv::Mat mat1Cut(mat1Gray,rectObj);

        cv::Mat matObjKp,matSceneKp,matResult;
        //parameter1 : obj
        //parameter2: scene.
        qint32 nRetFDM=this->ZDoFeatureDetectMatch(mat1Cut,mat2Gray,matObjKp,matSceneKp,matResult,rectObj);

        if(gGblPara.m_bFMode && 0==nRetFDM)
        {
            //将图像复制到一个图像上
            //matObjKp.copyTo(mat1Gray(cv::Rect(0,0,300,300)));
            emit this->ZSigObjFeatureKeyPoints(cvMat2QImage(matObjKp));
            emit this->ZSigSceneFeatureKeyPoints(cvMat2QImage(matSceneKp));
        }
    }else if(gGblPara.m_bXMode)
    {
        //模板匹配有2种方法
        //普通模式作全局匹配，XMode将图像降分辨率后再匹配
        cv::Mat mat1Gray,mat2Gray;
        cv::cvtColor(mat1,mat1Gray,CV_BGR2GRAY);
        cv::cvtColor(mat2,mat2Gray,CV_BGR2GRAY);
        //resize image to 1/2.
        cv::Mat mat1GrayResize,mat2GrayResize;
        cv::resize(mat1Gray,mat1GrayResize,cv::Size(mat1.cols/2,mat1.rows/2));
        cv::resize(mat2Gray,mat2GrayResize,cv::Size(mat2.cols/2,mat2.rows/2));

        //如果命令行参数-x启用了XMode
        //则将图像做1/2缩小，然后比对，这样可以大大节省时间
        //同时校准中心点(x,y)分别也做(x/2,y/2）
        //同时裁剪尺寸(w,h)分别也做1/2处理（w/2,h/2)
        //这样做完matchTemplate后，再将最合适的匹配点坐标再乘以2,
        //然后在原图未绽放的图像上画矩形框
        this->ZDoTemplateMatchResize(mat1GrayResize,mat2GrayResize,true);

        //这里做PSNR/SSIM算法来评估图像相似度
        //SSIM适合2个内容完全一样，但模糊度，噪声水平等不同的图片进行相似度估计
        this->ZDoPSNRAndSSIM(mat1GrayResize,mat2GrayResize);
    }else{
        //直接从图像1中以校准中心点为中心，截取一块矩形区域
        //然后从图像2中全局匹配该区域
        this->ZDoTemplateMatchDirectly(mat1,mat2,true);

        //这里做PSNR/SSIM算法来评估图像相似度
        this->ZDoPSNRAndSSIM(mat1,mat2);
    }


    this->msleep(10);//to reduce cpu heavy load.
    return;


    //        continue;

    //        //直接对图像作灰度处理并显示.
    //        cv::Mat mat1Gray,mat2Gray;
    //        cv::cvtColor(mat1,mat1Gray,CV_BGR2GRAY);
    //        cv::cvtColor(mat2,mat2Gray,CV_BGR2GRAY);
    //        emit this->ZSigGrayImg1(cvMat2QImage(mat1Gray));


    //        //这里直接作原始图像的灰度图作直方图比对
    //        qint32 nMat12GraySimilarity=this->ZDoHistogramCompare(mat1Gray,mat2Gray);
    //        emit this->ZSigOriginalGrayImgSimilarity(nMat12GraySimilarity);
    //        continue;

    //        //为了保证尺寸，我们将图像2缩放至跟图像1一样的大小
    //        if(mat2Gray.size!=mat1Gray.size)
    //        {
    //            cv::resize(mat2Gray,mat2Gray,cv::Size(mat1Gray.cols,mat1Gray.rows));
    //        }

    //        //这里以图像中心点为中间截取一个1/3 width,1/3 height的矩形作为处理的图像
    //        qint32 nSplitW=mat2Gray.cols/3;
    //        qint32 nSplitH=mat2Gray.rows/3;
    //        cv::Mat mat2GrayCut(mat2Gray,cv::Rect(nSplitW,nSplitH,nSplitW,nSplitH));

    //        emit this->ZSigGrayImg2(cvMat2QImage(mat2GrayCut));

    //Sudoku histogram compare.
    //here return the latest similarity image.
    //为了提高模板匹配度，我们这里将图像2的1/3截取图像跟图像1中的分割的9幅图像作对比，找到相似度最好的那一个来做为匹配模板
    //而不是使用图像2的1/3的图像作为匹配模板
    //这样的匹配度更高

    //        //从图像1中分割的9幅格子中找出跟图像2最接近的那幅图像,并返回它在图像1中的区域位置
    //        cv::Rect rectROIMat1;
    //        cv::Mat mat1GrayCut=this->ZDoSoduKoHistogramCompare(mat1Gray,mat2GrayCut,rectROIMat1);
    //        emit this->ZSigTemplateMatchImg1(cvMat2QImage(mat1GrayCut));
    //#if 1
    //        //作模板匹配
    //        cv::Mat matCutMatch=this->ZDoTemplateMatch(mat1Gray,mat1GrayCut,CV_TM_SQDIFF);
    //        matCutMatch=this->ZDoTemplateMatch(mat1Gray,mat1GrayCut,CV_TM_SQDIFF_NORMED);
    //        matCutMatch=this->ZDoTemplateMatch(mat1Gray,mat1GrayCut,CV_TM_CCORR);
    //        matCutMatch=this->ZDoTemplateMatch(mat1Gray,mat1GrayCut,CV_TM_CCORR_NORMED);
    //        matCutMatch=this->ZDoTemplateMatch(mat1Gray,mat1GrayCut,CV_TM_CCOEFF);
    //        matCutMatch=this->ZDoTemplateMatch(mat1Gray,mat1GrayCut,CV_TM_CCOEFF_NORMED);
    //        emit this->ZSigTemplateMatchImg2(cvMat2QImage(matCutMatch));
    //#endif

    //        //image fusion.
    //        cv::Mat matFusionBig=mat1.clone();
    //        cv::Mat matFusionBigRoi(mat1,rectROIMat1);
    //        //这里以图像中心点为中间截取一个1/3 width,1/3 height的矩形作为处理的图像
    //        qint32 nROIMat2Width=mat2.cols/3;
    //        qint32 nROIMat2Height=mat2.rows/3;
    //        cv::Rect rectROIMat2(nROIMat2Width,nROIMat2Height,nROIMat2Width,nROIMat2Height);
    //        cv::Mat matFusionSmall(mat2,rectROIMat2);
    //        //如果小视场取出的图像尺寸跟大视场的不相等，则需要变换尺寸
    //        //只有相同的尺寸才能贴过去
    //        if(matFusionSmall.cols!=rectROIMat1.width || matFusionSmall.rows!=rectROIMat1.height)
    //        {
    //            cv::resize(matFusionSmall,matFusionSmall,cv::Size(rectROIMat1.width,rectROIMat1.height));
    //        }

    //        //将2个区域图像进行加权融合
    //        cv::Mat matFusionDst;
    //        cv::addWeighted(matFusionBigRoi,0.5,matFusionSmall,0.5,0,matFusionDst);

    //        //将图像复制到一个图像上
    //        matFusionDst.copyTo(matFusionBig(rectROIMat1));
    //        emit this->ZSigFeaturePointsImg1(cvMat2QImage(matFusionBig));

    //区域模板匹配,用于实现目标区域匹配实现自动镜头追踪
    //mat1为大视场，mat2为小视场
    //matCut1为大视场中1/3视场，matCut2为小视场中的1/3视场
    //在mat1中作matCut2的模板匹配，并标注位置
    //这里使用多种方法进行模板匹配.
    //        cv::Mat matCutByTemplate;
    //        //cv::Mat matMatchResult1=this->ZDoTemplateMatch(mat1Gray,matCut2Gray,CV_TM_SQDIFF_NORMED,matCutByTemplate);
    //        //use color image to match template.
    //        cv::Mat matMatchResult1=this->ZDoTemplateMatch(mat1,matCut2,CV_TM_SQDIFF_NORMED,matCutByTemplate);
    //        emit this->ZSigTemplateMatchImg1(cvMat2QImage(matMatchResult1));
    //        cv::Mat matMatchResult2=this->ZDoTemplateMatch(mat1Gray,matCut2Gray,CV_TM_SQDIFF,matCutByTemplate);
    //        emit this->ZSigTemplateMatchImg2(cvMat2QImage(matMatchResult2));

    //进行图像相似度匹配，图像1是大视场中通过模板匹配截取的区域图像，图像2是小视场图像
    //this->ZDoHistogramCompare(matCutByTemplate,matCut2Gray);


#if 0
    //牲征点匹配.
    cv::Mat mat1Kp,mat2Kp,matResult;
    if(0==this->ZDoFeatureDetectMatch(mat1Gray,mat2Gray,mat1Kp,mat2Kp,matResult))
    {

        emit this->ZSigFeaturePointsImg1(cvMat2QImage(mat1Kp));
        emit this->ZSigFeaturePointsImg2(cvMat2QImage(mat2Kp));
        //显示匹配结果
        emit this->ZSigFeaturesMatchImg(cvMat2QImage(matResult));
    }
#endif
    //        //获取RGB三通道的直方图,heavy cpu load.
    //        std::vector<cv::Mat> mat1RGBHistogram=this->m_histogram->getHistogramImage(matCut1);
    //        std::vector<cv::Mat> mat2RGBHistogram=this->m_histogram->getHistogramImage(matCut2);
    //        QImage mat1RedHist=cvMat2QImage(mat1RGBHistogram[0]);
    //        emit this->ZSigHistogram1(mat1RedHist);
    //        QImage mat2RedHist=cvMat2QImage(mat2RGBHistogram[0]);
    //        emit this->ZSigHistogram2(mat2RedHist);

    //        this->msleep(nSleepMS);//to reduce cpu heavy load.
    //    }
    //    delete this->m_histogram;
    //    this->m_histogram=NULL;
    //    this->exit(0);
}
#endif
void ZImgProcessThread::ZSlotReadUARTData()
{
    qDebug()<<"got uart data:"<<this->m_uart->bytesAvailable();
    while(this->m_uart->bytesAvailable())
    {
        QByteArray baRecv=this->m_uart->readAll();
        this->m_baUARTRecvBuf.append(baRecv);

        qDebug()<<"uart rx:"<<baRecv;
        if(this->m_baUARTRecvBuf.contains("start"))
        {
            //set global start flag to cause other thread to start.
            gGblPara.m_bJsonImgPro=true;

            this->m_baUARTRecvBuf.clear();
        }else if(this->m_baUARTRecvBuf.contains("stop"))
        {
            //reset global start flag to cause other thread to stop.
            gGblPara.m_bJsonImgPro=false;

            this->m_baUARTRecvBuf.clear();
        }
    }
    if(this->m_baUARTRecvBuf.size()>64)
    {
        this->m_baUARTRecvBuf.clear();
    }
}

//execute template directly without any processing.
void ZImgProcessThread::ZDoTemplateMatchDirectly(const cv::Mat &mat1,const cv::Mat mat2,const bool bTreatAsGray)
{
    if(bTreatAsGray)
    {
        qint64 nStartMS,nEndMS;
        nStartMS=QDateTime::currentDateTime().toMSecsSinceEpoch();
        //here we only process gray,single channel to reduce cpu load.
        cv::Mat mat1Gray,mat2Gray;
        cv::cvtColor(mat1,mat1Gray,CV_BGR2GRAY);
        cv::cvtColor(mat2,mat2Gray,CV_BGR2GRAY);

        //we cut a width*height image centered on calibrate center.
        qint32 nCutX=gGblPara.m_calibrateX1-gGblPara.m_nCutTemplateWidth/2;
        qint32 nCutY=gGblPara.m_calibrateY1-gGblPara.m_nCutTemplateHeight/2;
        if(nCutX<0)
        {
            nCutX=0;
        }
        if(nCutY<0)
        {
            nCutY=0;
        }
        cv::Rect rectTemplate(nCutX,nCutY,gGblPara.m_nCutTemplateWidth,gGblPara.m_nCutTemplateHeight);
        cv::Mat matTemplateFromMat1(mat1Gray,rectTemplate);

        //do match template.
        cv::Mat matResult;
        cv::matchTemplate(mat2Gray,matTemplateFromMat1,matResult,CV_TM_SQDIFF_NORMED);
        cv::normalize(matResult,matResult,0,1,cv::NORM_MINMAX,-1,cv::Mat());

        //localizing the best match with minMaxLoc.
        double fMinVal,fMaxVal;
        cv::Point ptMinLoc,ptMaxLoc,ptMatched;
        cv::minMaxLoc(matResult,&fMinVal,&fMaxVal,&ptMinLoc,&ptMaxLoc,cv::Mat());
        ptMatched=ptMinLoc;//标准平方差匹配，数据越小匹配度越好

        qint32 nDiffX,nDiffY;
        if(nCutX>ptMatched.x)
        {
            nDiffX=nCutX-ptMatched.x;
        }else if(nCutX<ptMatched.x)
        {
            nDiffX=-(ptMatched.x-nCutX);
        }else{
            nDiffX=0;
        }
        if(nCutY>ptMatched.y)
        {
            nDiffY=nCutY-ptMatched.y;
        }else if(nCutY<ptMatched.y)
        {
            nDiffY=-(ptMatched.y-nCutY);
        }else{
            nDiffY=0;
        }
        nEndMS=QDateTime::currentDateTime().toMSecsSinceEpoch();

        //prepare to send template rectangle & matched rectangle to PC.
        QRect rectTemp(rectTemplate.x,rectTemplate.y,rectTemplate.width,rectTemplate.height);
        QRect rectMatch(ptMatched.x,ptMatched.y,rectTemplate.width,rectTemplate.height);
        emit this->ZSigDiffXYT(rectTemp,rectMatch,nDiffX,nDiffY,nEndMS-nStartMS);

        //dump diff x&y to tty UART.
        if(gGblPara.m_bDumpUART)
        {
            this->ZDumpDiffXY2UART(nDiffX,nDiffY);
        }
        return;
    }else{
        cv::Mat mat1Copy;
        mat1.copyTo(mat1Copy);

        //这里直接取图像2中间视场的1/3去图像1中做模板匹配
        qint32 nW1=mat2.cols/3;
        qint32 nH1=mat2.rows/3;
        qint32 nX1=nW1;
        qint32 nY1=nH1;
        cv::Rect rectTemplate(nX1,nY1,nW1,nH1);
        cv::Mat matTemplateFromMat2(mat2,rectTemplate);

        //calculate the result rectangle size.
        int nWidthResult=mat1.cols-matTemplateFromMat2.cols+1;
        int nHeightResult=mat1.rows-matTemplateFromMat2.rows+1;
        //create result rectangle.
        cv::Mat matResult;
        //matResult.create(cv::Size(nWidthResult,nHeightResult),mat1.type());

        //template matching.
        cv::matchTemplate(mat1,matTemplateFromMat2,matResult,CV_TM_SQDIFF_NORMED);
        cv::normalize(matResult,matResult,0,1,cv::NORM_MINMAX,-1,cv::Mat());

        //localizing the best match with minMaxLoc.
        double fMinVal,fMaxVal;
        cv::Point ptMinLoc,ptMaxLoc,ptMatched;
        cv::minMaxLoc(matResult,&fMinVal,&fMaxVal,&ptMinLoc,&ptMaxLoc,cv::Mat());
        ptMatched=ptMinLoc;//标准平方差匹配，数据越小匹配度越好

        qDebug()<<"Original pt:("<<nX1<<","<<nY1<<")"<<",Match pt:("<<ptMatched.x<<","<<ptMatched.y<<")";
        qint32 nDiffX,nDiffY;
        if(nX1>ptMatched.x)
        {
            nDiffX=nX1-ptMatched.x;
        }else if(nX1<ptMatched.x)
        {
            nDiffX=-(ptMatched.x-nX1);
        }else{
            nDiffX=0;
        }
        if(nY1>ptMatched.y)
        {
            nDiffY=nY1-ptMatched.y;
        }else if(nY1<ptMatched.y)
        {
            nDiffY=-(ptMatched.y-nY1);
        }else{
            nDiffY=0;
        }
        //emit this->ZSigTemplateMatchDiffXY(nDiffX,nDiffY,0);

        //在原始图像上将本身的区域标识出来，同时把匹配到的图像标识出来
        cv::Rect rectMatched(ptMatched.x,ptMatched.y,matTemplateFromMat2.cols,matTemplateFromMat2.rows);
        cv::rectangle(mat1Copy,rectTemplate,cv::Scalar(0,255,0),2,8,0);
        cv::rectangle(mat1Copy,rectMatched,cv::Scalar(0,0,255),2,8,0);
        return;
    }
}
void ZImgProcessThread::ZDoTemplateMatchResize(const cv::Mat &mat1,const cv::Mat mat2,const bool bTreatAsGray)
{

    qint64 nStartMS,nEndMS;
    nStartMS=QDateTime::currentDateTime().toMSecsSinceEpoch();

    //we cut a width*height image centered on calibrate center.
    qint32 nCutX=gGblPara.m_calibrateX1-gGblPara.m_nCutTemplateWidth/2;
    qint32 nCutY=gGblPara.m_calibrateY1-gGblPara.m_nCutTemplateHeight/2;
    //整体尺寸绽小2倍，所以坐标也要缩小2倍
    nCutX=(nCutX<0)?0:(nCutX/2);
    nCutY=(nCutY<0)?0:(nCutY/2);
    //整体尺寸绽小2倍，模板的长宽也要缩小2倍.
    cv::Rect rectTemplate(nCutX,nCutY,gGblPara.m_nCutTemplateWidth/2,gGblPara.m_nCutTemplateHeight/2);
    cv::Mat matTemplateFromMat1Resize(mat1,rectTemplate);

    //do match template.
    cv::Mat matResult;
    cv::matchTemplate(mat2,matTemplateFromMat1Resize,matResult,CV_TM_SQDIFF_NORMED);
    cv::normalize(matResult,matResult,0,1,cv::NORM_MINMAX,-1,cv::Mat());

    //localizing the best match with minMaxLoc.
    double fMinVal,fMaxVal;
    cv::Point ptMinLoc,ptMaxLoc,ptMatched;
    cv::minMaxLoc(matResult,&fMinVal,&fMaxVal,&ptMinLoc,&ptMaxLoc,cv::Mat());
    ptMatched=ptMinLoc;//标准平方差匹配，数据越小匹配度越好

    qint32 nDiffX,nDiffY;
#if 0
    if(nCutX>ptMatched.x)
    {
        nDiffX=nCutX-ptMatched.x;
    }else if(nCutX<ptMatched.x)
    {
        nDiffX=-(ptMatched.x-nCutX);
    }else{
        nDiffX=0;
    }
    if(nCutY>ptMatched.y)
    {
        nDiffY=nCutY-ptMatched.y;
    }else if(nCutY<ptMatched.y)
    {
        nDiffY=-(ptMatched.y-nCutY);
    }else{
        nDiffY=0;
    }
#endif
#if 0
    if(gGblPara.m_calibrateX2>ptMatched.x)
    {
        nDiffX=gGblPara.m_calibrateX2-ptMatched.x;
    }else if(gGblPara.m_calibrateX2<ptMatched.x)
    {
        nDiffX=-(ptMatched.x-gGblPara.m_calibrateX2);
    }else{
        nDiffX=0;
    }
    if(gGblPara.m_calibrateY2>ptMatched.y)
    {
        nDiffY=gGblPara.m_calibrateY2-ptMatched.y;
    }else if(gGblPara.m_calibrateY2<ptMatched.y)
    {
        nDiffY=-(ptMatched.y-gGblPara.m_calibrateY2);
    }else{
        nDiffY=0;
    }
#endif
    nEndMS=QDateTime::currentDateTime().toMSecsSinceEpoch();

    //prepare to send template rectangle & matched rectangle to PC.
    QRect rectTemp(rectTemplate.x*2,rectTemplate.y*2,rectTemplate.width*2,rectTemplate.height*2);
    QRect rectMatch(ptMatched.x*2,ptMatched.y*2,rectTemplate.width*2,rectTemplate.height*2);

    nDiffX=gGblPara.m_calibrateX2/2-(ptMatched.x+rectTemplate.width/2);
    nDiffY=gGblPara.m_calibrateY2/2-(ptMatched.y+rectTemplate.height/2);
    //emit this->ZSigDiffXYT(rectTemp,rectMatch,nDiffX*2,nDiffY*2,nEndMS-nStartMS);
    //将计算结果投入processedSet队列中.
    ZImgProcessedSet newProcessedSet;
    newProcessedSet.rectTemplate=rectTemp;
    newProcessedSet.rectMatched=rectMatch;
    newProcessedSet.nDiffX=nDiffX*2;
    newProcessedSet.nDiffY=nDiffY*2;
    newProcessedSet.nCostMs=nEndMS-nStartMS;
    this->m_semaProcessedSetFree->acquire();//空闲信号量减1.
    this->m_queueProcessedSet->enqueue(newProcessedSet);
    this->m_semaProcessedSetUsed->release();//已用信号量加1.

    //qDebug()<<"put processed set.";
    //dump diff x&y to tty UART.
    if(gGblPara.m_bDumpUART)
    {
        this->ZDumpDiffXY2UART(nDiffX*2,nDiffY*2);
    }
    return;
}
//do PSNR and SSIM to check the image similarity.
void ZImgProcessThread::ZDoPSNRAndSSIM(const cv::Mat &mat1,const cv::Mat &mat2)
{
    ZAlgorithmSet  algorithm;
    //double pnsr=algorithm.ZGetPSNR(mat1Gray,mat2Gray);
    //SSIM适合2个内容完全一样，但模糊度，噪声水平等不同的图片进行相似度估计
    cv::Scalar ssim=algorithm.ZGetSSIM(mat1,mat2);
    //qDebug("pnsrt=%f\n",pnsr);
    //qDebug("MSSIM:%f,%f,%f\n",ssim[0],ssim[1],ssim[2]);
    double ssimSum=ssim[0]+ssim[1]+ssim[2];
    //emit this->ZSigSSIMImgSimilarity((qint32)(ssimSum*100)/3);
    emit this->ZSigSSIMImgSimilarity((qint32)(ssimSum*100));
}
void ZImgProcessThread::ZCalcImgHistogram(cv::Mat &img,cv::MatND &bHist,cv::MatND &gHist,cv::MatND &rHist)
{
    //split into RGB channels.
    std::vector<cv::Mat> RGBChannels;
    cv::split(img,RGBChannels);

    int histSize=255;
    float range[]={0,255};
    const float* histRange={range};

    //calc hist.
    bool bUniform=true;
    bool bAccumulate=false;
    cv::calcHist(&RGBChannels[0],1,0,cv::Mat(),bHist,1,&histSize,&histRange,bUniform,bAccumulate);
    cv::calcHist(&RGBChannels[1],1,0,cv::Mat(),gHist,1,&histSize,&histRange,bUniform,bAccumulate);
    cv::calcHist(&RGBChannels[2],1,0,cv::Mat(),rHist,1,&histSize,&histRange,bUniform,bAccumulate);

    //normalize [0,1].
    cv::normalize(rHist,rHist,0,1,cv::NORM_MINMAX,-1);
    cv::normalize(gHist,gHist,0,1,cv::NORM_MINMAX,-1);
    cv::normalize(bHist,bHist,0,1,cv::NORM_MINMAX,-1);
}
QImage ZImgProcessThread::ZGetRGBHistogram(const cv::Mat &mat)
{
    //create histogram.
    std::vector<cv::Mat> RGBMat;
    cv::split(mat,RGBMat);
    int histBinNum=255;
    float range[]={0,255};
    const float* histRange={range};

    cv::Mat redHist,greenHist,blueHist;
    //calculate histogram.
    cv::calcHist(&RGBMat[0],1,0,cv::Mat(),redHist,1,&histBinNum,&histRange,true,false);
    cv::calcHist(&RGBMat[1],1,0,cv::Mat(),greenHist,1,&histBinNum,&histRange,true,false);
    cv::calcHist(&RGBMat[2],1,0,cv::Mat(),blueHist,1,&histBinNum,&histRange,true,false);

    int histW=400;
    int histH=400;
    int binW=cvRound((double)mat.cols/histBinNum);
    cv::Mat histImg(mat.rows,mat.cols,CV_8UC3,cv::Scalar(0,0,0));
    //normalize [0,1].
    cv::normalize(redHist,redHist,0,histImg.rows,cv::NORM_MINMAX,-1,cv::Mat());
    cv::normalize(greenHist,greenHist,0,histImg.rows,cv::NORM_MINMAX,-1,cv::Mat());
    cv::normalize(blueHist,blueHist,0,histImg.rows,cv::NORM_MINMAX,-1,cv::Mat());

    for(qint32 i=0;i<histBinNum;i++)
    {
        cv::line(histImg,///<
                 cv::Point(binW*(i-1),mat.rows-cvRound(redHist.at<float>(i-1))),///<
                 cv::Point(binW*(i),mat.rows-cvRound(redHist.at<float>(i-1))),///<
                 cv::Scalar(0,0,255),2,8,0);
        cv::line(histImg,///<
                 cv::Point(binW*(i-1),mat.rows-cvRound(greenHist.at<float>(i-1))),///<
                 cv::Point(binW*(i),mat.rows-cvRound(greenHist.at<float>(i-1))),///<
                 cv::Scalar(0,255,0),2,8,0);
        cv::line(histImg,///<
                 cv::Point(binW*(i-1),mat.rows-cvRound(blueHist.at<float>(i-1))),///<
                 cv::Point(binW*(i),mat.rows-cvRound(blueHist.at<float>(i-1))),///<
                 cv::Scalar(255,0,0),2,8,0);
    }
    QImage imgHist=cvMat2QImage(histImg);
    return imgHist;
}
//method2. use Perceptual hash algorithm.
qint32 ZImgProcessThread::ZCalcPreHash(cv::Mat &mat1,cv::Mat &mat2)
{
    cv::Mat matDst1,matDst2;
    cv::resize(mat1,matDst1,cv::Size(8,8),0,0,cv::INTER_CUBIC);
    cv::resize(mat2,matDst2,cv::Size(8,8),0,0,cv::INTER_CUBIC);
    cv::cvtColor(matDst1,matDst1,CV_BGR2GRAY);
    cv::cvtColor(matDst2,matDst2,CV_BGR2GRAY);
    int iAVG1=0,iAVG2=0;
    int arr1[64],arr2[64];
    for(int i=0;i<8;i++)
    {
        uchar *data1=matDst1.ptr<uchar>(i);
        uchar *data2=matDst2.ptr<uchar>(i);
        int tmp=i*8;
        for(int j=0;j<8;j++)
        {
            int tmp1=tmp+j;
            arr1[tmp1]=data1[j]/4*4;
            arr2[tmp1]=data2[j]/4*4;

            iAVG1+=arr1[tmp1];
            iAVG2+=arr2[tmp1];
        }
    }
    iAVG1/=64;
    iAVG2/=64;

    for(int i=0;i<64;i++)
    {
        arr1[i]=(arr1[i]>=iAVG1)?1:0;
        arr2[i]=(arr2[i]>=iAVG2)?1:0;
    }

    int iDiffNum=0;
    for(int i=0;i<64;i++)
    {
        if(arr1[i]!=arr2[i])
        {
            iDiffNum++;
        }
    }
    return iDiffNum;
}

//method4.TemplateMatching.
//mat1:is the original image.
//mat2: is the template area.
cv::Mat ZImgProcessThread::ZDoTemplateMatch(const cv::Mat &matImg,const cv::Mat &matTemplate,qint32 nMatchMethod)
{
    cv::Mat matImgCopy;
    matImg.copyTo(matImgCopy);

    //calculate the result rectangle size.
    int nWidthResult=matImg.cols-matTemplate.cols+1;
    int nHeightResult=matImg.rows-matTemplate.rows+1;
    //create result rectangle.
    cv::Mat matResult;
    matResult.create(cv::Size(nWidthResult,nHeightResult),/*CV_32FC1_CV_8UC1*/matImg.type());

    //template matching.
    cv::matchTemplate(matImg,matTemplate,matResult,nMatchMethod);
    cv::normalize(matResult,matResult,0,1,cv::NORM_MINMAX,-1,cv::Mat());

    //localizing the best match with minMaxLoc.
    double fMinVal,fMaxVal;
    cv::Point ptMinLoc,ptMaxLoc,ptMatched;
    cv::minMaxLoc(matResult,&fMinVal,&fMaxVal,&ptMinLoc,&ptMaxLoc,cv::Mat());
    switch(nMatchMethod)
    {
    case CV_TM_SQDIFF_NORMED://标准平方差匹配，数据越小匹配度越好
    case CV_TM_SQDIFF:
        ptMatched=ptMinLoc;
        break;
    default:
        ptMatched=ptMaxLoc;
        break;
    }
    qDebug()<<"Match pt:"<<ptMatched.x<<","<<ptMatched.y;
    //draw a rectangle to track the template in original image.
    //在原始图像上画一个矩形标识匹配到的区域.
    cv::Rect rectMatched(ptMatched.x,ptMatched.y,matTemplate.cols,matTemplate.rows);
    cv::rectangle(matImgCopy,rectMatched,cv::Scalar(255,255,255),2,8,0);

#if 0
    //do mid-value filter.
    //summary all values.
    //sub the mimimum value,
    //sub the maximum value.
    //others do average calculate.
    if(this->m_tempMatchRectVector.size()>100)
    {
        this->m_tempMatchRectVector.removeFirst();
    }
    this->m_tempMatchRectVector.append(ptMatched);

    if(this->m_tempMatchRectVector.size()>10)
    {
        qint32 nMaxX,nMinX,nMaxY,nMinY;
        nMaxX=nMinX=this->m_tempMatchRectVector.at(0).x;
        nMaxY=nMinY=this->m_tempMatchRectVector.at(0).y;
        qint32 nSumX=0;
        qint32 nSumY=0;
        for(qint32 i=0;i<this->m_tempMatchRectVector.size();i++)
        {
            if(this->m_tempMatchRectVector.at(i).x>nMaxX)
            {
                nMaxX=this->m_tempMatchRectVector.at(i).x;
            }
            if(this->m_tempMatchRectVector.at(i).x<nMinX)
            {
                nMinX=this->m_tempMatchRectVector.at(i).x;
            }
            if(this->m_tempMatchRectVector.at(i).y>nMaxY)
            {
                nMaxY=this->m_tempMatchRectVector.at(i).y;
            }
            if(this->m_tempMatchRectVector.at(i).y<nMinY)
            {
                nMinY=this->m_tempMatchRectVector.at(i).y;
            }
            nSumX+=this->m_tempMatchRectVector.at(i).x;
            nSumY+=this->m_tempMatchRectVector.at(i).y;
        }
        qint32 nFilterX=(nSumX-nMaxX-nMinX)/(this->m_tempMatchRectVector.size()-2);
        qint32 nFilterY=(nSumY-nMaxY-nMinY)/(this->m_tempMatchRectVector.size()-2);
        cv::Rect rectFiler(nFilterX,nFilterY,matSmall.cols,matSmall.rows);
        //draw the match rectangle in the original image with red color.
        cv::rectangle(matOriginalWithRect,rectFiler,cv::Scalar(255,255,255),2,8,0);
    }
#endif
    return matImgCopy;
}
qint32 ZImgProcessThread::ZDoFeatureDetectMatch(const cv::Mat &matObj,const cv::Mat &matScene,///<
                                                cv::Mat &matObjKp,cv::Mat &matSceneKp,///<
                                                cv::Mat &matResult,cv::Rect rectObj)
{
    qint64 nStartMS,nEndMS;
    nStartMS=QDateTime::currentDateTime().toMSecsSinceEpoch();

    //make sure we only process the gray image.
    if(matObj.channels()!=1 || matScene.channels()!=1)
    {
        emit this->ZSigMsg(tr("do feature detect and match failed,input mat is not gray!"),Log_Msg_Error);
        return -1;
    }

    vector<KeyPoint> kP1;
    vector<KeyPoint> kP2;
    Mat mat1Desc,mat2Desc;
    vector<DMatch> matches;

    ZImgFeatureDetectMatch featureDetectMatch;
    //从源图像中找出特征点并存放在vector中,计算特征向量
    featureDetectMatch.detect_and_compute("akaze",matObj,kP1,mat1Desc);
    featureDetectMatch.detect_and_compute("akaze",matScene,kP2,mat2Desc);
    if(kP1.size()<=0)
    {
        qDebug()<<"0 feature points detected in matObj!";
        return -1;
    }
    if(kP2.size()<=0)
    {
        qDebug()<<"0 feature points detected in matScene!";
        return -1;
    }
    //把特征点描绘在图像上,用于中途处理过程的观察，工程优化要去掉
    cv::drawKeypoints(matObj, kP1, matObjKp, Scalar(0, 0, 255),DrawMatchesFlags::DEFAULT);
    cv::drawKeypoints(matScene, kP2, matSceneKp, Scalar(0, 0, 255), DrawMatchesFlags::DEFAULT);

    //执行向量匹配.
    featureDetectMatch.match("bf",mat1Desc,mat2Desc,matches);
    if(matches.size()<=0)
    {
        qDebug()<<"0 feature points matched in Image1 & Image2!";
        return -1;
    }
    //对匹配结果进行筛选,根据DMatch结构体中的float类型变量distance距离进行筛选找出最大值最小值.
    float minDistance=100;
    float maxDistance=0;
    for(int i=0;i<matches.size();i++)
    {
        if(matches[i].distance<minDistance)
        {
            minDistance=matches[i].distance;
        }
        if(matches[i].distance>maxDistance)
        {
            maxDistance=matches[i].distance;
        }
    }
    qDebug()<<"minDistance="<<minDistance<<",maxDistance="<<maxDistance;
    //我们认为距离小于2倍的最小距离是质量好的匹配点.
    std::vector<DMatch> goodMatches;
    for(unsigned int i=0;i<matches.size();i++)
    {
        if(matches[i].distance<2*minDistance)
        {
            goodMatches.push_back(matches[i]);
        }
    }
    if(goodMatches.size()<3)
    {
        qDebug()<<"less than 3 good matches!";
        return 0;
    }

    cv::drawMatches(matObj,kP1,matScene,kP2,goodMatches,matResult,Scalar::all(-1),Scalar::all(-1),vector<char>(),DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

    //localize the object.
    std::vector<Point2f> obj;
    std::vector<Point2f> scene;
    for(unsigned int i=0;i<goodMatches.size();i++)
    {
        //get keypoints from the good matches.
        obj.push_back(kP1[goodMatches[i].queryIdx].pt);
        scene.push_back(kP2[goodMatches[i].trainIdx].pt);
    }

    cv::Mat h=cv::findHomography(obj,scene,CV_RANSAC);
    if(h.empty())
    {
        qDebug()<<"empty mat!";
        return -1;
    }
    std::vector<Point2f> objCorner(4);
    objCorner[0]=cvPoint(0,0);
    objCorner[1]=cvPoint(matObj.cols,0);
    objCorner[2]=cvPoint(matObj.cols,matObj.rows);
    objCorner[3]=cvPoint(0,matObj.rows);

    std::vector<Point2f> sceneCorner(4);
    cv::perspectiveTransform(objCorner,sceneCorner,h);

    //draw lines.
    cv::line(matResult,sceneCorner[0]+Point2f(matObj.cols,0),sceneCorner[1]+Point2f(matObj.cols,0),Scalar(0,255,0),4);
    cv::line(matResult,sceneCorner[1]+Point2f(matObj.cols,0),sceneCorner[2]+Point2f(matObj.cols,0),Scalar(0,255,0),4);
    cv::line(matResult,sceneCorner[2]+Point2f(matObj.cols,0),sceneCorner[3]+Point2f(matObj.cols,0),Scalar(0,255,0),4);
    cv::line(matResult,sceneCorner[3]+Point2f(matObj.cols,0),sceneCorner[0]+Point2f(matObj.cols,0),Scalar(0,255,0),4);

    //find out the min&max ,min&max y to generate a new rectangle.
    qint32 nMinX,nMaxX,nMinY,nMaxY;
    nMinX=nMaxX=nMinY=nMaxY=0;
    for(qint32 i=0;i<4;i++)
    {
        if(sceneCorner[i].x<nMinX)
        {
            nMinX=sceneCorner[i].x;
        }
        if(sceneCorner[i].x>nMaxX)
        {
            nMaxX=sceneCorner[i].x;
        }
        if(sceneCorner[i].y<nMinY)
        {
            nMinY=sceneCorner[i].y;
        }
        if(sceneCorner[i].y>nMaxY)
        {
            nMaxY=sceneCorner[i].y;
        }
    }
    cv::rectangle(matSceneKp,cv::Rect(nMinX,nMinY,nMaxX-nMinX,nMaxY-nMinY),Scalar(255,0,0),4);
    cv::line(matSceneKp,sceneCorner[0],sceneCorner[1],Scalar(0,0,255),4);
    cv::line(matSceneKp,sceneCorner[1],sceneCorner[2],Scalar(0,0,255),4);
    cv::line(matSceneKp,sceneCorner[2],sceneCorner[3],Scalar(0,0,255),4);
    cv::line(matSceneKp,sceneCorner[3],sceneCorner[0],Scalar(0,0,255),4);
    //    ///
    //    vector<char> match_mask(matches.size(),1);
    //    featureDetectMatch.findKeyPointsHomography(kP1,kP2,/*matches*/goodMatches,match_mask);

    //    //draw the matched point & lines.
    //    cv::drawMatches(mat1,kP1,mat2,kP2,/*matches*/goodMatches,matResult,Scalar::all(-1),Scalar::all(-1), match_mask, DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);


    return 0;
}
//do gray histogram compare.
//return value 0%~100%.
qint32 ZImgProcessThread::ZDoHistogramCompare(const cv::Mat &mat1,const cv::Mat &mat2)
{
    //make sure the input is gray.single channel.
    //the input mat is gray.
    cv::Mat hist1,hist2;
    int histBinNum=255;
    float range[]={0,255};
    const float* histRange={range};

    //calculate histogram.
    cv::calcHist(&mat1,1,0,cv::Mat(),hist1,1,&histBinNum,&histRange,true,false);
    cv::calcHist(&mat2,1,0,cv::Mat(),hist2,1,&histBinNum,&histRange,true,false);
    cv::normalize(hist1,hist1,0,1,cv::NORM_MINMAX,-1,cv::Mat());
    cv::normalize(hist2,hist2,0,1,cv::NORM_MINMAX,-1,cv::Mat());

    double fCORREL=cv::compareHist(hist1,hist2,CV_COMP_CORREL);
    double fCHISQR=cv::compareHist(hist1,hist2,CV_COMP_CHISQR);
    double fINTERSECT=cv::compareHist(hist1,hist2,CV_COMP_INTERSECT);
    double fBHATTACHARYVA=cv::compareHist(hist1,hist2,CV_COMP_BHATTACHARYYA);
    //calculate preHash.
    //int preHash=this->ZCalcPreHash(matCut1,mat2);
    qDebug()<<fCORREL;

#if 0
    //这里只保持最近100次的值作为滤波基础.
    if(this->m_vecCORREL.size()>100)
    {
        this->m_vecCORREL.removeFirst();
    }
    this->m_vecCORREL.append(fCORREL);

    //中值滤波增加平滑性
    float fTempMax,fTempMin;
    float fTempSum=0.0f;
    fTempMax=fTempMin=this->m_vecCORREL.at(0);
    if(this->m_vecCORREL.size()>10)
    {
        for(qint32 i=0;i<this->m_vecCORREL.size();i++)
        {
            if(this->m_vecCORREL.at(i)>fTempMax)
            {
                fTempMax=this->m_vecCORREL.at(i);
            }else if(this->m_vecCORREL.at(i)<fTempMin)
            {
                fTempMin=this->m_vecCORREL.at(i);
            }
            fTempSum+=this->m_vecCORREL.at(i);
        }
        //get the mid-value average.
        //here *100,convert [0~1] to [0~100].
        float f1AVG=(fTempSum-fTempMax-fTempMin)/(this->m_vecCORREL.size()-2);
        fCORREL=f1AVG;
    }
#endif
    //qDebug("%f,%f,%f,%f,%d\n",fCORREL,fCHISQR,fINTERSECT,fBHATTACHARYVA,0);
    return (qint32)(fCORREL*100);
}
//Soduko 3x3.
cv::Mat ZImgProcessThread::ZDoSoduKoHistogramCompare(const cv::Mat &matBig,const cv::Mat &matSmall,cv::Rect &rectROI)
{
    //将图像1按九宫格切分作直方图对比
    qint32 nSplitW=matBig.cols/3;
    qint32 nSplitH=matBig.rows/3;
    cv::Rect rect00(0,0,nSplitW,nSplitH);
    cv::Rect rect01(nSplitW,0,nSplitW,nSplitH);
    cv::Rect rect02(nSplitW*2,0,nSplitW,nSplitH);
    cv::Rect rect10(0,nSplitH,nSplitW,nSplitH);
    cv::Rect rect11(nSplitW,nSplitH,nSplitW,nSplitH);
    cv::Rect rect12(nSplitW*2,nSplitH,nSplitW,nSplitH);
    cv::Rect rect20(0,nSplitH*2,nSplitW,nSplitH);
    cv::Rect rect21(nSplitW,nSplitH*2,nSplitW,nSplitH);
    cv::Rect rect22(nSplitW*2,nSplitH*2,nSplitW,nSplitH);
    //3x3=9.
    cv::Mat matBigCut3x3_1(matBig,rect00);
    cv::Mat matBigCut3x3_2(matBig,rect01);
    cv::Mat matBigCut3x3_3(matBig,rect02);

    cv::Mat matBigCut3x3_4(matBig,rect10);
    cv::Mat matBigCut3x3_5(matBig,rect11);
    cv::Mat matBigCut3x3_6(matBig,rect12);

    cv::Mat matBigCut3x3_7(matBig,rect20);
    cv::Mat matBigCut3x3_8(matBig,rect21);
    cv::Mat matBigCut3x3_9(matBig,rect22);

    //这里我们收集所有切割图像的相似度，然后找出匹配度最好的那个幅图像
    qint32 nImgSimilarity[9];
    nImgSimilarity[0]=this->ZDoHistogramCompare(matBigCut3x3_1,matSmall);
    nImgSimilarity[1]=this->ZDoHistogramCompare(matBigCut3x3_2,matSmall);
    nImgSimilarity[2]=this->ZDoHistogramCompare(matBigCut3x3_3,matSmall);

    nImgSimilarity[3]=this->ZDoHistogramCompare(matBigCut3x3_4,matSmall);
    nImgSimilarity[4]=this->ZDoHistogramCompare(matBigCut3x3_5,matSmall);
    nImgSimilarity[5]=this->ZDoHistogramCompare(matBigCut3x3_6,matSmall);

    nImgSimilarity[6]=this->ZDoHistogramCompare(matBigCut3x3_7,matSmall);
    nImgSimilarity[7]=this->ZDoHistogramCompare(matBigCut3x3_8,matSmall);
    nImgSimilarity[8]=this->ZDoHistogramCompare(matBigCut3x3_9,matSmall);

    //qDebug()<<"histcomp:"<<nImgSimilarity[0]<<","<<nImgSimilarity[1]<<","<<nImgSimilarity[2]<<","<<nImgSimilarity[3]<<","<<nImgSimilarity[4]<<","<<nImgSimilarity[5]<<","<<nImgSimilarity[6]<<","<<nImgSimilarity[7]<<","<<nImgSimilarity[8];

    //find out the maximum similarity value.
    qint32 nMaxVal=0;
    for(qint32 i=0;i<9;i++)
    {
        if(nImgSimilarity[i]>nMaxVal)
        {
            nMaxVal=nImgSimilarity[i];
        }
    }
    //qDebug()<<"max is:"<<nMaxVal;
    //emit this->ZSigSudokuImgSimilarity(nMaxVal);

    if(nMaxVal==nImgSimilarity[0])
    {
        rectROI=rect00;
        return matBigCut3x3_1;
    }else if(nMaxVal==nImgSimilarity[1])
    {
        rectROI=rect01;
        return matBigCut3x3_2;
    }else if(nMaxVal==nImgSimilarity[2])
    {
        rectROI=rect02;
        return matBigCut3x3_3;
    }else if(nMaxVal==nImgSimilarity[3])
    {
        rectROI=rect10;
        return matBigCut3x3_4;
    }else if(nMaxVal==nImgSimilarity[4])
    {
        rectROI=rect11;
        return matBigCut3x3_5;
    }else if(nMaxVal==nImgSimilarity[5])
    {
        rectROI=rect12;
        return matBigCut3x3_6;
    }else if(nMaxVal==nImgSimilarity[6])
    {
        rectROI=rect20;
        return matBigCut3x3_7;
    }else if(nMaxVal==nImgSimilarity[7])
    {
        rectROI=rect21;
        return matBigCut3x3_8;
    }else if(nMaxVal==nImgSimilarity[8])
    {
        rectROI=rect22;
        return matBigCut3x3_9;
    }else{
        //return the middle img.
        rectROI=rect11;
        return matBigCut3x3_5;
    }
}
