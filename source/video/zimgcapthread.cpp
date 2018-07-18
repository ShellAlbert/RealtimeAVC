#include "zimgcapthread.h"
#include <QDateTime>
#include <QDebug>
short redAdjust[] = {
    -161,-160,-159,-158,-157,-156,-155,-153,
    -152,-151,-150,-149,-148,-147,-145,-144,
    -143,-142,-141,-140,-139,-137,-136,-135,
    -134,-133,-132,-131,-129,-128,-127,-126,
    -125,-124,-123,-122,-120,-119,-118,-117,
    -116,-115,-114,-112,-111,-110,-109,-108,
    -107,-106,-104,-103,-102,-101,-100, -99,
    -98, -96, -95, -94, -93, -92, -91, -90,
    -88, -87, -86, -85, -84, -83, -82, -80,
    -79, -78, -77, -76, -75, -74, -72, -71,
    -70, -69, -68, -67, -66, -65, -63, -62,
    -61, -60, -59, -58, -57, -55, -54, -53,
    -52, -51, -50, -49, -47, -46, -45, -44,
    -43, -42, -41, -39, -38, -37, -36, -35,
    -34, -33, -31, -30, -29, -28, -27, -26,
    -25, -23, -22, -21, -20, -19, -18, -17,
    -16, -14, -13, -12, -11, -10,  -9,  -8,
    -6,  -5,  -4,  -3,  -2,  -1,   0,   1,
    2,   3,   4,   5,   6,   7,   9,  10,
    11,  12,  13,  14,  15,  17,  18,  19,
    20,  21,  22,  23,  25,  26,  27,  28,
    29,  30,  31,  33,  34,  35,  36,  37,
    38,  39,  40,  42,  43,  44,  45,  46,
    47,  48,  50,  51,  52,  53,  54,  55,
    56,  58,  59,  60,  61,  62,  63,  64,
    66,  67,  68,  69,  70,  71,  72,  74,
    75,  76,  77,  78,  79,  80,  82,  83,
    84,  85,  86,  87,  88,  90,  91,  92,
    93,  94,  95,  96,  97,  99, 100, 101,
    102, 103, 104, 105, 107, 108, 109, 110,
    111, 112, 113, 115, 116, 117, 118, 119,
    120, 121, 123, 124, 125, 126, 127, 128,
};

short greenAdjust1[] = {
    34,  34,  33,  33,  32,  32,  32,  31,
    31,  30,  30,  30,  29,  29,  28,  28,
    28,  27,  27,  27,  26,  26,  25,  25,
    25,  24,  24,  23,  23,  23,  22,  22,
    21,  21,  21,  20,  20,  19,  19,  19,
    18,  18,  17,  17,  17,  16,  16,  15,
    15,  15,  14,  14,  13,  13,  13,  12,
    12,  12,  11,  11,  10,  10,  10,   9,
    9,   8,   8,   8,   7,   7,   6,   6,
    6,   5,   5,   4,   4,   4,   3,   3,
    2,   2,   2,   1,   1,   0,   0,   0,
    0,   0,  -1,  -1,  -1,  -2,  -2,  -2,
    -3,  -3,  -4,  -4,  -4,  -5,  -5,  -6,
    -6,  -6,  -7,  -7,  -8,  -8,  -8,  -9,
    -9, -10, -10, -10, -11, -11, -12, -12,
    -12, -13, -13, -14, -14, -14, -15, -15,
    -16, -16, -16, -17, -17, -17, -18, -18,
    -19, -19, -19, -20, -20, -21, -21, -21,
    -22, -22, -23, -23, -23, -24, -24, -25,
    -25, -25, -26, -26, -27, -27, -27, -28,
    -28, -29, -29, -29, -30, -30, -30, -31,
    -31, -32, -32, -32, -33, -33, -34, -34,
    -34, -35, -35, -36, -36, -36, -37, -37,
    -38, -38, -38, -39, -39, -40, -40, -40,
    -41, -41, -42, -42, -42, -43, -43, -44,
    -44, -44, -45, -45, -45, -46, -46, -47,
    -47, -47, -48, -48, -49, -49, -49, -50,
    -50, -51, -51, -51, -52, -52, -53, -53,
    -53, -54, -54, -55, -55, -55, -56, -56,
    -57, -57, -57, -58, -58, -59, -59, -59,
    -60, -60, -60, -61, -61, -62, -62, -62,
    -63, -63, -64, -64, -64, -65, -65, -66,
};

