#ifndef ZTCPSERVER_H
#define ZTCPSERVER_H

#include <QTcpSocket>
#include <QTcpServer>
#include <QThread>
#include <QMap>
#include <QQueue>
#include <QSemaphore>
class ZTcpDumpThread:public QThread
{
    Q_OBJECT
public:
    ZTcpDumpThread();
    qint32 ZStartThread(QQueue<QByteArray> *queue,QSemaphore *semaUsed,QSemaphore *semaFree);
    qint32 ZStopThread();
signals:
    void ZSigThreadFinished();
protected:
    void run();
private:
    QQueue<QByteArray> *m_queue;
    QSemaphore *m_semaUsed;
    QSemaphore *m_semaFree;
    bool m_bExitFlag;
};
#endif // ZTCPSERVER_H
