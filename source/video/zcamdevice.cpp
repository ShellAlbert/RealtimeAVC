#include "zcamdevice.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <asm/types.h>
#include "zgblpara.h"
#include <linux/videodev2.h>
#include <QDebug>
#include <QFile>
#include <QApplication>
#include <QDateTime>
#include <sys/select.h>
ZCAMDevice::ZCAMDevice(QString devName,qint32 nWidth,qint32 nHeight,qint32 nFps,bool bMainCamera,QObject*parent):QObject(parent)
{
    this->m_devName=devName;
    this->m_nPredefinedWidth=nWidth;
    this->m_nPredefinedHeight=nHeight;
    this->m_nPredefinedFps=nFps;
    this->m_bMainCamera=bMainCamera;
    ///////////////////
    this->m_fd=-1;
    this->m_nIndex=-1;
    this->m_nImgWidth=-1;
    this->m_nImgHeight=-1;
    //we will use 4 buffers.
    this->m_nBufferCount=4;
    this->m_nFPS=0;
}
ZCAMDevice::~ZCAMDevice()
{

}
int ZCAMDevice::ZOpenCAM()
{
    this->m_fd=open(this->m_devName.toStdString().c_str(),O_RDWR);
    if(this->m_fd<0)
    {
        emit this->ZSigMsg("failed to open device node!",Log_Msg_Error);
        return -1;
    }
    return 0;
}
int ZCAMDevice::ZCloseCAM()
{
    close(this->m_fd);
    return 0;
}
int ZCAMDevice::ZInitCAM()
{
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmtDesc;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    struct v4l2_streamparm streamPara;

    //  /dev/video2,here 5
    QFile fileCamInfo(QApplication::applicationDirPath()+"/"+this->m_devName.mid(5));
    if(1)
    {
        fileCamInfo.open(QIODevice::WriteOnly);
        fileCamInfo.write(QDateTime::currentDateTime().toString("yyyy/MM/dd hh:mm:ss").toLatin1()+"\n");
        fileCamInfo.write(this->m_devName.toLocal8Bit()+"\n");
    }

    //query capability.
    if(ioctl(this->m_fd,VIDIOC_QUERYCAP,&cap)<0)
    {
        emit this->ZSigMsg(tr("failed to query %1 capability.").arg(this->m_devName),Log_Msg_Error);
        return -1;
    }
    //emit this->ZSigMsg(tr("%1,%2,%3,%4").arg(this->m_devName).arg((char*)cap.driver).arg((char*)cap.card).arg((char*)cap.bus_info),Log_Msg_Info);
    //qDebug()<<"Driver:"<<(char*)cap.driver<<",Device:"<<(char*)cap.card<<",Bus info:"<<(char*)cap.bus_info;
    //qDebug()<<"Capabilities:"<<cap.capabilities;
    if(1)
    {
        fileCamInfo.write(QString((char*)cap.driver).toLocal8Bit()+"\n");
        fileCamInfo.write(QString((char*)cap.card).toLocal8Bit()+"\n");
        fileCamInfo.write(QString((char*)cap.bus_info).toLocal8Bit()+"\n");
    }
    if(this->m_bMainCamera)
    {
        gGblPara.m_video.m_Cam1ID=QString((char*)cap.bus_info);
    }else{
        gGblPara.m_video.m_Cam2ID=QString((char*)cap.bus_info);
    }


    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        //emit this->ZSigMsg(tr("%1 has no video capture!").arg(this->m_devName),Log_Msg_Error);
        emit this->ZSigMsg(tr("%1 has no video capture ability!").arg(this->m_devName),Log_Msg_Error);
        return -1;
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        emit this->ZSigMsg(tr("%1 has no capture streaming ability!").arg(this->m_devName),Log_Msg_Error);
        return -1;
    }

    //get support format.
    fmtDesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtDesc.index=0;//enumerate all format.
    while(ioctl(this->m_fd,VIDIOC_ENUM_FMT,&fmtDesc)!=-1)
    {
        QString fmtName((char*)fmtDesc.description);
        //qDebug()<<this->m_devName<<fmtName;
        //get all frameSize.
        struct v4l2_frmsizeenum frmSizeEnum;
        frmSizeEnum.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        frmSizeEnum.pixel_format=fmtDesc.pixelformat;
        frmSizeEnum.index=0;//enumerate all frameSize.
        while(ioctl(this->m_fd,VIDIOC_ENUM_FRAMESIZES,&frmSizeEnum)!=-1)
        {
            //get all frameRate.
            struct v4l2_frmivalenum frmRateEnum;
            frmRateEnum.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
            frmRateEnum.pixel_format=fmtDesc.pixelformat;
            frmRateEnum.width=frmSizeEnum.discrete.width;
            frmRateEnum.height=frmSizeEnum.discrete.height;
            frmRateEnum.index=0;//enumerate all frame Intervals.(fps).
            while(ioctl(this->m_fd,VIDIOC_ENUM_FRAMEINTERVALS,&frmRateEnum)!=-1)
            {
                //dump width*height,fps to file.
                if(1)
                {
                    QString fmtInfo;
                    fmtInfo+=fmtName+",";
                    fmtInfo+=QString::number(frmSizeEnum.discrete.width,10)+"*"+QString::number(frmSizeEnum.discrete.height)+",";
                    fmtInfo+=QString::number(frmRateEnum.discrete.denominator/frmRateEnum.discrete.numerator,10)+"fps";
                    fmtInfo+="\n";
                    fileCamInfo.write(fmtInfo.toLocal8Bit());
                }
                frmRateEnum.index++;
            }
            frmSizeEnum.index++;
        }
        fmtDesc.index++;
    }

    //get default format.
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(this->m_fd,VIDIOC_G_FMT,&fmt)<0)
    {
        emit this->ZSigMsg("failed to get fmt!",Log_Msg_Error);
        return -1;
    }
    if(1)
    {
        QString fmtDefault;
        fmtDefault+=QString("Default setting format:\n");
        switch(fmt.fmt.pix.pixelformat)
        {
        case V4L2_PIX_FMT_YUYV:
            fmtDefault+=QString("YUYV pixel format.\n");
            break;
        case V4L2_PIX_FMT_MJPEG:
            fmtDefault+=QString("MJPEG pixel format.\n");
            break;
        default:
            fmtDefault+=QString("other pixel format.\n");
            break;
        }
        fmtDefault+=QString("%1*%2,image size=%3").arg(fmt.fmt.pix.width).arg(fmt.fmt.pix.height).arg(fmt.fmt.pix.sizeimage);
        fmtDefault+="\n";
        fileCamInfo.write(fmtDefault.toLocal8Bit());
    }

    //get default fps.
    memset(&streamPara,0,sizeof(streamPara));
    streamPara.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(this->m_fd,VIDIOC_G_PARM,&streamPara)<0)
    {
        emit this->ZSigMsg("failed to get stream param!",Log_Msg_Error);
        return -1;
    }
    if(1)
    {
        QString fpsDefault;
        fpsDefault+=QString("Default fps:%1/%2\n")///<
                .arg(streamPara.parm.output.timeperframe.numerator)///<
                .arg(streamPara.parm.output.timeperframe.denominator);
        fileCamInfo.write(fpsDefault.toLocal8Bit());
    }

    /* 16  YUV 4:2:2     */
    //set predefined format.
    memset(&fmt,0,sizeof(fmt));
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width=this->m_nPredefinedWidth;
    fmt.fmt.pix.height=this->m_nPredefinedHeight;
    fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_YUYV;//V4L2_PIX_FMT_MJPEG.
    fmt.fmt.pix.field=V4L2_FIELD_INTERLACED;//V4L2_FIELD_NONE;//V4L2_FIELD_INTERLACED;
    if(ioctl(this->m_fd,VIDIOC_S_FMT,&fmt)<0)
    {
        //emit this->ZSigMsg("failed to set format!",Log_Msg_Error);
        qDebug()<<"<error>:failed to set format YUYV.";
        return -1;
    }
    //set predefined fps.
    streamPara.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    streamPara.parm.capture.timeperframe.numerator=1;
    streamPara.parm.capture.timeperframe.denominator=this->m_nPredefinedFps;
    if(ioctl(this->m_fd,VIDIOC_S_PARM,&streamPara)<0)
    {
        this->ZSigMsg("failed to set fps!",Log_Msg_Error);
        return -1;
    }
    if(1)
    {
        QString fmtDefault;
        fmtDefault+=QString("Reqeust to set %1*%2,%3 fps").arg(this->m_nPredefinedWidth).arg(this->m_nPredefinedHeight).arg(this->m_nPredefinedFps);
        fmtDefault+="\n";
        fileCamInfo.write(fmtDefault.toLocal8Bit());
    }

    //get format again to verify my setting.
    memset(&fmt,0,sizeof(fmt));
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(this->m_fd,VIDIOC_G_FMT,&fmt)<0)
    {
        this->ZSigMsg("failed to get fmt!",Log_Msg_Error);
        return -1;
    }
    if(1)
    {
        QString fmtDefault;
        fmtDefault+=QString("Readback setting format:\n");
        switch(fmt.fmt.pix.pixelformat)
        {
        case V4L2_PIX_FMT_YUYV:
            fmtDefault+=QString("YUYV pixel format.\n");
            break;
        case V4L2_PIX_FMT_MJPEG:
            fmtDefault+=QString("MJPEG pixel format.\n");
            break;
        default:
            fmtDefault+=QString("other pixel format.\n");
            break;
        }
        fmtDefault+=QString("%1*%2,image size=%3").arg(fmt.fmt.pix.width).arg(fmt.fmt.pix.height).arg(fmt.fmt.pix.sizeimage);
        fmtDefault+="\n";
        fileCamInfo.write(fmtDefault.toLocal8Bit());
    }
    //hold the width*height.
    this->m_nImgWidth=fmt.fmt.pix.width;
    this->m_nImgHeight=fmt.fmt.pix.height;
    if(this->m_nImgWidth<=0 || this->m_nImgHeight<=0)
    {
        emit this->ZSigMsg("failed to verify width*height!",Log_Msg_Error);
        return -1;
    }

    //get fps again to verify my setting.
    streamPara.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(this->m_fd,VIDIOC_G_PARM,&streamPara)<0)
    {
        emit this->ZSigMsg("failed to get stream param!",Log_Msg_Error);
        return -1;
    }
    if(1)
    {
        QString fpsDefault;
        fpsDefault+=QString("Readback fps:%1/%2\n")///<
                .arg(streamPara.parm.output.timeperframe.numerator)///<
                .arg(streamPara.parm.output.timeperframe.denominator);
        fileCamInfo.write(fpsDefault.toLocal8Bit());
        fileCamInfo.flush();
        fileCamInfo.close();
    }
    if(streamPara.parm.output.timeperframe.denominator>streamPara.parm.output.timeperframe.numerator)
    {
        this->m_nFPS=streamPara.parm.output.timeperframe.denominator/streamPara.parm.output.timeperframe.numerator;
    }
    //check fps.
    if(this->m_nFPS<=0)
    {
        emit this->ZSigMsg("failed to verify fps!",Log_Msg_Error);
        return -1;
    }

    return 0;
}
int ZCAMDevice::ZUnInitCAM()
{
    for(unsigned  int i=0;i<this->m_nBufferCount;i++)
    {
        if(munmap(this->m_IMGBuffer[i].pStart,this->m_IMGBuffer[i].nLength)<0)
        {
            emit this->ZSigMsg("failed to munmap!",Log_Msg_Error);
            return -1;
        }
    }
    free(this->m_IMGBuffer);
    return 0;
}
int ZCAMDevice::ZInitMAP()
{
    //request the uvc driver to allocate buffers.
    struct v4l2_requestbuffers reqBuf;
    reqBuf.count=this->m_nBufferCount;
    reqBuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqBuf.memory=V4L2_MEMORY_MMAP;
    if(ioctl(this->m_fd,VIDIOC_REQBUFS,&reqBuf)<0)
    {
        emit this->ZSigMsg("failed to request buffer.",Log_Msg_Error);
        return -1;
    }

    this->m_IMGBuffer=(IMGBufferStruct*)malloc(reqBuf.count*sizeof(IMGBufferStruct));
    if(NULL==this->m_IMGBuffer)
    {
        emit this->ZSigMsg("failed to calloc buffer!",Log_Msg_Error);
        return -1;
    }
    for(unsigned int i=0;i<reqBuf.count;i++)
    {
        //query buffer & do mmap.
        struct v4l2_buffer buf;
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        buf.index=i;
        if(ioctl(this->m_fd,VIDIOC_QUERYBUF,&buf)<0)
        {
            emit this->ZSigMsg("failed to query buffer!",Log_Msg_Error);
            return -1;
        }
        this->m_IMGBuffer[i].nLength=buf.length;
        this->m_IMGBuffer[i].pStart=mmap(NULL,buf.length,PROT_READ|PROT_WRITE,MAP_SHARED,this->m_fd,buf.m.offset);
        if(MAP_FAILED==this->m_IMGBuffer[i].pStart)
        {
            emit this->ZSigMsg("failed to mmap!",Log_Msg_Error);
            return -1;
        }
    }
    return 0;
}
int ZCAMDevice::ZStartCapture()
{
    this->m_nCapTotal=0;
    for(unsigned int i=0;i<this->m_nBufferCount;i++)
    {
        //queue buffer.
        struct v4l2_buffer buf;
        memset(&buf,0,sizeof(buf));
        buf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory=V4L2_MEMORY_MMAP;
        buf.index=i;
        if(ioctl(this->m_fd,VIDIOC_QBUF,&buf)<0)
        {
            emit this->ZSigMsg("failed to query buffer!",Log_Msg_Error);
            return -1;
        }
    }
    //stream on.
    v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(this->m_fd,VIDIOC_STREAMON,&type)<0)
    {
        emit this->ZSigMsg("failed to stream on",Log_Msg_Error);
        return -1;
    }
    return 0;
}
int ZCAMDevice::ZStopCapture()
{
    v4l2_buf_type type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(this->m_fd,VIDIOC_STREAMOFF,&type)<0)
    {
        emit this->ZSigMsg("failed to stream off",Log_Msg_Error);
        return -1;
    }
    return 0;
}
int ZCAMDevice::ZGetFrame(void **pBuffer,size_t *nLen)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(this->m_fd,&fds);
    struct timeval tv;
    tv.tv_sec=0;
    tv.tv_usec=1000*100;//100ms.
    int ret=select(this->m_fd+1,&fds,NULL,NULL,&tv);
    if(ret<0)
    {
        qDebug()<<"<error>:select() cam device error.";
        return -1;
    }else if(0==ret){
        qDebug()<<"<Warning>:select() timeout for cam device.";
        return 0;
    }else{
        struct v4l2_buffer getBuf;
        memset(&getBuf,0,sizeof(getBuf));
        getBuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        getBuf.memory=V4L2_MEMORY_MMAP;
        if(ioctl(this->m_fd,VIDIOC_DQBUF,&getBuf)<0)
        {
            qDebug()<<"<error>:failed to DQBUF";
            return -1;
        }
        *pBuffer=this->m_IMGBuffer[getBuf.index].pStart;
        *nLen=this->m_IMGBuffer[getBuf.index].nLength;
        //remember the index.
        this->m_nIndex=getBuf.index;
        return 1;
    }

    //qDebug()<<"get frame len:"<<*nLen;
#if 0
    //write img to file.
    QString fileName=tr("%1.yuv").arg(m_nCapTotal);
    QFile fileYUV(fileName);
    qDebug()<<fileName;
    if(fileYUV.open(QIODevice::WriteOnly))
    {
        fileYUV.write((const char*)&pBuffer,*nLen);
        fileYUV.flush();
        fileYUV.close();
    }
    m_nCapTotal++;
#endif

}
int ZCAMDevice::ZUnGetFrame()
{
    if(this->m_nIndex<0)
    {
        return -1;
    }
    struct v4l2_buffer putBuf;
    memset(&putBuf,0,sizeof(putBuf));
    putBuf.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    putBuf.memory=V4L2_MEMORY_MMAP;
    putBuf.index=this->m_nIndex;
    if(ioctl(this->m_fd,VIDIOC_QBUF,&putBuf)<0)
    {
        emit this->ZSigMsg("failed to QBUF!",Log_Msg_Error);
        return -1;
    }
    this->m_nIndex=-1;//reset it.
    return 0;
}

int ZCAMDevice::ZGetFrameRate()
{
    return this->m_nFPS;
}
