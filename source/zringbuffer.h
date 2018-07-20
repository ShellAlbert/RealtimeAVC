#ifndef ZRINGBUFFERBYTEARRAY_H
#define ZRINGBUFFERBYTEARRAY_H

#include <QByteArray>
#include <QVector>
#include <QSemaphore>
class ZRingBufferElement
{
public:
    qint8 *m_pData;
    qint32 m_nLen;
};
class ZRingBuffer
{
public:
    ZRingBuffer(qint32 nMaxElement,qint32 nEachBytes);
    ~ZRingBuffer();
    bool ZIsFull();
    bool ZIsEmpty();
    qint32 ZPutElement(const qint8 *buffer,qint32 len);
    qint32 ZGetElement(qint8 *buffer,qint32 len);

    qint32 ZGetValidNum();

    bool ZTryAcquire(qint32 num=1);
    void ZAcquire(qint32 num=1);
    void ZRelease(qint32 num=1);
private:
    QVector<ZRingBufferElement*> m_vec;
    qint32 m_nMaxElement;
    qint32 nEachBytes;
    qint32 m_nValidElement;
private:
    //read & write pos.
    qint32 m_nRdPos;
    qint32 m_nWrPos;
public:
    QSemaphore *m_semaUsed;
    QSemaphore *m_semaFree;
};

#endif // ZRINGBUFFERBYTEARRAY_H
