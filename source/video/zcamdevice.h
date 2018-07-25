#ifndef ZCAMDEVICE_H
#define ZCAMDEVICE_H
#include <vector>
#include <iostream>
#include <QObject>
typedef struct{
    void *pStart;
    size_t nLength;
}IMGBufferStruct;
using namespace std;
class ZCAMDevice:public QObject
{
    Q_OBJECT
public:
    ZCAMDevice(QString devName,qint32 nWidth,qint32 nHeight,qint32 nFps,bool bMainCamera,QObject*parent=nullptr);
    ~ZCAMDevice();

    int ZOpenCAM();
    int ZCloseCAM();
    int ZInitCAM();
    int ZUnInitCAM();
    int ZInitMAP();
    int ZStartCapture();
    int ZStopCapture();
    int ZGetFrame(void **pBuffer,size_t *nLen);
    int ZUnGetFrame();

    int ZGetFrameRate();
signals:
    void ZSigMsg(const QString &msg,const qint32 &type);
private:
    int m_fd;
    QString m_devName;

    IMGBufferStruct* m_IMGBuffer;
    unsigned int m_nBufferCount;
    int m_nIndex;

    int m_nImgWidth;
    int m_nImgHeight;

    int m_nFPS;//the frame rates.

    int m_nCapTotal;//capture counter.

    //predefined parameters.
    qint32 m_nPredefinedWidth;
    qint32 m_nPredefinedHeight;
    qint32 m_nPredefinedFps;
private:
    bool m_bMainCamera;
};

#endif // ZCAMDEVICE_H
