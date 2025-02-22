QT       += core gui
QT       += opengl
QT       += openglwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += _DEBUG_

INCLUDEPATH += D:/environment/ffmpeg/include \
               D:/environment/SDL2/include \

LIBS += D:/environment/ffmpeg/lib/avcodec.lib \
        D:/environment/ffmpeg/lib/avdevice.lib \
        D:/environment/ffmpeg/lib/avfilter.lib \
        D:/environment/ffmpeg/lib/avformat.lib \
        D:/environment/ffmpeg/lib/avutil.lib \
        D:/environment/ffmpeg/lib/postproc.lib \
        D:/environment/ffmpeg/lib/swresample.lib \
        D:/environment/ffmpeg/lib/swscale.lib \
        D:/environment/SDL2/lib/x64/SDL2.lib

SOURCES += \
    glplayer.cpp \
    imageplayer.cpp \
    main.cpp \
    mediadecoder.cpp \
    videoplayer.cpp

HEADERS += \
    glplayer.h \
    imageplayer.h \
    mediadecoder.h \
    videoplayer.h

FORMS += \
    imageplayer.ui \
    videoplayer.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
