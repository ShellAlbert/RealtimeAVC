#ifndef ZAUDIOPLAYTHREAD_H
#define ZAUDIOPLAYTHREAD_H

#include <QThread>
#include <QTimer>
#include <QQueue>
#include <QSemaphore>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>
#include <zringbuffer.h>
class ZAudioPlayThread : public QThread
{
    Q_OBJECT
public:
    ZAudioPlayThread(QString playCardName);
    ~ZAudioPlayThread();

    qint32 ZStartThread(ZRingBuffer *rbPlay);
    qint32 ZStopThread();
    bool ZIsExitCleanup();
protected:
    void run();
signals:
    void ZSigThreadFinished();
private:
    quint64 ZGetTimestamp();
private:
    bool m_bExitFlag;
    bool m_bCleanup;

    QTimer *m_timerPlay;
private:
    QString m_playCardName;
    snd_pcm_t *m_pcmHandle;
    ZRingBuffer *m_rbPlay;
    char *m_pcmBuffer;
};

#endif // ZAUDIOPLAYTHREAD_H
