#include "zfiltecamdev.h"
#include <QDir>
#include <iostream>
#include <QDebug>
ZFilteCAMDev::ZFilteCAMDev()
{

}
QStringList ZFilteCAMDev::ZGetCAMDevList()
{
    //already exists inode list.
    QString extractNode;
//    extractNode.append("video0");
//    extractNode.append("video1");
//    extractNode.append("video16");
//    extractNode.append("video17");

    //fetch all videoX node names.
    QStringList nodeNameList;
    QDir dir("/dev");
    QStringList fileList=dir.entryList(QDir::System);
    for(qint32 i=0;i<fileList.size();i++)
    {
        QString nodeName=fileList.at(i);
        if(nodeName.startsWith("video"))
        {
            nodeNameList.append(nodeName);
        }
    }

    //filter out the real capture device.
    QStringList lstRealDev;
    for(qint32 i=0;i<nodeNameList.size();i++)
    {
        if(!extractNode.contains(nodeNameList.at(i)))
        {
            lstRealDev.append(nodeNameList.at(i));
        }
    }
    return lstRealDev;
}
