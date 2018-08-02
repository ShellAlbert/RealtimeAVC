#ifndef ZTCP2UARTFORWARDTHREAD_H
#define ZTCP2UARTFORWARDTHREAD_H

#include <QThread>
//串口透传，Android(TCP) <---> UART(STM32主控板).
//用于Android手工调节电机，测距，设置参数等.
class ZTcp2UartForwardThread : public QThread
{
    Q_OBJECT
public:
    ZTcp2UartForwardThread();
    qint32 ZStartThread();
    qint32 ZStopThread();

    bool ZIsExitCleanup();
signals:
    void ZSigThreadFinished();
protected:
    void run();
private:
    bool m_bExitFlag;
    bool m_bCleanup;
};

#endif // ZTCP2UARTFORWARDTHREAD_H
