#ifndef ZCTLTHREAD_H
#define ZCTLTHREAD_H

#include <QThread>
#include <QJsonDocument>
class ZCtlThread : public QThread
{
    Q_OBJECT
public:
    ZCtlThread();
    qint32 ZStartThread();
    qint32 ZStopThread();
protected:
    void run();
private:
    QByteArray ZParseJson(const QJsonDocument &jsonDoc);
private:
    bool m_bExitFlag;
};

#endif // ZCTLTHREAD_H
