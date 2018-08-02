#ifndef ZCTLTHREAD_H
#define ZCTLTHREAD_H

#include <QThread>
#include <QJsonDocument>
#include <QVector>
#include <QTcpSocket>
#include <QTcpServer>
#if 1
class ZJsonThread:public QThread
{
    Q_OBJECT
public:
    ZJsonThread(qintptr handle,QObject *parent=nullptr);
protected:
    void run();
private slots:
    void ZSlotReadSocketData();
    void ZSlotSocketErr(QAbstractSocket::SocketError err);
private:
    QByteArray ZParseJson(const QJsonDocument &jsonDoc);
private:
    qintptr m_handle;
    QTcpSocket *m_tcpSocket;
private:
    qint32 m_nJsonLen;
};
class ZCtlServer:public QTcpServer
{
    Q_OBJECT
public:
    ZCtlServer();
    qint32 ZStartServer();
protected:
    void incomingConnection(qintptr socketDescriptor);
private slots:
    void ZSlotJsonThreadFinished();
private:
    ZJsonThread *m_jsonThread;
};
#endif

#if 0
class ZCtlThread : public QThread
{
    Q_OBJECT
public:
    ZCtlThread();
    qint32 ZStartThread();
    qint32 ZStopThread();
    bool ZIsExitCleanup();
signals:
    void ZSigThreadExited();
protected:
    void run();
private slots:
    void ZSlotAcceptErr(QAbstractSocket::SocketError err);
    void ZSlotNewConnection();
    void ZSlotReadSocketData();
    void ZSlotSocketErr(QAbstractSocket::SocketError err);

private:
    QByteArray ZParseJson(const QJsonDocument &jsonDoc);
private:
    bool m_bExitFlag;
    bool m_bCleanup;
private:
    QTcpServer *m_tcpServer;
    QTcpSocket *m_tcpSocket;
private:
    qint32 m_nJsonLen;
};
#endif

#endif // ZCTLTHREAD_H
