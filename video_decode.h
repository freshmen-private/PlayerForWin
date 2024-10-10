#ifndef VIDEO_DECODE_H
#define VIDEO_DECODE_H
#include <QQueue>
#include <QImage>
#include <QThread>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavutil/imgutils.h>
}

class Video_decode
{
public:
    Video_decode();
    ~Video_decode();
    void decode(QString& fileName);
    void saveFrame(AVFrame *pFrame, int width, int height,int index);
    QQueue<QImage> image_queue;
};

#endif // VIDEO_DECODE_H
