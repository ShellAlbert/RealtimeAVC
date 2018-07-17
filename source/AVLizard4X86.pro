QT += gui


#CONFIG -= app_bundle  gui

QT += core serialport network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
TARGET = AVLizard.bin
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
    zimgdisplayer.cpp \
    zh264encthread.cpp \
    ztcp2uartforwardthread.cpp \
    zvideotxthread.cpp

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
    zh264encthread.h \
    ztcp2uartforwardthread.h \
    zvideotxthread.h

RESOURCES += \
    resource.qrc

LIBS += -L/usr/local/lib64 -lopencv_dnn -lopencv_ml -lopencv_objdetect -lopencv_shape -lopencv_stitching -lopencv_superres -lopencv_videostab -lopencv_calib3d -lopencv_features2d -lopencv_highgui -lopencv_videoio -lopencv_imgcodecs -lopencv_video -lopencv_photo -lopencv_imgproc -lopencv_flann -lopencv_core -lopencv_xfeatures2d

LIBS += -ljpeg -lx264

INCLUDEPATH += /home/zhangshaoyan/NoiseReduction/jrtplib-3.11.1/src  /home/zhangshaoyan/NoiseReduction/jrtplib-3.11.1/build/src
LIBS += -L/home/zhangshaoyan/NoiseReduction/jrtplib-3.11.1/build/src -ljrtp
