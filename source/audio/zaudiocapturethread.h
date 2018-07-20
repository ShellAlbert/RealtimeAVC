#ifndef ZAUDIOCAPTURETHREAD_H
#define ZAUDIOCAPTURETHREAD_H
#include "../zgblpara.h"
#include <QThread>
#include <QTimer>

/* Use the newer ALSA API */
#include <stdio.h>
#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <QSemaphore>
#include <QQueue>
#include <zringbuffer.h>

#define RSIZE    64    //buf的大小

/********以下是wave格式文件的文件头格式说明******/
/*------------------------------------------------
|             RIFF WAVE Chunk                  |
|             ID = 'RIFF'                     |
|             RiffType = 'WAVE'                |
------------------------------------------------
|             Format Chunk                     |
|             ID = 'fmt '                      |
------------------------------------------------
|             Fact Chunk(optional)             |
|             ID = 'fact'                      |
------------------------------------------------
|             Data Chunk                       |
|             ID = 'data'                      |
------------------------------------------------*/
/**********以上是wave文件格式头格式说明***********/
/*wave 文件一共有四个Chunk组成，其中第三个Chunk可以省略，每个Chunk有标示（ID）,
大小（size,就是本Chunk的内容部分长度）,内容三部分组成*/
typedef struct WAVEHEAD
{
    /****RIFF WAVE CHUNK*/
    unsigned char riff_head[4];//四个字节存放'R','I','F','F'
    int riffdata_len;        //整个文件的长度-8;每个Chunk的size字段，都是表示除了本Chunk的ID和SIZE字段外的长度;
    unsigned char wave_head[4];//四个字节存放'W','A','V','E'
    /****RIFF WAVE CHUNK*/
    /****Format CHUNK*/
    unsigned char fmt_head[4];//四个字节存放'f','m','t',''
    int fmtdata_len;       //16后没有附加消息，18后有附加消息；一般为16，其他格式转来的话为18
    short int format_tag;       //编码方式，一般为0x0001;
    short int channels;       //声道数目，1单声道，2双声道;
    int samples_persec;        //采样频率;
    int bytes_persec;        //每秒所需字节数;
    short int block_align;       //每个采样需要多少字节，若声道是双，则两个一起考虑;
    short int bits_persec;       //即量化位数
    /****Format CHUNK*/
    /***Data Chunk**/
    unsigned char data_head[4];//四个字节存放'd','a','t','a'
    int data_len;        //语音数据部分长度，不包括文件头的任何部分
}WAVE_HEAD; //定义WAVE文件的文件头结构体

class ZAudioCaptureThread : public QThread
{
    Q_OBJECT
public:
    ZAudioCaptureThread(QString capDevName,bool bDump2WavFile);
    ~ZAudioCaptureThread();

    qint32 ZStartThread(ZRingBuffer *rbNoise);
    qint32 ZStopThread();
    bool ZIsRunning();
protected:
    void run();
signals:
    void ZSigThreadFinished();
private:
    qint32 ZWriteWavHead2File();
    qint32 ZUpdateWavHead2File();

    quint64 ZGetTimestamp();
private:
    QString m_capDevName;
    bool m_bDump2WavFile;

    qint32 m_fd;
    QTimer *m_timerCapture;
    snd_pcm_t *pcmHandle;
    snd_pcm_uframes_t m_pcmFrames;
    char *m_pcmBuffer;
    int m_pcmBufferSize;

    qint32 m_nTotalBytes;

    //escape seconds.
    qint32 m_nEscapeMsec;
    qint64 m_nEscapeSec;
private:
    bool m_bExitFlag;
private:
    ZRingBuffer *m_rbNoise;
};

#endif // ZAUDIOCAPTURETHREAD_H
