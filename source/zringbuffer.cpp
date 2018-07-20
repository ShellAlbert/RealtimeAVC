#include "zringbuffer.h"
#include <QDebug>
ZRingBuffer::ZRingBuffer(qint32 nMaxElement,qint32 nEachBytes)
{
    for(qint32 i=0;i<nMaxElement;i++)
    {
        ZRingBufferElement *element=new ZRingBufferElement;
        element->m_pData=new qint8[nEachBytes];
        element->m_nLen=0;
        this->m_vec.append(element);
    }
    this->m_nMaxElement=nMaxElement;
    this->m_nValidElement=0;
    this->m_nRdPos=0;
    this->m_nWrPos=0;
    //create semaphore.
    this->m_semaUsed=new QSemaphore(0);
    this->m_semaFree=new QSemaphore(nMaxElement);
}
ZRingBuffer::~ZRingBuffer()
{
    for(qint32 i=0;i<this->m_nMaxElement;i++)
    {
        ZRingBufferElement *element=this->m_vec.at(i);
        delete [] element->m_pData;
        delete element;
    }
    this->m_vec.clear();

    delete this->m_semaUsed;
    delete this->m_semaFree;
}
//写入时判断队列是否为满.
bool ZRingBuffer::ZIsFull()
{
    if(this->m_nValidElement==this->m_nMaxElement)
    {
        return true;
    }else{
        return false;
    }
}
//读取时判断队列是否为空.
bool ZRingBuffer::ZIsEmpty()
{
    if(0==this->m_nValidElement)
    {
        return true;
    }else{
        return false;
    }
}

qint32 ZRingBuffer::ZPutElement(const qint8 *buffer,qint32 len)
{
    //写入数据.
    ZRingBufferElement *element=this->m_vec.at(this->m_nWrPos);
    memcpy(element->m_pData,buffer,len);
    element->m_nLen=len;

    //调整写指针.
    //wrPos始终指向下一个要写入的地址.
    this->m_nWrPos++;
    if(this->m_nWrPos==this->m_nMaxElement)
    {
        this->m_nWrPos=0;
    }
    //放入一个元素，有效个数加1.
    this->m_nValidElement++;

    return 0;
}
qint32 ZRingBuffer::ZGetElement(qint8 *buffer,qint32 len)
{
    ZRingBufferElement *element=this->m_vec.at(this->m_nRdPos);
    if(element->m_nLen>len)
    {
        qDebug()<<"<RingBuffer>:get element buffer is not big enough.";
        return -1;
    }
    //rdPos始终指定当前可读取的地址.
    memcpy(buffer,element->m_pData,element->m_nLen);

    //调整读指针.
    this->m_nRdPos++;
    if(this->m_nRdPos==this->m_nMaxElement)
    {
        this->m_nRdPos=0;
    }

    //取出一个元素，有效个数减1.
    this->m_nValidElement--;
    return element->m_nLen;
}
qint32 ZRingBuffer::ZGetValidNum()
{
    return this->m_nValidElement;
}
