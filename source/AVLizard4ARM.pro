QT += gui


#CONFIG -= app_bundle  gui

QT += core serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TARGET = IMP4ARM.bin
TEMPLATE = app

# The following define makes your compiler emit warnings if you useconsole
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    zcamdevice.cpp \
    zfiltecamdev.cpp \
    zimgcapthread.cpp \
    zimgprocessthread.cpp \
    zalgorithmset.cpp \
    zhistogram.cpp \
    zimgfeaturedetectmatch.cpp \
    zgblpara.cpp \
    zmaintask.cpp \
    zservicethread.cpp \
    zimgdisplayer.cpp \
    udt4/src/api.cpp \
    udt4/src/buffer.cpp \
    udt4/src/cache.cpp \
    udt4/src/ccc.cpp \
    udt4/src/channel.cpp \
    udt4/src/common.cpp \
    udt4/src/core.cpp \
    udt4/src/epoll.cpp \
    udt4/src/list.cpp \
    udt4/src/md5.cpp \
    udt4/src/packet.cpp \
    udt4/src/queue.cpp \
    udt4/src/window.cpp \
    zserverthread.cpp

HEADERS += \
    zcamdevice.h \
    zfiltecamdev.h \
    zgblpara.h \
    zimgcapthread.h \
    zimgprocessthread.h \
    zalgorithmset.h \
    zhistogram.h \
    zimgfeaturedetectmatch.h \
    zmaintask.h \
    zimgdisplayer.h \
    zservicethread.h \
    udt4/src/api.h \
    udt4/src/buffer.h \
    udt4/src/cache.h \
    udt4/src/ccc.h \
    udt4/src/channel.h \
    udt4/src/common.h \
    udt4/src/core.h \
    udt4/src/epoll.h \
    udt4/src/list.h \
    udt4/src/md5.h \
    udt4/src/packet.h \
    udt4/src/queue.h \
    udt4/src/udt.h \
    udt4/src/window.h \
    zserverthread.h

INCLUDEPATH += /opt/EmbedSky/gcc-linaro-5.3-2016.02-x86_64_arm-linux-gnueabihf/arm-linux-gnueabihf/libc/usr/include
#INCLUDEPATH += /home/zhangshaoyan/armbuild/opencv3.4.1.for.arm/include
INCLUDEPATH += /home/zhangshaoyan/libopencv341/include

INCLUDEPATH += /home/zhangshaoyan/armbuild/libjpeg4arm/include

#LIBS += -L/home/zhangshaoyan/armbuild/opencv3.4.1.for.arm/lib -lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core -lopencv_xfeatures2d
LIBS += -L/home/zhangshaoyan/libopencv341/lib -lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core

LIBS += -L/home/zhangshaoyan/armbuild/libjpeg4arm/lib -ljpeg

RESOURCES += \
    resource.qrc
