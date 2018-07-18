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

class ZAudioPlayThread : public QThread
{
    Q_OBJECT
public:
    ZAudioPlayThread(QString playCardName);
    ~ZAudioPlayThread();

    qint32 ZStartThread(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZStopThread();
    bool ZIsRunning();
protected:
    void run();
signals:
    void ZSigThreadFinished();
private:
    quint64 ZGetTimestamp();
private:
    bool m_bExitFlag;

    QTimer *m_timerPlay;
private:
    QString m_playCardName;
    snd_pcm_t *m_pcmHandle;

    QQueue<QByteArray> *m_queue;
    QSemaphore *m_semaUsed;
    QSemaphore *m_semaFree;

    char *m_pcmBuffer;
};

#endif // ZAUDIOPLAYTHREAD_H
