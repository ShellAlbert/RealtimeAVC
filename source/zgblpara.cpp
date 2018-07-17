#include "zgblpara.h"
#include <QDebug>
#include <QSettings>
#include <unistd.h>
#include <stdio.h>
#include <QFile>
ZGblPara gGblPara;
ZGblPara::ZGblPara()
{
    this->m_bDebugMode=false;
    this->m_bVerbose=false;
    this->m_bDumpCamInfo2File=false;
    this->m_bCaptureLog=false;
    this->m_bTransfer2PC=false;
    this->m_bTransferSpeedMonitor=false;
    this->m_bDumpUART=false;
    //default xMode is on.fMode is off.
    this->m_bXMode=true;
    this->m_bFMode=false;

    this->m_bGblRst2Exit=false;
    this->m_bGblStartFlag=false;


    this->m_bMainCapThreadExitFlag=false;
    this->m_bAuxCapThreadExitFlag=false;
    this->m_bImgCmpThreadExitFlag=false;
    this->m_bVideoTxThreadExitFlag=false;
    //Tcp2Uart thread exit flag.
    this->m_bTcp2UartThreadExitFlag=false;

    this->m_bTcpClientConnected=false;
    this->m_bTcp2UartConnected=false;
}
ZGblPara::~ZGblPara()
{
}
qint32 ZGblPara::writePid2File()
{
    //write pid file.
    pid_t pidMy=getpid();
    char buffer[32];
    sprintf(buffer,"%d",pidMy);
    QFile filePID("/tmp/AVLizard.pid");
    if(filePID.open(QIODevice::WriteOnly))
    {
        filePID.write(buffer,sizeof(buffer));
        filePID.close();
    }else{
        if(this->m_bVerbose)
        {
            qDebug()<<"<error>:failed to write /tmp/AVLizard.pid!";
        }
        return -1;
    }
    if(this->m_bVerbose)
    {
        qDebug()<<"<info>:/tmp/AVLizard.pid generated.";
    }
    return 0;
}
void ZGblPara::readCfgFile()
{
    //read calibrate center.
    QSettings iniFile("AVLizard.ini",QSettings::IniFormat);
    iniFile.beginGroup("CAM1");
    gGblPara.m_widthCAM1=iniFile.value("width",0).toInt();
    gGblPara.m_heightCAM1=iniFile.value("height",0).toInt();
    gGblPara.m_fpsCAM1=iniFile.value("fps",0).toInt();
    gGblPara.m_calibrateX1=iniFile.value("x1",0).toInt();
    gGblPara.m_calibrateY1=iniFile.value("y1",0).toInt();
    gGblPara.m_idCAM1=iniFile.value("id","cam1").toString();
    iniFile.endGroup();
    iniFile.beginGroup("CAM2");
    gGblPara.m_widthCAM2=iniFile.value("width",0).toInt();
    gGblPara.m_heightCAM2=iniFile.value("height",0).toInt();
    gGblPara.m_fpsCAM2=iniFile.value("fps",0).toInt();
    gGblPara.m_calibrateX2=iniFile.value("x2",0).toInt();
    gGblPara.m_calibrateY2=iniFile.value("y2",0).toInt();
    gGblPara.m_idCAM2=iniFile.value("id","cam2").toString();
    iniFile.endGroup();
    iniFile.beginGroup("CuteTemplate");
    gGblPara.m_nCutTemplateWidth=iniFile.value("width",0).toInt();
    gGblPara.m_nCutTemplateHeight=iniFile.value("height",0).toInt();
    iniFile.endGroup();
    iniFile.beginGroup("UART");
    gGblPara.m_uartName=iniFile.value("name","ttyS4").toString();
    iniFile.endGroup();
    if(this->m_bVerbose)
    {
        qDebug()<<"CAM1 parameters:";
        qDebug()<<"resolution:"<<gGblPara.m_widthCAM1<<"*"<<gGblPara.m_heightCAM1<<",fps:"<<gGblPara.m_fpsCAM1;
        qDebug()<<"calibrate center point ("<<gGblPara.m_calibrateX1<<","<<gGblPara.m_calibrateY1<<")";

        qDebug()<<"CAM2 parameters:";
        qDebug()<<"resolution:"<<gGblPara.m_widthCAM2<<"*"<<gGblPara.m_heightCAM2<<",fps:"<<gGblPara.m_fpsCAM2;
        qDebug()<<"calibrate center point ("<<gGblPara.m_calibrateX2<<","<<gGblPara.m_calibrateY2<<")";
        qDebug()<<"Cut Template size: ("<<gGblPara.m_nCutTemplateWidth<<"*"<<gGblPara.m_nCutTemplateHeight<<")";
    }
}
void ZGblPara::writeCfgFile()
{
    //write calibrate center.
    QSettings iniFile("AVLizard.ini",QSettings::IniFormat);
    iniFile.beginGroup("CAM1");
    iniFile.setValue("width",gGblPara.m_widthCAM1);
    iniFile.setValue("height",gGblPara.m_heightCAM1);
    iniFile.setValue("fps",gGblPara.m_fpsCAM1);
    iniFile.setValue("x1",gGblPara.m_calibrateX1);
    iniFile.setValue("y1",gGblPara.m_calibrateY1);
    iniFile.setValue("id",gGblPara.m_idCAM1);
    iniFile.endGroup();
    iniFile.beginGroup("CAM2");
    iniFile.setValue("width",gGblPara.m_widthCAM2);
    iniFile.setValue("height",gGblPara.m_heightCAM2);
    iniFile.setValue("fps",gGblPara.m_fpsCAM2);
    iniFile.setValue("x2",gGblPara.m_calibrateX2);
    iniFile.setValue("y2",gGblPara.m_calibrateY2);
    iniFile.setValue("id",gGblPara.m_idCAM2);
    iniFile.endGroup();

    iniFile.beginGroup("CuteTemplate");
    iniFile.setValue("width",gGblPara.m_nCutTemplateWidth);
    iniFile.setValue("height",gGblPara.m_nCutTemplateHeight);
    iniFile.endGroup();

    iniFile.beginGroup("UART");
    iniFile.setValue("name",gGblPara.m_uartName);
    iniFile.endGroup();
}
void ZGblPara::resetCfgFile()
{
    gGblPara.m_widthCAM1=640;
    gGblPara.m_heightCAM1=480;
    gGblPara.m_fpsCAM1=15;
    gGblPara.m_calibrateX1=320;
    gGblPara.m_calibrateY1=240;
    gGblPara.m_idCAM1=QString("cam1");

    gGblPara.m_widthCAM2=640;
    gGblPara.m_heightCAM2=480;
    gGblPara.m_fpsCAM2=15;
    gGblPara.m_calibrateX2=400;
    gGblPara.m_calibrateY2=310;
    gGblPara.m_idCAM2=QString("cam2");

    gGblPara.m_nCutTemplateWidth=200;
    gGblPara.m_nCutTemplateHeight=200;

    gGblPara.m_uartName=QString("ttyS4");


    this->writeCfgFile();
}
uint8_t ZGblPara::ZDoCheckSum(uint8_t *pack,uint8_t pack_len)
{
    uint8_t i;
    uint8_t check_sum=0;
    for(i=0;i<pack_len;i++)
    {
        check_sum+=*(pack+i);
    }
    return check_sum;
}
void ZGblPara::int32_char8x4(qint32 int32,char *char8x4)
{
    char *pInt32=(char*)&int32;
    char8x4[0]=pInt32[0];
    char8x4[1]=pInt32[1];
    char8x4[2]=pInt32[2];
    char8x4[3]=pInt32[3];
}
void ZGblPara::int32_char8x2_low(qint32 int32,char *char8x2)
{
    char *pInt32=(char*)&int32;
    char8x2[0]=pInt32[0];
    char8x2[1]=pInt32[1];
    if(int32<0)
    {
        char8x2[1]|=0x80;
    }
}

