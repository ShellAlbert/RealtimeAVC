#include "zaudioplaythread.h"
#include "../zgblpara.h"
#include <sys/time.h>
#include <QDebug>
ZAudioPlayThread::ZAudioPlayThread(QString playCardName)
{
    this->m_playCardName=playCardName;
}
ZAudioPlayThread::~ZAudioPlayThread()
{

}

qint32 ZAudioPlayThread::ZStartThread(ZRingBuffer *rbClear)
{
    this->m_rbClear=rbClear;
    this->m_bExitFlag=false;
    this->start();
    return 0;
}
qint32 ZAudioPlayThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
bool ZAudioPlayThread::ZIsRunning()
{
    return this->m_bExitFlag;
}

void ZAudioPlayThread::run()
{
    unsigned int nSampleRate=SAMPLE_RATE;
    // Input and output driver variables
    snd_pcm_t *pcmHandle;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_uframes_t pcmFrames;

    int pcmBlkSize=BLOCK_SIZE;	// Raw input or output frame size in bytes
    //pcmFrames=pcmBlkSize/BYTES_PER_FRAME;	// Convert bytes to frames

    // Now we can open the PCM device:
    /* Open PCM. The last parameter of this function is the mode. */
    /* If this is set to 0, the standard mode is used. Possible   */
    /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */
    /* If SND_PCM_NONBLOCK is used, read / write access to the    */
    /* PCM device will return immediately. If SND_PCM_ASYNC is    */
    /* specified, SIGIO will be emitted whenever a period has     */
    /* been completely processed by the soundcard.                */
    if(snd_pcm_open(&pcmHandle,(char*)gGblPara.m_audio.m_playCardName.toStdString().c_str(),SND_PCM_STREAM_PLAYBACK,0)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_open():"<<gGblPara.m_audio.m_playCardName;
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    /* Allocate the snd_pcm_hw_params_t structure on the stack. */
    snd_pcm_hw_params_alloca(&hwparams);

    /*
        Before we can write PCM data to the soundcard,
        we have to specify access type, sample format, sample rate, number of channels,
        number of periods and period size.
        First, we initialize the hwparams structure with the full configuration space of the soundcard.
    */
    /* Init hwparams with full configuration space */
    if(snd_pcm_hw_params_any(pcmHandle,hwparams)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_any().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    /*
    A frame is equivalent of one sample being played,
    irrespective of the number of channels or the number of bits. e.g.
    1 frame of a Stereo 48khz 16bit PCM stream is 4 bytes.
    1 frame of a 5.1 48khz 16bit PCM stream is 12 bytes.
    A period is the number of frames in between each hardware interrupt. The poll() will return once a period.
    The buffer is a ring buffer. The buffer size always has to be greater than one period size.
    Commonly this is 2*period size, but some hardware can do 8 periods per buffer.
    It is also possible for the buffer size to not be an integer multiple of the period size.
    Now, if the hardware has been set to 48000Hz , 2 periods, of 1024 frames each,
    making a buffer size of 2048 frames. The hardware will interrupt 2 times per buffer.
    ALSA will endeavor to keep the buffer as full as possible.
    Once the first period of samples has been played,
    the third period of samples is transfered into the space the first one occupied
    while the second period of samples is being played. (normal ring buffer behaviour).
*/

    /*
    The access type specifies the way in which multichannel data is stored in the buffer.
    For INTERLEAVED access, each frame in the buffer contains the consecutive sample data for the channels.
    For 16 Bit stereo data,
    this means that the buffer contains alternating words of sample data for the left and right channel.
    For NONINTERLEAVED access,
    each period contains first all sample data for the first channel followed by the sample data
    for the second channel and so on.
*/

    /* Set access type. This can be either    */
    /* SND_PCM_ACCESS_RW_INTERLEAVED or       */
    /* SND_PCM_ACCESS_RW_NONINTERLEAVED.      */
    /* There are also access types for MMAPed */
    /* access, but this is beyond the scope   */
    /* of this introduction.                  */
    if(snd_pcm_hw_params_set_access(pcmHandle,hwparams,SND_PCM_ACCESS_RW_INTERLEAVED)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_access().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    /* Set sample format */
    if(snd_pcm_hw_params_set_format(pcmHandle,hwparams,SND_PCM_FORMAT_S16_LE)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_format().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    /* Set sample rate. If the exact rate is not supported */
    /* by the hardware, use nearest possible rate.         */
    unsigned int nRealSampleRate=nSampleRate;
    if(snd_pcm_hw_params_set_rate_near(pcmHandle,hwparams,&nRealSampleRate,0u)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_rate_near().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }
    if(nRealSampleRate!=nSampleRate)
    {
        qDebug()<<"<Warning>:Audio PlayThread,the rate "<<nSampleRate<<" Hz is not supported by hardware.";
        qDebug()<<"<Warning>:Using "<<nRealSampleRate<<" instead.";
    }

    /* Set number of channels */
    if(snd_pcm_hw_params_set_channels(pcmHandle,hwparams,CHANNELS_NUM)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_channels().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    /* Set number of periods. Periods used to be called fragments. */
    /* Number of periods, See http://www.alsa-project.org/main/index.php/FramesPeriods */
    unsigned int periods=ALSA_PERIOD;
    unsigned int request_periods;
    int dir=0;
    request_periods=periods;
    //	Restrict a configuration space to contain only one periods count
    if(snd_pcm_hw_params_set_periods_near(pcmHandle,hwparams,&periods,&dir)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_periods_near().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }
    if(request_periods!=periods)
    {
        qDebug("<Warning>:Audio PlayThread,requested %d periods,but recieved %d.\n", request_periods, periods);
    }

    /*
        The unit of the buffersize depends on the function.
        Sometimes it is given in bytes, sometimes the number of frames has to be specified.
        One frame is the sample data vector for all channels.
        For 16 Bit stereo data, one frame has a length of four bytes.
    */
    if(snd_pcm_hw_params_set_buffer_size_near(pcmHandle,hwparams,&pcmFrames)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params_set_buffer_size_near().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    /*
        If your hardware does not support a buffersize of 2^n,
        you can use the function snd_pcm_hw_params_set_buffer_size_near.
        This works similar to snd_pcm_hw_params_set_rate_near.
        Now we apply the configuration to the PCM device pointed to by pcm_handle.
        This will also prepare the PCM device.
    */
    /* Apply HW parameter settings to PCM device and prepare device*/
    if(snd_pcm_hw_params(pcmHandle,hwparams)<0)
    {
        qDebug()<<"<Error>:Audio PlayThread,error at snd_pcm_hw_params().";
        //set global request to exit flag to cause other threads to exit.
        gGblPara.m_bGblRst2Exit=true;
        return;
    }

    char *pcmBuffer=new char[BLOCK_SIZE];
    qint32 nPCMBufferLen=0;
    qint32 nRemaingBytes=0;
    //the main loop.
    qDebug()<<"<MainLoop>:PlaybackThread starts.";
    while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
    {
        qint32 nPCMLen;
        //wait until the queue has 5 frames.
        //        if(this->m_semaUsed->available()<10)
        //        {
        //            continue;
        //        }

        //尝试从播放队列中取出一帧音频数据.
        if(!this->m_rbClear->m_semaUsed->tryAcquire())
        {
            this->usleep(AUDIO_THREAD_SCHEDULE_US);
            continue;
        }
        nPCMLen=this->m_rbClear->ZGetElement((qint8*)pcmBuffer,BLOCK_SIZE);
        this->m_rbClear->m_semaFree->release();
        if(nPCMLen<=0)
        {
            qDebug()<<"<Error>:Audio PlayThread get 0 length of pcm data from clear queue.";
            continue;
        }
#if 0
        //如果数据量不够，则开始整理数据.
        if(nPCMBufferLen<BLOCK_SIZE)
        {
            if((baPCMData.size()+nPCMBufferLen)>=BLOCK_SIZE)
            {
                qint32 nPaddingBytes=BLOCK_SIZE-nPCMBufferLen;
                memcpy(pcmBuffer+nPCMBufferLen,baPCMData.data(),nPaddingBytes);
                nPCMBufferLen+=nPaddingBytes;
                nRemaingBytes=baPCMData.size()-nPaddingBytes;
                //qDebug()<<"more:"<<nPCMBufferLen<<"pcmSize:"<<baPCMData.size()<<",remaing:"<<nRemaingBytes;
            }else{
                qint32 nPaddingBytes=baPCMData.size();
                memcpy(pcmBuffer+nPCMBufferLen,baPCMData.data(),nPaddingBytes);
                nPCMBufferLen+=nPaddingBytes;
                nRemaingBytes=0;
                //qDebug()<<"less:"<<nPCMBufferLen<<"pcmSize:"<<baPCMData.size()<<",remaing:"<<nRemaingBytes;
            }
        }else{
            nRemaingBytes=baPCMData.size();
        }

        //如果数据量够了，则写声卡.
        if(nPCMBufferLen>=BLOCK_SIZE)
        {
            snd_pcm_uframes_t pcmFrames=nPCMBufferLen/BYTES_PER_FRAME;
            qDebug()<<"playback: get pcm data:"<<nPCMBufferLen<<",pcmFrames:"<<(int)pcmFrames;
            //send data to pcm.
            while(snd_pcm_writei(pcmHandle,pcmBuffer,pcmFrames)<0)
            {
                snd_pcm_prepare(pcmHandle);
                //qDebug("<playback>:Buffer Underrun");
                gGblParam.m_nPlayUnderrun++;
            }

            //reset.
            nPCMBufferLen=0;
        }

        //如果有遗留数据.
        if(nRemaingBytes>0)
        {
            qint32 nOffsetIndex=baPCMData.size()-nRemaingBytes;
            memcpy(pcmBuffer,baPCMData.data()+nOffsetIndex,nRemaingBytes);
            nPCMBufferLen=nRemaingBytes;
            //qDebug()<<"reserve remaing bytes:"<<nRemaingBytes;
        }
#else
        snd_pcm_uframes_t pcmFrames=nPCMLen/BYTES_PER_FRAME;
        //qDebug()<<"playback: get pcm data:"<<baPCMData.size()<<",pcmFrames:"<<(int)pcmFrames;
        //send data to pcm.
        if(pcmFrames<=0)
        {
            qDebug()<<"<Error>:Audio PlayThread,error conversation at snd_pcm_uframes_t.";
            continue;
        }
        while(snd_pcm_writei(pcmHandle,pcmBuffer,pcmFrames)<0)
        {
            snd_pcm_prepare(pcmHandle);
            //qDebug("<playback>:Buffer Underrun");
            gGblPara.m_audio.m_nPlayUnderrun++;
        }
#endif
    }
    //do some clean here.
    /*
        If we want to stop playback, we can either use snd_pcm_drop or snd_pcm_drain.
        The first function will immediately stop the playback and drop pending frames.
        The latter function will stop after pending frames have been played.
    */
    /* Stop PCM device and drop pending frames */
    snd_pcm_drop(pcmHandle);

    /* Stop PCM device after pending frames have been played */
    //snd_pcm_drain(pcmHandle);

    delete [] pcmBuffer;

    qDebug()<<"<MainLoop>:PlaybackThread ends.";
    //set global request to exit flag to help other thread to exit.
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
    return;
}
quint64 ZAudioPlayThread::ZGetTimestamp()
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return now.tv_usec+(quint64)now.tv_sec*1000000;
}
