#ifndef ZFILTECAMDEV_H
#define ZFILTECAMDEV_H

#include <QVector>
#include <QString>
class ZFilteCAMDev
{
public:
    ZFilteCAMDev();

    QStringList ZGetCAMDevList();
};

#endif // ZFILTECAMDEV_H
