#include "zvideotask.h"
#include <video/zfiltecamdev.h>
#include <QStringList>
#include <QDebug>
#include <QDateTime>
#include <QPainter>
#include <QApplication>
ZVideoTask::ZVideoTask(QObject *parent) : QObject(parent)
{
    this->m_cap1=NULL;//主摄像头Main图像采集线程.
    this->m_cap2=NULL;//辅摄像头Aux图像采集线程.
    this->m_process=NULL;//图像处理线程.
    this->m_h264Thread=NULL;//h264图像编码线程.
    this->m_videoTxThread=NULL;//图像的TCP传输线程.
    this->m_tcp2Uart=NULL;//串口透传线程Android(tcp) <--> STM32(uart).

    //Main CaptureThread put QImage to this queue.
    //Main View Display UI get QImage from this queue for local display.
    this->m_rbDispMain=NULL;

    //Aux CaptureThread put QImage to this queue.
    //Aux View Display UI get QImage from this queue for local display.
    this->m_rbDispAux=NULL;

    //Main CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    this->m_rbProcessMain=NULL;

    //Aux CaptureThread put QImage to this queue.
    //ImgProcessThread get QImage from this queue.
    this->m_rbProcessAux=NULL;

    //Main CaptureThread put yuv to this queue.
    //H264 EncodeThread get yuv from this queue.
    this->m_rbYUV=NULL;

    //H264EncThread put h264 frame to this queue.
    //TcpTxThread get data from this queue.
    this->m_rbH264=NULL;

    //ImgProcessThread put data to this queue.
    //Main UI get data from this queue.
    this->m_queueProcessedSet=NULL;
    this->m_semaProcessedSetUsed=NULL;
    this->m_semaProcessedSetFree=NULL;
}
ZVideoTask::~ZVideoTask()
{
    if(this->m_process)
    {
        this->m_process->ZStopThread();
        this->m_process->wait(1000*10);//10s.
        delete this->m_process;
    }

    if(this->m_cap1)
    {
        this->m_cap1->ZStopThread();
        this->m_cap1->wait(1000*10);//10s.
        delete this->m_cap1;
    }

    if(this->m_cap2)
    {
        this->m_cap2->ZStopThread();
        this->m_cap2->wait(1000*10);//10s.
        delete this->m_cap2;
    }
}
qint32 ZVideoTask::ZBindMainDispQueue(ZRingBuffer *rbDispMain)
{
    this->m_rbDispMain=rbDispMain;
    return 0;
}
qint32 ZVideoTask::ZBindAuxDispQueue(ZRingBuffer *rbDispAux)
{
    this->m_rbDispAux=rbDispAux;
    return 0;
}
qint32 ZVideoTask::ZBindImgProcessedSet(QQueue<ZImgProcessedSet> *queue,QSemaphore *semaUsed,QSemaphore *semaFree)
{
    this->m_queueProcessedSet=queue;
    this->m_semaProcessedSetUsed=semaUsed;
    this->m_semaProcessedSetFree=semaFree;
    return 0;
}
qint32 ZVideoTask::ZDoInit()
{

    ZFilteCAMDev filteCAMDev;
    QStringList lstRealDev=filteCAMDev.ZGetCAMDevList();
    if(lstRealDev.size()<2)
    {
        qDebug()<<"<error>:failed to find 2 cameras.";
        return -1;
    }

    //start cap thread.
    QString cam1Device("/dev/"+lstRealDev.at(0));
    QString cam2Device("/dev/"+lstRealDev.at(1));
    if(gGblPara.m_bVerbose)
    {
        QString msgVerbose=QString("camera 1=%1,camera 2=%2").arg(cam1Device).arg(cam2Device);
        this->ZSlotMsg(msgVerbose,Log_Msg_Info);
    }

    /////////////////////////////the main camera queue///////////////////////////
    //main capture thread to img process queue.
    this->m_rbProcessMain=new ZRingBuffer(30,640*480*3*2);
    //main capture thread to yuv queue.
    this->m_rbYUV=new ZRingBuffer(30,640*480*3*2);
    ////////////////////////////////the aux camera queue///////////////////////////
    //aux capture thread to img process queue.
    this->m_rbProcessAux=new ZRingBuffer(30,640*480*3*2);
    ///////////////////////////////*h264 encode result queue/////////////////////////////
    this->m_rbH264=new ZRingBuffer(30,640*480*3*2);

    //create capture thread.
    this->m_cap1=new ZImgCapThread(cam1Device,gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,true);//Main Camera.
    this->m_cap1->ZBindDispQueue(this->m_rbDispMain);
    this->m_cap1->ZBindProcessQueue(this->m_rbProcessMain);
    this->m_cap1->ZBindYUVQueue(this->m_rbYUV);

    this->m_cap2=new ZImgCapThread(cam2Device,gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,false);
    this->m_cap2->ZBindDispQueue(this->m_rbDispAux);
    this->m_cap2->ZBindProcessQueue(this->m_rbProcessAux);

    //create h264 encode thread.
    this->m_h264Thread=new ZH264EncThread;
    this->m_h264Thread->ZBindYUVQueue(this->m_rbYUV);
    this->m_h264Thread->ZBindH264Queue(this->m_rbH264);

    //create image process thread.
    this->m_process=new ZImgProcessThread;
    this->m_process->ZBindMainQueue(this->m_rbProcessMain);
    this->m_process->ZBindAuxQueue(this->m_rbProcessAux);
    this->m_process->ZBindProcessedSetQueue(this->m_queueProcessedSet,this->m_semaProcessedSetUsed,this->m_semaProcessedSetFree);

    //start tcp server.
    this->m_videoTxThread=new ZVideoTxThread;
    this->m_videoTxThread->ZBindQueue(this->m_rbH264);
    return 0;
}
ZImgCapThread* ZVideoTask::ZGetImgCapThread(qint32 index)
{
    ZImgCapThread *capThread=NULL;
    switch(index)
    {
    case 0:
        capThread=this->m_cap1;
        break;
    case 1:
        capThread=this->m_cap2;
        break;
    default:
        break;
    }
    return capThread;
}
ZImgProcessThread* ZVideoTask::ZGetImgProcessThread()
{
    return this->m_process;
}
qint32 ZVideoTask::ZStartTask()
{

    this->m_videoTxThread->ZStartThread();

    //start h264 thread.
    this->m_h264Thread->ZStartThread();

    //start img process thread.
    this->m_process->ZStartThread();

    //start capture thread.
    this->m_cap1->ZStartThread();
    this->m_cap2->ZStartThread();

    return 0;
#if 0
    this->m_process=new ZImgProcessThread;
    QString cap1ID=this->m_cap1->ZGetCAMID();
    QString cap2ID=this->m_cap2->ZGetCAMID();
    if(cap1ID==gGblPara.m_idCAM1 && cap2ID==gGblPara.m_idCAM2)
    {
        this->m_disp[0]->ZSetCAMParameters(gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,cap1ID);//main camera.
        this->m_disp[1]->ZSetCAMParameters(gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,cap2ID);

        if(gGblPara.m_bFMode)
        {
            //F mode.
            QObject::connect(this->m_process,SIGNAL(ZSigObjFeatureKeyPoints(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_process,SIGNAL(ZSigSceneFeatureKeyPoints(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }else{
            //X mode.
            QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }

        QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }else if(cap1ID==gGblPara.m_idCAM2 && cap2ID==gGblPara.m_idCAM1){

        this->m_disp[0]->ZSetCAMParameters(gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,cap2ID);//main camera.
        this->m_disp[1]->ZSetCAMParameters(gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,cap1ID);

        if(gGblPara.m_bFMode)
        {
            //F mode.
            QObject::connect(this->m_process,SIGNAL(ZSigSceneFeatureKeyPoints(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_process,SIGNAL(ZSigObjFeatureKeyPoints(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }else{
            //X mode.
            QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }

        QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }else{ //anyway here.

        this->m_disp[0]->ZSetCAMParameters(gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,cap1ID);//main camera.
        this->m_disp[1]->ZSetCAMParameters(gGblPara.m_widthCAM2,gGblPara.m_heightCAM2,gGblPara.m_fpsCAM2,cap2ID);

        if(gGblPara.m_bFMode)
        {
            //F mode.
            QObject::connect(this->m_process,SIGNAL(ZSigObjFeatureKeyPoints(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_process,SIGNAL(ZSigSceneFeatureKeyPoints(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }else{
            //X mode.
            QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_disp[0],SLOT(ZSlotDispImg(QImage)));
            QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_disp[1],SLOT(ZSlotDispImg(QImage)));
        }

        QObject::connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        QObject::connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }

    //safety thread exit mechanism.
    connect(this->m_cap1,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));
    connect(this->m_cap2,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));
    connect(this->m_process,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));

    //display all log messages on the main windows's text edit.
    connect(this->m_cap1,SIGNAL(ZSigMsg(QString,qint32)),this,SLOT(ZSlotMsg(QString,qint32)));
    connect(this->m_cap2,SIGNAL(ZSigMsg(QString,qint32)),this,SLOT(ZSlotMsg(QString,qint32)));
    connect(this->m_process,SIGNAL(ZSigMsg(QString,qint32)),this,SLOT(ZSlotMsg(QString,qint32)));


    connect(this->m_process,SIGNAL(ZSigDiffXYT(QRect,QRect,qint32,qint32,qint32)),this,SLOT(ZSlotDiffXYT(QRect,QRect,qint32,qint32,qint32)));
    connect(this->m_process,SIGNAL(ZSigSSIMImgSimilarity(qint32)),this,SLOT(ZSlotSSIMImgSimilarity(qint32)));
#if 0
    //we make sure the image1 is bigger than image2.
    qint32 nImage1Size=this->m_cap1->ZGetCAMImgWidth()*this->m_cap1->ZGetCAMImgHeight();
    qint32 nImage2Size=this->m_cap2->ZGetCAMImgWidth()*this->m_cap2->ZGetCAMImgHeight();
    if(nImage1Size>=nImage2Size)
    {
        this->ZSlotMsg(tr("%1 > %2 , use %1 as main camera.")///<
                       .arg(this->m_cap1->ZGetDevName())//<
                       .arg(this->m_cap2->ZGetDevName())///<
                       .arg(this->m_cap1->ZGetDevName()),Log_Msg_Info);

        //Capture thread to ImgProcess thread vector.
        connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }else{
        this->ZSlotMsg(tr("%1 > %2 , use %1 as main camera.")///<
                       .arg(this->m_cap2->ZGetDevName())///<
                       .arg(this->m_cap1->ZGetDevName())///<
                       .arg(this->m_cap2->ZGetDevName()),Log_Msg_Info);
        //Capture thread to ImgProcess thread vector.
        connect(this->m_cap2,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg1(QImage)));
        connect(this->m_cap1,SIGNAL(ZSigCapImg(QImage)),this->m_process,SLOT(ZSlotGetImg2(QImage)));
    }
#endif

    //transfer video to pc through UDT.
    if(gGblPara.m_bTransfer2PC)
    {
        this->m_video2PC=new ZServerThread(this->m_process);
        connect(this->m_video2PC,SIGNAL(ZSigThreadFinished()),this,SLOT(ZSlotSubThreadFinished()));
        this->m_video2PC->ZStartThread();
    }

    //start img process thread.
    this->m_process->ZStartThread();

    //start img capture thread.
    this->m_cap1->ZStartThread();
    this->m_cap2->ZStartThread();
#endif
}

void ZVideoTask::ZSlotMsg(const QString &msg,const qint32 &type)
{
    switch(type)
    {
    case Log_Msg_Info:
        qDebug()<<"<info>:"<<msg;
        break;
    case Log_Msg_Warning:
        qDebug()<<"<warning>:"<<msg;
        break;
    case Log_Msg_Error:
        qDebug()<<"<error>:"<<msg;
        break;
    default:
        qDebug()<<"<unknown>:"<<msg;
        break;
    }
}
