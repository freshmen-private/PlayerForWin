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
#include <libavutil/time.h>
}

class Video_decode:public QThread
{
    Q_OBJECT
public:
    Video_decode();
    ~Video_decode();
    void decode(QString& fileName);
    void saveFrame(AVFrame *pFrame, int width, int height,int index);
    QQueue<QImage> image_queue;
signals:
    void sendOneFrame(QImage&);
public slots:
    void getFileName(QString fileName);
protected:
    void run();
private:
    QString FileName;
    double Time_Base;
    double currentTime;
};

#endif // VIDEO_DECODE_H
