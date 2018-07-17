#ifndef ZGBLPARA_H
#define ZGBLPARA_H
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QtGui/QImage>

#define APP_VERSION "0.1.0" //2018/07/10.

//port define.
#define TCP_PORT_AUDIO  6801 //传输opus音频
#define TCP_PORT_CTL    6802 //传输音频算法启停控制
#define TCP_PORT_VIDEO  6803 //传输h264视频
#define TCP_PORT_FORWARD   6804 //串口透传，用于Android手工调节电机，测距，设置参数等

#define DRW_IMG_SIZE_W  500
#define DRW_IMG_SIZE_H  500


//#define USE_TCP 1

//#define SPLIT_IMG_3X3   1
#define SPLIT_IMG_2X2   1
class ZGblPara
{
public:
    ZGblPara();
    ~ZGblPara();

public:
    uint8_t ZDoCheckSum(uint8_t *pack,uint8_t pack_len);
    void int32_char8x2_low(qint32 int32,char *char8x2);
    void int32_char8x4(qint32 int32,char *char8x4);

    qint32 writePid2File();
    void readCfgFile();
    void writeCfgFile();
    void resetCfgFile();
public:
    //(x1,y1) center of camera1.
    qint32 m_calibrateX1;
    qint32 m_calibrateY1;
    //(x2,y2) center of camera2.
    qint32 m_calibrateX2;
    qint32 m_calibrateY2;

    //cam1 setting parameters.
    qint32 m_widthCAM1;
    qint32 m_heightCAM1;
    qint32 m_fpsCAM1;
    QString m_idCAM1;
    //cam2 setting parameters.
    qint32 m_widthCAM2;
    qint32 m_heightCAM2;
    qint32 m_fpsCAM2;
    QString m_idCAM2;

    //cut template size.
    qint32 m_nCutTemplateWidth;
    qint32 m_nCutTemplateHeight;

    //uart name.
    //because /dev/ttyS0~ttyS4 are not work well.
    //so we use /dev/ttyFIQ0 now.
    QString m_uartName;
public:
    bool m_bDebugMode;
    bool m_bVerbose;
    bool m_bDumpCamInfo2File;
    bool m_bCaptureLog;
    bool m_bTransfer2PC;
    bool m_bTransferSpeedMonitor;
    bool m_bDumpUART;
    bool m_bXMode;
    bool m_bFMode;
public:
    //the global request to exit flag.
    //it will cause every thread occurs errors.
    bool m_bGblRst2Exit;

public:
    //global start or stop flag.
    bool m_bGblStartFlag;

public:
    //主摄像头采集线程.
    bool m_bMainCapThreadExitFlag;

public:
    //辅摄像头采集线程.
    bool m_bAuxCapThreadExitFlag;

public:
    //图像比对处理线程.
    bool m_bImgCmpThreadExitFlag;

public:
    //图像传输线程.
    bool m_bVideoTxThreadExitFlag;
    bool m_bTcpClientConnected;

public:
    //Android(tcp) <--> STM32(uart) 串口透传线程相关.
    //Tcp2Uart thread related flags.
    bool m_bTcp2UartThreadExitFlag;
    bool m_bTcp2UartConnected;
};
extern ZGblPara gGblPara;
extern cv::Mat QImage2cvMat(const QImage &img);
extern QImage cvMat2QImage(const cv::Mat &mat);

//log message type.
typedef int LogMsgType;
#define Log_Msg_Info 0 //general msg.
#define Log_Msg_Warning 1 //warning msg.
#define Log_Msg_Error 2 //error msg.

class ZImgProcessedSet
{
public:
    QRect rectTemplate;
    QRect rectMatched;
    qint32 nDiffX;
    qint32 nDiffY;
    qint32 nCostMs;
};

extern QByteArray qint32ToQByteArray(qint32 val);
extern qint32 QByteArrayToqint32(QByteArray ba);

#endif // ZGBLPARA_H
