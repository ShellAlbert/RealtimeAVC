#include "zaudiocapturethread.h"
#include "../zgblpara.h"
#include <QDebug>
#include <unistd.h>
#include <sys/time.h>
#include <QFile>
#include <QApplication>
ZAudioCaptureThread::ZAudioCaptureThread(QString capDevName,bool bDump2WavFile)
{
    this->m_capDevName=capDevName;
    this->m_bDump2WavFile=bDump2WavFile;

    this->m_nTotalBytes=0;

    this->m_bCleanup=true;
}
ZAudioCaptureThread::~ZAudioCaptureThread()
{

}
qint32 ZAudioCaptureThread::ZWriteWavHead2File()
{
#if 0
    WAVE_HEAD wavehead;
    /*以下wave 文件头赋值*/
    memcpy(wavehead.riff_head, "RIFF", 4);
    wavehead.riffdata_len = 1*SampleRate*ChannelNum*SampleBits/8 + 36;
    memcpy(wavehead.wave_head, "WAVE", 4);
    memcpy(wavehead.fmt_head, "fmt ", 4);
    wavehead.fmtdata_len = 16;
    wavehead.format_tag = 1;
    wavehead.channels = ChannelNum;
    wavehead.samples_persec = SampleRate;
    wavehead.bytes_persec=SampleRate*ChannelNum*SampleBits/8;
    wavehead.block_align=ChannelNum*SampleBits/8;
    wavehead.bits_persec=SampleBits;
    memcpy(wavehead.data_head, "data", 4);
    wavehead.data_len = 1*SampleRate*ChannelNum*SampleBits/8;
    /*以上wave 文件头赋值*/

    if(write(this->m_fd, &wavehead, sizeof(wavehead))<0)//写入wave文件的文件头
    {
        perror("write to sound'head wrong!!");
        return -1;
    }
#endif
    return 0;
}
qint32 ZAudioCaptureThread::ZUpdateWavHead2File()
{
    int nRiffDataLen=this->m_nTotalBytes-8;
    int nPCMDataLen=this->m_nTotalBytes-44;
    //更新wav文件头，[0x04~0x08],4 bytes.
    //该长度=文件总长-8
    lseek(this->m_fd,4,SEEK_SET);
    write(this->m_fd,&nRiffDataLen,sizeof(nRiffDataLen));

    //更新WAV数据体的长度.
    //实际PCM数据体的长度
    //大小为文件总长度减去44字节的WAV头
    lseek(this->m_fd,40,SEEK_SET);
    write(this->m_fd,&nPCMDataLen,sizeof(nPCMDataLen));

    return 0;
}
qint32 ZAudioCaptureThread::ZStartThread(ZRingBuffer *rbNoise)
{
    this->m_rbNoise=rbNoise;

    this->m_bExitFlag=false;

    this->m_nTotalBytes=0;
    this->m_nEscapeMsec=0;
    this->m_nEscapeSec=0;

    this->start();
    return 0;
}
qint32 ZAudioCaptureThread::ZStopThread()
{
    this->m_bExitFlag=true;
    return 0;
}
bool ZAudioCaptureThread::ZIsExitCleanup()
{
    return this->m_bCleanup;
}
void ZAudioCaptureThread::run()
{
    /*
        The most important ALSA interfaces to the PCM devices are the "plughw" and the "hw" interface.
        If you use the "plughw" interface, you need not care much about the sound hardware.
        If your soundcard does not support the sample rate or sample format you specify,
        your data will be automatically converted.
        This also applies to the access type and the number of channels.
        With the "hw" interface,
        you have to check whether your hardware supports the configuration you would like to use.
    */
    /* Name of the PCM device, like plughw:0,0          */
    /* The first number is the number of the soundcard, */
    /* the second number is the number of the device.   */

    unsigned int nSampleRate=SAMPLE_RATE;
    char *inputBuffer=NULL;	// Input buffer for driver to read into
    do{
        // Input and output driver variables
        snd_pcm_t	*pcmHandle;
        snd_pcm_hw_params_t *hwparams;
        snd_pcm_uframes_t pcmFrames;

        int pcmBlkSize= BLOCK_SIZE;	// Raw input or output frame size in bytes
        pcmFrames=pcmBlkSize/BYTES_PER_FRAME;// Convert bytes to frames

        // Now we can open the PCM device:
        /* Open PCM. The last parameter of this function is the mode. */
        /* If this is set to 0, the standard mode is used. Possible   */
        /* other values are SND_PCM_NONBLOCK and SND_PCM_ASYNC.       */
        /* If SND_PCM_NONBLOCK is used, read / write access to the    */
        /* PCM device will return immediately. If SND_PCM_ASYNC is    */
        /* specified, SIGIO will be emitted whenever a period has     */
        /* been completely processed by the soundcard.                */
        if (snd_pcm_open(&pcmHandle,(char*)gGblPara.m_audio.m_capCardName.toStdString().c_str(),SND_PCM_STREAM_CAPTURE,0)<0)
        {
            qDebug()<<"<Error>:Audio CapThread,error to open pcm device "<<gGblPara.m_audio.m_capCardName;
            //set global request to exit flag to cause other threads to exit.
            gGblPara.m_bGblRst2Exit=true;
            return;
        }

        /*
            Before we can write PCM data to the soundcard,
            we have to specify access type, sample format, sample rate, number of channels,
            number of periods and period size.
            First, we initialize the hwparams structure with the full configuration space of the soundcard.
        */
        /* Init hwparams with full configuration space */
        snd_pcm_hw_params_alloca(&hwparams);
        if(snd_pcm_hw_params_any(pcmHandle,hwparams)<0)
        {
            qDebug()<<"<error>:Audio CapThread,Cannot configure this PCM device.";
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
            qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params_set_access().";
            //set global request to exit flag to cause other threads to exit.
            gGblPara.m_bGblRst2Exit=true;
            return;
        }

        /* Set sample format */
        if(snd_pcm_hw_params_set_format(pcmHandle,hwparams,SND_PCM_FORMAT_S16_LE)<0)
        {
            qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params_set_format().";
            //set global request to exit flag to cause other threads to exit.
            gGblPara.m_bGblRst2Exit=true;
            return;
        }

        /* Set sample rate. If the exact rate is not supported */
        /* by the hardware, use nearest possible rate.         */
        unsigned int nNearSampleRate=nSampleRate;
        if(snd_pcm_hw_params_set_rate_near(pcmHandle,hwparams,&nNearSampleRate,0u)<0)
        {
            qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params_set_rate_near().";
            //set global request to exit flag to cause other threads to exit.
            gGblPara.m_bGblRst2Exit=true;
            return;
        }
        if(nNearSampleRate!=nSampleRate)
        {
            qDebug()<<"<Warning>:Audio CapThread,the sampled rate "<<nSampleRate<<" Hz is not supported by hardware.";
            qDebug()<<"<Warning>:Using "<<nNearSampleRate<<" Hz instead.";
        }

        /* Set number of channels */
        if(snd_pcm_hw_params_set_channels(pcmHandle,hwparams,CHANNELS_NUM)<0)
        {
            qDebug()<<"<Error>:CapThread,error at snd_pcm_hw_params_set_channels().";
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
        //qDebug("Requesting period count of %d\n", request_periods);
        //	Restrict a configuration space to contain only one periods count
        if(snd_pcm_hw_params_set_periods_near(pcmHandle,hwparams,&periods,&dir)<0)
        {
            qDebug("<Error>:Audio CapThread,error at setting periods.\n");
            //set global request to exit flag to cause other threads to exit.
            gGblPara.m_bGblRst2Exit=true;
            return;
        }
        if(request_periods!=periods)
        {
            qDebug("<Warning>:Audio CapThread,requested %d periods,but recieved %d.\n", request_periods, periods);
        }

        /*
            The unit of the buffersize depends on the function.
            Sometimes it is given in bytes, sometimes the number of frames has to be specified.
            One frame is the sample data vector for all channels.
            For 16 Bit stereo data, one frame has a length of four bytes.
        */
        if(snd_pcm_hw_params_set_buffer_size_near(pcmHandle,hwparams,&pcmFrames)<0)
        {
            qDebug()<<"<Error>:CapThread,error at snd_pcm_hw_params_set_buffer_size_near().";
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
            qDebug()<<"<Error>:Audio CapThread,error at snd_pcm_hw_params().";
            //set global request to exit flag to cause other threads to exit.
            gGblPara.m_bGblRst2Exit=true;
            return;
        }

        //allocate buffer to store input pcm data.
        inputBuffer=new char[pcmBlkSize];
        //the main-loop.
        qDebug()<<"<MainLoop>:CaptureThread starts.";
        this->m_bCleanup=false;
        while(!gGblPara.m_bGblRst2Exit && !this->m_bExitFlag)
        {
            // Read capture buffer from ALSA input device.
            if(snd_pcm_readi(pcmHandle,inputBuffer,pcmFrames)<0)
            {
                snd_pcm_prepare(pcmHandle);
                //qDebug( "<cap>:Buffer Overrun");
                gGblPara.m_audio.m_nCapOverrun++;
                continue;
            }
            //try to put original pcm data into noise queue.
            this->m_rbNoise->m_semaFree->acquire();
            this->m_rbNoise->ZPutElement((qint8*)inputBuffer,pcmBlkSize);
            this->m_rbNoise->m_semaUsed->release();
        }
    }while(0);

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

    if(inputBuffer!=NULL)
    {
        delete [] inputBuffer;
    }

    qDebug()<<"<MainLoop>:CaptureThread ends.";
    //set global request to exit flag to help other thread to exit.
    gGblPara.m_bGblRst2Exit=true;
    emit this->ZSigThreadFinished();
    this->m_bCleanup=true;
    return;
}
quint64 ZAudioCaptureThread::ZGetTimestamp()
{
    struct timeval now;
    gettimeofday(&now,NULL);
    return now.tv_usec+(quint64)now.tv_sec*1000000;
}

