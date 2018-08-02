#ifndef ZGBLPARA_H
#define ZGBLPARA_H
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <QtGui/QImage>
//use hard h264 encoder
//or use soft h264 encoder.
//#define USE_HARD_H264_ENCODER   1

#define APP_VERSION "0.1.0" //2018/07/10.

//port define.
#define TCP_PORT_AUDIO  6801 //传输opus音频
#define TCP_PORT_CTL    6802 //传输音频算法启停控制
#define TCP_PORT_VIDEO  6803 //传输h264视频
#define TCP_PORT_FORWARD   6804 //串口透传，用于Android手工调节电机，测距，设置参数等.
#define TCP_PORT_VIDEO2  6805 //传输h264视频

#define DRW_IMG_SIZE_W  500
#define DRW_IMG_SIZE_H  500

//use usleep() to reduce cpu heavy load.
#define AUDIO_THREAD_SCHEDULE_US    (1000*10) //10ms.
#define VIDEO_THREAD_SCHEDULE_US    (1000*10) //10ms.
/////////////////////////////////////////////////////////////////////////////
#include <QString>
#include <QQueue>
#include <QSemaphore>
//当声卡工作时，数据总是连续地在硬件缓冲区与应用程序之间传输。
//在录音时，如果应用程序读取数据不够快，将导致缓冲区旧数据被新数据覆盖，这种数据丢失叫overrun.
//在回放时，如果应用程序填充硬件缓冲区速度不够快，将导致缓冲区被饿死，这种现在叫underrun.

//* The sample rate of the audio codec **
#define     SAMPLE_RATE      48000

//frame帧是播放样本的一个计量单位，由通道数和比特数决定。
//立体声48KHz 16-bit的PCM，那么一帧的大小就是4字节(2 Channels*16-bit=32bit/8bit=4 bytes)
//5.1通道48KHz 16-bit的PCM，那么一帧的大小就是12字节(5.1这里取6,6Channels*16bit=96bit/8bit=12 bytes)
#define CHANNELS_NUM    2
#define BYTES_PER_FRAME 4

//period:周期，是指每两个硬件中断之间的帧数，poll会在每个周期返回一次.
//alsa将内部的缓冲区拆分成period(周期）又叫fragments（片断）
//alsa以period为单元来传送数据，一个period存储一些frames,一个frames中包含了左右声道的数据采样点.
//硬件的buffer被拆分成period，一个period包含多个frame，一个frame包含多个数据采样点.
#define ALSA_PERIOD     20//10//4
////////////1秒中断4次会出现卡顿很严重的情况。
////////////修改为1秒中断10次，现象好多了。



//立体声，16-bit采样位数，44.1KHz采样率
//立体声，ChannelNum=2
//1次模拟采样就是16-bit（2字节），因为是双通道，所以是4字节
//1个frame中最小的传输单位，
//1 frame=(ChannelNum)*(1 sample in bytes)=(2通道）×16bit=4字节。
//为了达到2*44.1KHz的采样率，系统必须达到该值的传输速率
//传输速率=(ChannelNum)*(1 sample in bytes)*(SampleRate)
//=2通道*一次采样16bit×44.1KHz采样率
//=2×16bit×44100Hz=1411200bits_per_seconds=176400 Bytes_per_second.
//现在，如果ALSA每秒中断一次，那么我们就至少需要准备好176400字节数据发送给声卡，才不会导致播放卡
//如果中断每500ms发生一次，那么我们就至少需要176400Bytes/(1s/500ms)=88200bytes的数据
//如果中断每100ms发生一次，那么我们就至少要准备176400Bytes/(1s/100ms)=17640Bytes的数据
///////////////////////////////////////////////////////////////////////////////////
//这里我们使用48KHz的采样率，双声道，采样位数16bit
//则有bps=2*16bit*48000=1536000bits/s=192000Bytes
//这里我们设置period为4，即1秒发生4次中断，则中断间隔为1s/4=250ms.
//则每次中断发生时，我们至少需要填充192000Bytes/(1s/250ms)=48000Bytes
#define     BLOCK_SIZE        48000	// Number of bytes

#define TCP_PORT    6801

//for opus encode/decode.
#define OPUS_SAMPLE_FRMSIZE     (2880) //frame size in 16 bit sample.
#define OPUS_BLKFRM_SIZEx2      (OPUS_SAMPLE_FRMSIZE*CHANNELS_NUM*sizeof(opus_int16)) //2 channels.
#define OPUS_BITRATE            64000


//#define SPLIT_IMG_3X3   1
#define SPLIT_IMG_2X2   1
//Video related parameters.

#define BUFSIZE_1MB     (1*1024*1024)
#define BUFSIZE_2MB     (2*1024*1024)
#define BUFSIZE_4MB     (4*1024*1024)
#define BUFSIZE_8MB     (8*1024*1024)
#define BUFSIZE_10MB    (10*1024*1024)

//audio related parameters.
class ZAudioParam
{
public:
    ZAudioParam();
public:
    //capture card name.
    QString m_capCardName;
    //play card name.
    QString m_playCardName;

public:
    //run mode.
    //0:Only do capture and write pcm to wav file.
    //1:realtime stream process,capture-process-playback.
    //1 at default.
    qint32 m_runMode;

    //capture audio sample rate.
    qint32 m_nCaptureSampleRate;
    //playback audio sample rate.
    qint32 m_nPlaybackSampleRate;
    //channel number for capture & playback.
    qint32 m_nChannelNum;
    //de-noise control.
    qint32 m_nDeNoiseMethod;

    qint32 m_nGaindB;
    qint32 m_nBevisGrade;
public:
    //capture thread thread buffer overrun.
    qint32 m_nCapOverrun;
    //playback thread buffer underrun.
    qint32 m_nPlayUnderrun;

public:
    //同一时刻我们仅允许一个tcp客户端连接.
    bool m_bAudioTcpConnected;
};
class ZVideoParam
{
public:
    ZVideoParam();
public:
    QString m_Cam1ID;
    QString m_Cam2ID;

    //不比较CamID，不决定主辅摄像头
    //用于第一次运行时获取摄像头信息
    bool m_bDoNotCmpCamId;
};
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
    bool m_bVideoTcpConnected;
    bool m_bVideoTcpConnected2;

public:
    //Android(tcp) <--> STM32(uart) 串口透传线程相关.
    //Tcp2Uart thread related flags.
    bool m_bTcp2UartThreadExitFlag;
    bool m_bTcp2UartConnected;
    qint64 m_nTcp2UartBytes;
    qint64 m_nUart2TcpBytes;

public:
    //控制端口线程.
    bool m_bCtlThreadExitFlag;
    bool m_bCtlClientConnected;


public:
    //JSON协议控制标志位.
    bool m_bJsonImgPro;//ImgPro图像比对启停控制标志位.
    bool m_bJsonFlushUIImg;//是否刷新本地UI.
    bool m_bJsonFlushUIWav;
public:
    //audio related parameters.
    ZAudioParam m_audio;
    //video related parameters.
    ZVideoParam m_video;

public:
    //accumulated run seconds.
    qint64 m_nAccumulatedSec;
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


#define _CURRENT_DATETIME_    QDateTime::currentDateTime().toString("[yyyy/MM/dd hh:mm:ss]")


#define MAX_AUDIO_RING_BUFFER  (10)
#define MAX_VIDEO_RING_BUFFER  (10)
#endif // ZGBLPARA_H