short greenAdjust2[] = {
    74,  73,  73,  72,  71,  71,  70,  70,
    69,  69,  68,  67,  67,  66,  66,  65,
    65,  64,  63,  63,  62,  62,  61,  60,
    60,  59,  59,  58,  58,  57,  56,  56,
    55,  55,  54,  53,  53,  52,  52,  51,
    51,  50,  49,  49,  48,  48,  47,  47,
    46,  45,  45,  44,  44,  43,  42,  42,
    41,  41,  40,  40,  39,  38,  38,  37,
    37,  36,  35,  35,  34,  34,  33,  33,
    32,  31,  31,  30,  30,  29,  29,  28,
    27,  27,  26,  26,  25,  24,  24,  23,
    23,  22,  22,  21,  20,  20,  19,  19,
    18,  17,  17,  16,  16,  15,  15,  14,
    13,  13,  12,  12,  11,  11,  10,   9,
    9,   8,   8,   7,   6,   6,   5,   5,
    4,   4,   3,   2,   2,   1,   1,   0,
    0,   0,  -1,  -1,  -2,  -2,  -3,  -4,
    -4,  -5,  -5,  -6,  -6,  -7,  -8,  -8,
    -9,  -9, -10, -11, -11, -12, -12, -13,
    -13, -14, -15, -15, -16, -16, -17, -17,
    -18, -19, -19, -20, -20, -21, -22, -22,
    -23, -23, -24, -24, -25, -26, -26, -27,
    -27, -28, -29, -29, -30, -30, -31, -31,
    -32, -33, -33, -34, -34, -35, -35, -36,
    -37, -37, -38, -38, -39, -40, -40, -41,
    -41, -42, -42, -43, -44, -44, -45, -45,
    -46, -47, -47, -48, -48, -49, -49, -50,
    -51, -51, -52, -52, -53, -53, -54, -55,
    -55, -56, -56, -57, -58, -58, -59, -59,
    -60, -60, -61, -62, -62, -63, -63, -64,
    -65, -65, -66, -66, -67, -67, -68, -69,
    -69, -70, -70, -71, -71, -72, -73, -73,
};

