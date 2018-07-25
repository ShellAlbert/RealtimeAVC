#include "zvideotask.h"
#include <video/zfiltecamdev.h>
#include <QStringList>
#include <QDebug>
#include <QDateTime>
#include <QPainter>
#include <QApplication>
#include <QDir>
#include <sys/stat.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <linux/videodev2.h>
ZVideoTask::ZVideoTask(QObject *parent) : QObject(parent)
{
    for(qint32 i=0;i<2;i++)
    {
        this->m_rbProcess[i]=NULL;
        this->m_rbYUV[i]=NULL;
        this->m_rbH264[i]=NULL;

        this->m_capThread[i]=NULL;
        this->m_h264Thread[i]=NULL;
        this->m_videoTxThread[i]=NULL;//图像的TCP传输线程.
    }

    this->m_process=NULL;//图像处理线程.
    this->m_tcp2Uart=NULL;//串口透传线程Android(tcp) <--> STM32(uart).
}
ZVideoTask::~ZVideoTask()
{
    if(this->m_process)
    {
        this->m_process->ZStopThread();
        this->m_process->wait(1000*10);//10s.
        delete this->m_process;
    }

    for(qint32 i=0;i<2;i++)
    {
        this->m_capThread[i]->ZStopThread();
        this->m_capThread[i]->wait(1000*10);
        delete this->m_capThread[i];

        this->m_h264Thread[i]->ZStopThread();
        this->m_h264Thread[i]->wait(1000*10);
        delete this->m_h264Thread[i];

        this->m_videoTxThread[i]->ZStopThread();
        this->m_videoTxThread[i]->wait(1000*10);
        delete this->m_videoTxThread[i];

        delete [] this->m_rbProcess[i];
        delete [] this->m_rbYUV[i];
        delete [] this->m_rbH264[i];
    }
}
qint32 ZVideoTask::ZDoInit()
{

    //需要排除的设备节点列表.
    QStringList extractNode;
    //    extractNode.append("video0");
    //    extractNode.append("video1");
    //    extractNode.append("video16");
    //    extractNode.append("video17");

    //列出/dev目录下所有的videoX设备节点.
    QStringList nodeNameList;
    QDir dir("/dev");
    QStringList fileList=dir.entryList(QDir::System);
    for(qint32 i=0;i<fileList.size();i++)
    {
        QString nodeName=fileList.at(i);
        if(nodeName.startsWith("video"))
        {
            if(!extractNode.contains(nodeName))
            {
                nodeNameList.append(nodeName);
            }
        }
    }

    if(nodeNameList.size()<2)
    {
        qDebug()<<"<Error>:No 2 cameras found at least.";
        return -1;
    }

    //比对配置文件ini设置的CamID来决定哪一个是主摄像头，哪一个是辅摄像头.
    QString mainCamera,auxCamera;
    if(gGblPara.m_video.m_bDoNotCmpCamId)
    {
        mainCamera="/dev/"+nodeNameList.at(0);
        auxCamera="/dev/"+nodeNameList.at(1);
    }else
    {
        for(qint32 i=0;i<nodeNameList.size();i++)
        {
            struct v4l2_capability cap;
            QString nodeDev="/dev/"+nodeNameList.at(i);
            int fd=open(nodeDev.toStdString().c_str(),O_RDWR);
            if(ioctl(fd,VIDIOC_QUERYCAP,&cap)<0)
            {
                qDebug()<<"<Error>:failed to query capability "<<nodeDev;
                return -1;
            }
            //qDebug()<<nodeDev<<","<<QString((char*)cap.bus_info);
            //qDebug()<<"cam1:"<<gGblPara.m_video.m_Cam1ID<<",cam2:"<<gGblPara.m_video.m_Cam2ID;
            if(QString((char*)cap.bus_info)==gGblPara.m_video.m_Cam1ID)
            {
                mainCamera=nodeDev;
            }else if(QString((char*)cap.bus_info)==gGblPara.m_video.m_Cam2ID)
            {
                auxCamera=nodeDev;
            }
            close(fd);
        }
    }
    if(mainCamera.isEmpty() || auxCamera.isEmpty())
    {
        qDebug()<<"<Error>:can not find matched cam id (bus_info) for main/aux camera.";
        return -1;
    }

    for(qint32 i=0;i<2;i++)
    {
        //main/aux capture thread to img process thread queue.
        this->m_rbProcess[i]=new ZRingBuffer(30,gGblPara.m_widthCAM1*gGblPara.m_heightCAM1*3*2);
        //main/aux cap thread to h264 enc thread.
        this->m_rbYUV[i]=new ZRingBuffer(30,gGblPara.m_widthCAM1*gGblPara.m_heightCAM1*3*2);
        //h264 enc thread to video tx thread.
        this->m_rbH264[i]=new ZRingBuffer(30,gGblPara.m_widthCAM1*gGblPara.m_heightCAM1*3*2);

        //create capture thread.
        if(i==0)
        {
            this->m_capThread[i]=new ZImgCapThread(mainCamera,gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1,true);//Main Camera.
        }else{
            this->m_capThread[i]=new ZImgCapThread(auxCamera,gGblPara.m_widthCAM1,gGblPara.m_heightCAM1,gGblPara.m_fpsCAM1);//Aux Camera.
        }
        this->m_capThread[i]->ZBindProcessQueue(this->m_rbProcess[i]);
        this->m_capThread[i]->ZBindYUVQueue(this->m_rbYUV[i]);

        //create h264 encode thread.
        this->m_h264Thread[i]=new ZH264EncThread;
        this->m_h264Thread[i]->ZBindYUVQueue(this->m_rbYUV[i]);
        this->m_h264Thread[i]->ZBindH264Queue(this->m_rbH264[i]);

        //create video tx thread.
        if(0==i)
        {
            this->m_videoTxThread[i]=new ZVideoTxThread(TCP_PORT_VIDEO);
        }else{
            this->m_videoTxThread[i]=new ZVideoTxThread(TCP_PORT_VIDEO2);
        }
        this->m_videoTxThread[i]->ZBindQueue(this->m_rbH264[i]);
    }
    //create image process thread.
    this->m_process=new ZImgProcessThread;
    this->m_process->ZBindMainAuxImgQueue(this->m_rbProcess[0],this->m_rbProcess[1]);
    return 0;
}
ZImgCapThread* ZVideoTask::ZGetImgCapThread(qint32 index)
{
    ZImgCapThread *capThread=NULL;
    switch(index)
    {
    case 0:
        capThread=this->m_capThread[0];
        break;
    case 1:
        capThread=this->m_capThread[1];
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

    for(qint32 i=0;i<2;i++)
    {
        this->m_videoTxThread[i]->ZStartThread();
        this->m_h264Thread[i]->ZStartThread();
        this->m_capThread[i]->ZStartThread();
    }
    this->m_process->ZStartThread();
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
