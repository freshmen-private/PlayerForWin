#ifndef MEDIADECODER_H
#define MEDIADECODER_H

#include <QQueue>
#include <QImage>
#include <QThread>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>

#include <SDL.h>
#include <SDL_audio.h>
#include <SDL_types.h>
#include <SDL_name.h>
#include <SDL_main.h>
#include <SDL_config.h>
}

#define VIDEO_PICTURE_QUEUE_SIZE 1
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio

#define MAX_AUDIO_SIZE (25 * 16 * 1024)
#define MAX_VIDEO_SIZE (25 * 256 * 1024)

typedef struct AVPacketlist{
    AVPacket pkt;
    AVPacketlist* next;
}AVPacketlist;

typedef struct PacketQueue {
    AVPacketlist *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

class MediaDecoder;

typedef struct MediaState{
    AVFormatContext* FC;
    AVCodecContext* aCodecC, * vCodecC;

    SwsContext* swsC;

    PacketQueue* aqueue, * vqueue;
    uint8_t audio_decode_buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    SDL_AudioDeviceID audioID;
    int audio_buf_size, audio_buf_index;

    int videoStream, audioStream;
    bool audioExist;

    SDL_Thread* video_thread;
    double audio_time_base;
    int64_t audio_pts;
    double time_base;
    bool isPause;

    MediaDecoder *Decoder; //记录下这个类的指针  主要用于在线程里面调用激发信号的函数
}MediaState;

class MediaDecoder:public QThread
{
    Q_OBJECT
public:
    explicit MediaDecoder();
    ~MediaDecoder();

    void startPlay();

    void disPlayVideo(QImage image);

signals:
    void sendOneFrame(QImage); //没获取到一帧图像 就发送此信号
public slots:
    void getFileName(QString filename);

protected:
    void run();

private:
    QString mFileName;
    MediaState mMediastate;
};

#endif // MEDIADECODER_H
