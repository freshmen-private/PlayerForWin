#ifndef MEDIADECODER_H
#define MEDIADECODER_H

#include <QQueue>
#include <QImage>
#include <QThread>
#include "glplayer.h"

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
    AVStream* aStream,* pStream;
    // const AVCodec* aCodec, * pCodec;

    SwsContext* swsC;

    PacketQueue* aqueue, * vqueue;
    uint8_t audio_decode_buffer[AVCODEC_MAX_AUDIO_FRAME_SIZE];
    SDL_AudioDeviceID audioID;
    int audio_buf_size, audio_buf_index;

    int videoStream, audioStream;
    bool audioExist;

    SDL_Thread* video_thread;
    double audio_time_base;
    double video_time_base;
    int64_t audio_pts;
    double video_clock, audio_clock;
    double clock;

    /// 跳转相关的变量
    int        seek_req; //跳转标志
    int64_t    seek_pos; //跳转的位置 -- 微秒
    int        seek_flag_audio;//跳转标志 -- 用于音频线程中
    int        seek_flag_video;//跳转标志 -- 用于视频线程中
    double     seek_time; //跳转的时间(秒)  值和seek_pos是一样的

    ///播放控制相关
    int  play;
    bool readFinished; //文件读取完毕
    bool readThreadFinished;
    bool videoThreadFinished;

    MediaDecoder *Decoder; //记录下这个类的指针  主要用于在线程里面调用激发信号的函数
}MediaState;

class MediaDecoder:public QThread
{
    Q_OBJECT
public:

    enum PlayerState
    {
        Playing,
        Pause,
        Stop
    };

public:
    explicit MediaDecoder();
    ~MediaDecoder();

    bool play();
    bool pause();
    bool stop(bool isWait = false);

    void seek();

    int64_t getTotalTime();
    double getCurrentTime();
    PlayerState getPlayState();
    void setPlayState(PlayerState s);

    void displayVideo(QImage img);
    QWidget *getVideoWidget(){return glplayer;}

signals:
    void sendOneFrame(QImage); //没获取到一帧图像 就发送此信号
    void sendFrameSize(QRect rect);
    void sendMediaDuration();

    void sig_Statechanged(MediaDecoder::PlayerState);
public slots:
    void getFileName(QString filename);

protected:
    void run();

private:
    QString mFileName;

    MediaState mMediastate;

    PlayerState mPlayerState; //播放状态
    GLPlayer* glplayer;
};

#endif // MEDIADECODER_H