short blueAdjust[] = {
    -276,-274,-272,-270,-267,-265,-263,-261,
    -259,-257,-255,-253,-251,-249,-247,-245,
    -243,-241,-239,-237,-235,-233,-231,-229,
    -227,-225,-223,-221,-219,-217,-215,-213,
    -211,-209,-207,-204,-202,-200,-198,-196,
    -194,-192,-190,-188,-186,-184,-182,-180,
    -178,-176,-174,-172,-170,-168,-166,-164,
    -162,-160,-158,-156,-154,-152,-150,-148,
    -146,-144,-141,-139,-137,-135,-133,-131,
    -129,-127,-125,-123,-121,-119,-117,-115,
    -113,-111,-109,-107,-105,-103,-101, -99,
    -97, -95, -93, -91, -89, -87, -85, -83,
    -81, -78, -76, -74, -72, -70, -68, -66,
    -64, -62, -60, -58, -56, -54, -52, -50,
    -48, -46, -44, -42, -40, -38, -36, -34,
    -32, -30, -28, -26, -24, -22, -20, -18,
    -16, -13, -11,  -9,  -7,  -5,  -3,  -1,
    0,   2,   4,   6,   8,  10,  12,  14,
    16,  18,  20,  22,  24,  26,  28,  30,
    32,  34,  36,  38,  40,  42,  44,  46,
    49,  51,  53,  55,  57,  59,  61,  63,
    65,  67,  69,  71,  73,  75,  77,  79,
    81,  83,  85,  87,  89,  91,  93,  95,
    97,  99, 101, 103, 105, 107, 109, 112,
    114, 116, 118, 120, 122, 124, 126, 128,
    130, 132, 134, 136, 138, 140, 142, 144,
    146, 148, 150, 152, 154, 156, 158, 160,
    162, 164, 166, 168, 170, 172, 175, 177,
    179, 181, 183, 185, 187, 189, 191, 193,
    195, 197, 199, 201, 203, 205, 207, 209,
    211, 213, 215, 217, 219, 221, 223, 225,
    227, 229, 231, 233, 235, 238, 240, 242,
};
//判断范围
unsigned char clip(int val)
{
    if(val > 255)
    {
        return 255;
    }
    else if(val > 0)
    {
        return val;
    }
    else
    {
        return 0;
    }
}
//查表法YUV TO RGB
int YUYVToRGB_table(unsigned char *yuv, unsigned char *rgb, unsigned int width,unsigned int height)
{
    //YU YV YU YV .....
    short y1=0, y2=0, u=0, v=0;
    unsigned char *pYUV = yuv;
    unsigned char *pGRB = rgb;
    int i=0;
    //int y=0,x=0,in=0,y0,out=0;
    int count =width*height/2;
    for(i = 0; i < count; i++)
    {
        y1 = *pYUV++ ;
        u  = *pYUV++ ;
        y2 = *pYUV++ ;
        v  = *pYUV++ ;

        *pGRB++ = clip(y1 + redAdjust[v]);
        *pGRB++ = clip(y1 + greenAdjust1[u] + greenAdjust2[v]);
        *pGRB++ = clip(y1 + blueAdjust[u]);
        *pGRB++ = clip(y2 + redAdjust[v]);
        *pGRB++ = clip(y2 + greenAdjust1[u] + greenAdjust2[v]);
        *pGRB++ = clip(y2 + blueAdjust[u]);
    }
    return 0;
}
ZImgCapThread::ZImgCapThread(QString devNodeName,qint32 nPreWidth,qint32 nPreHeight,qint32 nPreFps,bool bMainCamera)
{
    this->m_devName=devNodeName;
    this->m_nPreWidth=nPreWidth;
    this->m_nPreHeight=nPreHeight;
    this->m_nPreFps=nPreFps;
    this->m_bMainCamera=bMainCamera;

    //capture image to local display queue.
    this->m_queueDisp=NULL;
     this->m_semaDispUsed=NULL;
     this->m_semaDispFree=NULL;
    //capture image to process queue.
     this->m_queueProcess=NULL;
     this->m_semaProcessUsed=NULL;
     this->m_semaProcessFree=NULL;
    //capture yuv to yuv queue.
     this->m_queueYUV=NULL;
     this->m_semaYUVUsed=NULL;
     this->m_semaYUVFree=NULL;

    this->m_bExitFlag=false;
}
ZImgCapThread::~ZImgCapThread()
{

}
qint32 ZImgCapThread::ZBindDispQueue(QQueue<QImage> *queueDisp,QSemaphore *semaDispUsed,QSemaphore *semaDispFree)
{
    this->m_queueDisp=queueDisp;
    this->m_semaDispUsed=semaDispUsed;
    this->m_semaDispFree=semaDispFree;
    return 0;
}
qint32 ZImgCapThread::ZBindProcessQueue(QQueue<QImage> *queueProcess,QSemaphore *semaProcessUsed,QSemaphore *semaProcessFree)
{
    this->m_queueProcess=queueProcess;
    this->m_semaProcessUsed=semaProcessUsed;
    this->m_semaProcessFree=semaProcessFree;
    return 0;
}
qint32 ZImgCapThread::ZBindYUVQueue(QQueue<QByteArray> *queueYUV,QSemaphore *semaYUVUsed,QSemaphore *semaYUVFree)
{
    this->m_queueYUV=queueYUV;
    this->m_semaYUVUsed=semaYUVUsed;
    this->m_semaYUVFree=semaYUVFree;
    return 0;
}
qint32 ZImgCapThread::ZStartThread()
{
    //check disp queue.
    if(this->m_queueDisp==NULL || this->m_semaDispUsed==NULL || this->m_semaDispFree==NULL)
    {
        qDebug()<<"<error>:no bind display queue,cannot start thread.";
        return -1;
    }
    //check process queue.
    if(this->m_queueProcess==NULL || this->m_semaProcessUsed==NULL || this->m_semaProcessFree==NULL)
    {
        qDebug()<<"<error>:no bind process queue,cannot start thread.";
        return -1;
    }
    //check the yuv queue if i am the main camera.
    if(this->m_bMainCamera)
    {
        if(this->m_queueYUV==NULL || this->m_semaYUVUsed==NULL || this->m_semaYUVFree==NULL)
        {
            qDebug()<<"<error>:no bind yuv queue,cannot start thread.";
            return -1;
        }
    }
    this->m_bExitFlag=false;
    this->start();
    return 0;
}
qint32 ZImgCapThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
qint32 ZImgCapThread::ZGetCAMImgWidth()
{
    return this->m_cam->ZGetImgWidth();
}
qint32 ZImgCapThread::ZGetCAMImgHeight()
{
    return this->m_cam->ZGetImgHeight();
}
qint32 ZImgCapThread::ZGetCAMImgFps()
{
    return this->m_cam->ZGetFrameRate();
}
QString ZImgCapThread::ZGetDevName()
{
    return this->m_devName;
}
bool ZImgCapThread::ZIsRunning()
{
    return true;
}
QString ZImgCapThread::ZGetCAMID()
{
    return this->m_cam->ZGetCAMID();
}