cv::Mat QImage2cvMat(const QImage &img)
{
    cv::Mat mat;
    //qDebug()<<"format:"<<img.format();
    switch(img.format())
    {
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat=cv::Mat(img.height(),img.width(),CV_8UC4,(void*)img.constBits(),img.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat=cv::Mat(img.height(),img.width(),CV_8UC3,(void*)img.constBits(),img.bytesPerLine());
        cv::cvtColor(mat,mat,CV_BGR2RGB);
        break;
    case QImage::Format_Indexed8:
        mat=cv::Mat(img.height(),img.width(),CV_8UC1,(void*)img.constBits(),img.bytesPerLine());
        break;
    default:
        break;
    }
    return mat;
}
QImage cvMat2QImage(const cv::Mat &mat)
{
    //8-bit unsigned ,number of channels=1.
    if(mat.type()==CV_8UC1)
    {
        QImage img(mat.cols,mat.rows,QImage::Format_Indexed8);
        //set the color table
        //used to translate colour indexes to qRgb values.
        img.setColorCount(256);
        for(qint32 i=0;i<256;i++)
        {
            img.setColor(i,qRgb(i,i,i));
        }
        //copy input mat.
        uchar *pSrc=mat.data;
        for(qint32 row=0;row<mat.rows;row++)
        {
            uchar *pDst=img.scanLine(row);
            memcpy(pDst,pSrc,mat.cols);
            pSrc+=mat.step;
        }
        return img;
    }else if(mat.type()==CV_8UC3)
    {
        //8-bits unsigned,number of channels=3.
        const uchar *pSrc=(const uchar*)mat.data;
        QImage img(pSrc,mat.cols,mat.rows,mat.step,QImage::Format_RGB888);
        return img.rgbSwapped();
    }else if(mat.type()==CV_8UC4)
    {
        const uchar *pSrc=(const uchar*)mat.data;
        QImage img(pSrc,mat.cols,mat.rows,mat.step,QImage::Format_ARGB32);
        return img.copy();
    }else{
        if(gGblPara.m_bVerbose)
        {
            qDebug()<<"failed to convert cv::Mat to QImage!";
        }
        return QImage();
    }
}

QByteArray qint32ToQByteArray(qint32 val)
{
    QByteArray ba;
    ba.resize(4);
    ba[0]=(uchar)((0x000000FF & val)>>0);
    ba[1]=(uchar)((0x0000FF00 & val)>>8);
    ba[2]=(uchar)((0x00FF0000 & val)>>16);
    ba[3]=(uchar)((0xFF000000 & val)>>24);
    return ba;
}
qint32 QByteArrayToqint32(QByteArray ba)
{
    qint32 val=0;
    val|=((ba[0]<<0)&0x000000FF);
    val|=((ba[1]<<8)&0x0000FF00);
    val|=((ba[2]<<16)&0x00FF0000);
    val|=((ba[3]<<24)&0xFF000000);
    return val;
}