void ZImgCapThread::run()
{
    ZCAMDevice *camDev=new ZCAMDevice(this->m_devName,this->m_nPreWidth,this->m_nPreHeight,this->m_nPreFps);
    if(camDev->ZOpenCAM()<0)
    {
        qDebug()<<"<error>:failed to open cam device "<<this->m_devName;
        return;
    }
    if(camDev->ZInitCAM()<0)
    {
        qDebug()<<"<error>:failed to init cam device "<<this->m_devName;
        return;
    }
    if(camDev->ZInitMAP()<0)
    {
        qDebug()<<"<error>:failed to init map device "<<this->m_devName;
        return;
    }
    //malloc memory.
    qint32 nSingleImgSize=camDev->ZGetImgWidth()*camDev->ZGetImgHeight()*3*sizeof(char);
    unsigned char *pRGBBuffer=(unsigned char*)malloc(nSingleImgSize);
    if(NULL==pRGBBuffer)
    {
        qDebug()<<"<error>:failed to allocate RGB buffer for device "<<this->m_devName;
        return;
    }
    //start capture.
    if(camDev->ZStartCapture()<0)
    {
        qDebug()<<"<error>:failed to start capture "<<this->m_devName;
        return;
    }
    if(this->m_bMainCamera)
    {
        qDebug()<<"<MainLoop>:MainCamera capture starts "<<this->m_devName<<".";
    }else{
        qDebug()<<"<MainLoop>:AuxCamera capture starts "<<this->m_devName<<".";
    }

    while(!gGblPara.m_bGblRst2Exit || !this->m_bExitFlag)
    {
        qint32 nLen;
        qint64 nStartTs,nEndTs;
        unsigned char *pYUVData;
        if(gGblPara.m_bCaptureLog)
        {
            nStartTs=QDateTime::currentDateTime().toMSecsSinceEpoch();
            qDebug()<<"<TS>:"<<this->m_devName<<" start at "<<nStartTs;
        }
        if(camDev->ZGetFrame((void**)&pYUVData,(size_t*)&nLen)<0)
        {
            qDebug()<<"<error>:failed to get yuv data from "<<this->m_devName;
            break;
        }

#if 1
        //if I am the main camera,put yuv data to yuv queue.
        //只有当有客户端连接时,才将yuv数据扔入yuv队列中,促使编码线程工作.
        if(this->m_bMainCamera && gGblPara.m_bTcpClientConnected)
        {
            QByteArray baYUVData((const char*)pYUVData,nLen);
            this->m_semaYUVFree->acquire();//空闲信号量减1.
            this->m_queueYUV->enqueue(baYUVData);
            this->m_semaYUVUsed->release();//已用信号量加1.
        }
#endif
        //convert yuv to RGB.
        YUYVToRGB_table(pYUVData,pRGBBuffer,camDev->ZGetImgWidth(),camDev->ZGetImgHeight());
        //build a RGB888 QImage object.
        QImage newQImg((uchar*)pRGBBuffer,camDev->ZGetImgWidth(),camDev->ZGetImgHeight(),QImage::Format_RGB888);
        //free a buffer for device.
        camDev->ZUnGetFrame();

        if(gGblPara.m_bCaptureLog)
        {
            nEndTs=QDateTime::currentDateTime().toMSecsSinceEpoch();
            qDebug()<<"<TS>:"<<this->m_devName<<" end at "<<nEndTs<<",cost "<<nEndTs-nStartTs<<"ms";
        }

        //put QImage to QImage queue for ImgProcessThread.
        if(this->m_semaProcessFree->tryAcquire())//空闲信号量减1.
        {
            this->m_queueProcess->enqueue(newQImg);
            this->m_semaProcessUsed->release();//已用信号量加1.
        }
#if 1
        //put QImage to local queue for LocalDisplay.
        if(this->m_semaDispFree->tryAcquire())//空闲信号量减1.
        {
            this->m_queueDisp->enqueue(newQImg);
            this->m_semaDispUsed->release();//已用信号量加1.
        }
#endif
        this->usleep(VIDEO_THREAD_SCHEDULE_US);
    }
    //do some clean. stop camera.
    camDev->ZStopCapture();
    camDev->ZUnInitCAM();
    camDev->ZCloseCAM();
    //free memory.
    free(pRGBBuffer);
    //此处设置本线程退出标志.
    //同时设置全局请求退出标志，请求其他线程退出.
    if(this->m_bMainCamera)
    {
        qDebug()<<"<MainLoop>:MainCamera capture starts "<<this->m_devName<<".";
        gGblPara.m_bMainCapThreadExitFlag=true;
    }else{
        qDebug()<<"<MainLoop>:AuxCamera capture starts "<<this->m_devName<<".";
        gGblPara.m_bAuxCapThreadExitFlag=true;
    }
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
    return;
}
