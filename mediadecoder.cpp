#include "mediadecoder.h"
#include <QDebug>
#include <stdio.h>
#include <QDebug>
#define SDL_AUDIO_BUFFER_SIZE 1024
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
    if(q->mutex == NULL || q->cond == NULL)
    {
        qDebug()<<"mutex and cond create failed";
    }
    q->size = 0;
    q->nb_packets = 0;
    q->first_pkt = NULL;
    q->last_pkt = NULL;
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    AVPacketlist *pkt1;
    if (av_packet_ref(pkt, pkt) < 0) {
        return -1;
    }
    pkt1 = (AVPacketlist*)av_malloc(sizeof(AVPacketlist));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;
    if(q->mutex == NULL)
    {
        qDebug()<<"put mutex error";
    }
    SDL_LockMutex(q->mutex);
    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    if(q->cond == NULL)
    {
        qDebug()<<"put cond error";
    }
    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);
    return 0;
}
static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block) {
    AVPacketlist *pkt1 = NULL;
    int ret;
    if(q->mutex == NULL)
    {
        qDebug()<<"get mutex error";
    }
    SDL_LockMutex(q->mutex);
    for (;;) {
        pkt1 = q->first_pkt;
        if (pkt1) {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            av_packet_ref(pkt, &pkt1->pkt);
            // *pkt = pkt1->pkt;
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            if(q->cond == NULL)
            {
                qDebug()<<"put cond error";
            }
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}
static int audio_decode_frame(MediaState *is)
{
    AVPacket pkt;
    int data_size, n;
    static AVFrame* aFrame = av_frame_alloc();
    //qDebug()<<"audio avFrame alloc success";
    double pts;
    for(;;)
    {
        data_size = 0;
        if(packet_queue_get(is->aqueue, &pkt, 1) < 0)
        {
            return -1;
        }
        qDebug()<<"audio_pts = "<<pkt.pts;
        if (pkt.pts != AV_NOPTS_VALUE) {
            is->audio_clock = av_q2d(is->aStream->time_base) * pkt.pts;
        }
        qDebug()<<"is->aStream->time_base = "<<is->aStream->time_base.den<<" "<<is->aStream->time_base.num;
        qDebug()<<"is->audio_clock = "<<is->audio_clock;
        //qDebug()<<"packet.dts = "<<pkt.dts;
        //qDebug()<<"packet.pts = "<<pkt.pts;
        int ret = avcodec_send_packet(is->aCodecC, &pkt);
        if( ret < 0 ) {
            printf("Error in decoding audio frame. %d\n", ret);
            exit(0);
        }
        // qDebug()<<"avcodec_send_packet ret = " << ret;
        while(avcodec_receive_frame(is->aCodecC, aFrame) >= 0)
        {
            //qDebug()<<"aFrame->nb_samples = "<<aFrame->nb_samples;
            int in_samples = aFrame->nb_samples;
            int bytes_per_sample;
            switch(is->aCodecC->sample_fmt)
            {
            case AV_SAMPLE_FMT_FLT:       // 32 位浮点数
            case AV_SAMPLE_FMT_FLTP:     // 平面 32 位浮点数
                bytes_per_sample = sizeof(float); // 4 字节
                break;
            case AV_SAMPLE_FMT_S16:       // 16 位整型
            case AV_SAMPLE_FMT_S16P:     // 平面 16 位整型
                bytes_per_sample = sizeof(int16_t); // 2 字节
                break;
            case AV_SAMPLE_FMT_S32:       // 32 位整型
            case AV_SAMPLE_FMT_S32P:     // 平面 32 位整型
                bytes_per_sample = sizeof(int32_t); // 4 字节
                break;
            // 其他样本格式可以根据需要添加
            default:
                qDebug() << "不支持的样本格式";
                // 处理错误情况
                return -1;
            }
            //qDebug()<<"channels = "<<aFrame->ch_layout.nb_channels;
            float *sample_buffer = (float*)malloc(aFrame->linesize[0]);
            memset(sample_buffer, 0, aFrame->linesize[0]);

            int i=0;
            float *inputChannel0 = (float*)(aFrame->extended_data[0]);

            // // Mono
            if( aFrame->ch_layout.nb_channels == 1 ) {
                for( i=0; i<in_samples; i++ ) {
                    sample_buffer[i] = *inputChannel0++;
                }
            } else { // Stereo
                float* inputChannel1 = (float*)(aFrame->extended_data[1]);
                for( i=0; i<in_samples; i++) {
                    sample_buffer[i*2] = ((*inputChannel0++));
                    sample_buffer[i*2+1] = ((*inputChannel1++));
                }
            }
            memcpy(is->audio_decode_buffer, (uint8_t*)sample_buffer, aFrame->linesize[0]);
            data_size += aFrame->linesize[0];
            if (aFrame->nb_samples <= 0)
            {
                continue;
            }
        }
        if(pkt.data)
            av_packet_unref(&pkt);
        return data_size;
    }

}
static void audio_callback(void *userdata, Uint8 *stream, int len) {
    //qDebug()<<"audio callback";
    MediaState *is = (MediaState *) userdata;
    int len1, audio_data_size;
    //double pts;
    /*   len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 */
    while (len > 0) {
        /*  audio_buf_index 和 audio_buf_size 标示我们自己用来放置解码出来的数据的缓冲区，*/
        /*   这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size的时候意味着我*/
        /*   们的缓冲为空，没有数据可供copy，这时候需要调用audio_decode_frame来解码出更 */
         /*   多的桢数据 */
        if (is->audio_buf_index >= is->audio_buf_size) {
            // qDebug()<<"decode frame";
            audio_data_size = audio_decode_frame(is);
            // qDebug()<<"decode frame finish";
            /* audio_data_size < 0 标示没能解码出数据，我们默认播放静音 */
            if (audio_data_size < 0) {
                /* silence */
                is->audio_buf_size = len;
                /* 清零，静音 */
                memset(is->audio_decode_buffer, 0, is->audio_buf_size);
            } else {
                is->audio_buf_size = audio_data_size;
            }
            is->audio_buf_index = 0;
        }
        /*  查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy */
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        memcpy(stream, (uint8_t *) is->audio_decode_buffer + is->audio_buf_index, len1);
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
}
// static double get_audio_clock(MediaState *is)
// {
//     double pts;
//     int hw_buf_size, bytes_per_sec, n;
//     pts = is->audio_clock; /* maintained in the audio thread */
//     hw_buf_size = is->audio_buf_size - is->audio_buf_index;
//     bytes_per_sec = 0;
//     n = is->codec->ch_layout.nb_channels * 2;
//     if(is->audio_st)
//     {
//         bytes_per_sec = is->codec->sample_rate * n;
//     }
//     if(bytes_per_sec)
//     {
//         pts -= (double)hw_buf_size / bytes_per_sec;
//     }
//     return pts;
// }
static double synchronize_video(MediaState *is, AVFrame *src_frame, double pts) {
    double frame_delay;
    if (pts != 0) {
        /* if we have pts, set video clock to it */
        is->video_clock = pts;
    } else {
        /* if we aren't given a pts, set it to the clock */
        pts = is->video_clock;
    }
    /* update the video clock */
    frame_delay = av_q2d(is->vCodecC->time_base);
    /* if we are repeating a frame, adjust clock accordingly */
    frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
    is->video_clock += frame_delay;
    return pts;
}
// this function is used to configure audioDevice
int audio_stream_component_open(MediaState *is)
{
    SDL_AudioSpec wanted_spec, spec;
    /*  SDL支持的声道数为 1, 2, 4, 6 */
    /*  后面我们会使用这个数组来纠正不支持的声道数目 */
    const int next_nb_channels[] = { 0, 0, 1, 6, 2, 6, 4, 6 };

    wanted_spec.channels = is->aCodecC->ch_layout.nb_channels;
    wanted_spec.freq = is->aCodecC->sample_rate;
    if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
        //fprintf(stderr,"Invalid sample rate or channel count!\n");
        return -1;
    }
    wanted_spec.format = AUDIO_S16SYS; // 具体含义请查看“SDL宏定义”部分
    wanted_spec.silence = 0;            // 0指示静音
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;  // 自定义SDL缓冲区大小
    wanted_spec.callback = audio_callback;        // 音频解码的关键回调函数
    wanted_spec.userdata = is;                    // 传给上面回调函数的外带数据
    if(SDL_OpenAudio(&wanted_spec, &spec) < 0)
    {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return -1;
    }
    SDL_UnlockAudio();
    SDL_PauseAudio(0);
    // SDL_GetAudioDeviceName(0,0)
    // do {
    //     is->audioID = SDL_OpenAudioDevice(NULL,0,&wanted_spec, &spec,0);
    //     fprintf(stderr,"SDL_OpenAudio (%d channels): %s\n",wanted_spec.channels, SDL_GetError());
    //     qDebug()<<QString("SDL_OpenAudio (%1 channels): %2").arg(wanted_spec.channels).arg(SDL_GetError());
    //     wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
    //     if (!wanted_spec.channels) {
    //         fprintf(stderr,"No more channel combinations to try, audio open failed\n");
    //         break;
    //     }
    // }while(is->audioID == 0);

    /* 检查实际使用的配置（保存在spec,由SDL_OpenAudio()填充） */
    // if (spec.format != AUDIO_S16SYS) {
    //     fprintf(stderr,"SDL advised audio format %d is not supported!\n",spec.format);
    //     return -1;
    // }

    //is->audio_hw_buf_size = spec.size;
    /* 把设置好的参数保存到大结构中 */
    // is->audio_src_fmt = is->audio_tgt_fmt = AV_SAMPLE_FMT_S16;
    // is->audio_src_freq = is->audio_tgt_freq = spec.freq;
    // is->audio_src_ch_layout = is->audio_tgt_ch_layout = codecCtx->ch_layout;
    // is->audio_src_channels = is->audio_tgt_channels = spec.channels;
    //is->FC->streams[stream_index]->discard = AVDISCARD_DEFAULT;
    // is->audio_st = ic->streams[stream_index];
    is->audio_buf_size = 0;
    is->audio_buf_index = 0;
    // memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
    // SDL_PauseAudioDevice(is->audioID, 0);

    return 0;
}
int video_thread(void *arg)
{
    MediaState *is = (MediaState *) arg;
    while(is->aCodecC == NULL || is->vCodecC == NULL)
    {
        QThread::msleep(1);
    }
    AVPacket pkt1, *packet = &pkt1;

    int ret, numBytes;
    double video_pts = 0; //当前视频的pts
    double audio_pts = 0; //音频pts
    ///解码视频相关
    AVFrame *pFrame, *pFrameRGB;
    uint8_t *out_buffer_rgb; //解码后的rgb数据
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    ///这里我们改成了 将解码后的YUV数据转换成RGB32


    numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, is->vCodecC->width, is->vCodecC->height, 1);

    out_buffer_rgb = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize,
                         out_buffer_rgb, AV_PIX_FMT_RGB32,
                         is->vCodecC->width, is->vCodecC->height, 1);
    while(1)
    {
        if (packet_queue_get(is->vqueue, packet, 1) <= 0) break;//队列里面没有数据了  读取完毕了

        ret = avcodec_send_packet(is->vCodecC, packet);
        if (ret < 0) {
            printf("decode error.\n");
            return -1;
        }

        while (avcodec_receive_frame(is->vCodecC, pFrame) >= 0) {
            sws_scale(is->swsC,
                      pFrame->data, pFrame->linesize, 0, is->vCodecC->height,
                      pFrameRGB->data, pFrameRGB->linesize);
            //把这个RGB数据 用QImage加载
            QImage image(is->vCodecC->width, is->vCodecC->height, QImage::Format_RGBA8888);
            for (int y = 0; y < is->vCodecC->height; y++) {
                memcpy(image.scanLine(y), pFrameRGB->data[0] + y * pFrameRGB->linesize[0], is->vCodecC->width * 4);
            }
            QImage bgraImage = image.convertToFormat(QImage::Format_ARGB32);
            // if(is->time_base * pFrame->pts > is->audio_pts)
            // {
            //     SDL_Delay(is->time_base * pFrame->pts - is->audio_pts);
            // }
            if (packet->dts == AV_NOPTS_VALUE && pFrame->opaque&& *(uint64_t*) pFrame->opaque != AV_NOPTS_VALUE)
            {
                video_pts = *(uint64_t *) pFrame->opaque;
            }
            else if (packet->dts != AV_NOPTS_VALUE)
            {
                video_pts = packet->dts;
            }
            else
            {
                video_pts = 0;
            }

            video_pts *= av_q2d(is->pStream->time_base);
            qDebug()<<"is->pStream->time_base =  = "<<is->pStream->time_base.den<<" "<<is->pStream->time_base.num;
            video_pts = synchronize_video(is, pFrame, video_pts);
            while(1)
            {
                audio_pts = is->audio_clock;
                //qDebug()<<"video_pts = "<<video_pts;
                //qDebug()<<"audio_pts = "<<is->audio_clock;
                if (video_pts <= audio_pts) break;

                int delayTime = (video_pts - audio_pts) * 1000;

                delayTime = delayTime > 5 ? 5:delayTime;

                SDL_Delay(delayTime);
            }
            is->Decoder->disPlayVideo(bgraImage); //调用激发信号的函数
        }
        av_packet_unref(packet);
    }
    av_free(pFrame);
    av_free(pFrameRGB);
    av_free(out_buffer_rgb);

    return 0;
}
MediaDecoder::MediaDecoder()
{
    mMediastate.video_thread = SDL_CreateThread(video_thread, "video_thread", &mMediastate);
    mMediastate.aqueue = new PacketQueue();
    mMediastate.vqueue = new PacketQueue();
    mMediastate.aCodecC = NULL;
    mMediastate.vCodecC = NULL;
    mMediastate.audio_pts = 0;
    mMediastate.aStream = NULL;
    mMediastate.pStream = NULL;
}
MediaDecoder::~MediaDecoder()
{
    avcodec_free_context(&mMediastate.aCodecC);
    avcodec_free_context(&mMediastate.vCodecC);
}
void MediaDecoder::disPlayVideo(QImage img)
{
    emit sendOneFrame(img);  //发送信号
}

void MediaDecoder::startPlay()
{
    /// 调用 QThread 的start函数 将会自动执行下面的run函数 run函数是一个新的线程
    this->start();
}

void MediaDecoder::getFileName(QString filename)
{
    mFileName = filename;
    //mediastateInit();
}

void MediaDecoder::run()//读视频文件，从视频文件解析视频音频packet并压入待解析队列
{
    char *file_path = NULL;
    QString temp = "";
    while(1)
    {
        file_path = NULL;
        if(temp != mFileName)
        {
            temp = mFileName;
            file_path = temp.toStdString().data();
        }
        else{
            QThread::msleep(1);
            continue;
        }
        if (SDL_Init(SDL_INIT_AUDIO)) {
            fprintf(stderr,"Could not initialize SDL - %s. \n", SDL_GetError());
            continue;
        }
        qDebug()<<file_path;
        MediaState *is = &mMediastate;
        const AVCodec *pCodec;

        const AVCodec *aCodec;

        //int audioStream ,videoStream, i;

        //Allocate an AVFormatContext.
        is->FC = avformat_alloc_context();
        if (avformat_open_input(&is->FC, file_path, NULL, NULL) != 0) {
            qDebug()<<"can't open the file. \n";
            continue;
        }

        if (avformat_find_stream_info(is->FC, NULL) < 0) {
            qDebug()<<"Could't find stream infomation.\n";
            continue;
        }
        mMediastate.videoStream = -1;
        mMediastate.audioStream = -1;

        ///循环查找视频中包含的流信息，
        for (unsigned int i = 0; i < is->FC->nb_streams; i++) {
            if (is->FC->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                mMediastate.videoStream = i;
            }
            if (is->FC->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
            {
                mMediastate.audioStream = i;
            }
        }
        ///如果videoStream为-1 说明没有找到视频流
        qDebug()<<"get video and audio stream";
        if (mMediastate.videoStream >= 0) {
            mMediastate.pStream = is->FC->streams[mMediastate.videoStream];
            pCodec = avcodec_find_decoder(is->pStream->codecpar->codec_id);
            if(is->vCodecC != NULL)
            {
                avcodec_free_context(&mMediastate.vCodecC);
            }
            is->vCodecC = avcodec_alloc_context3(pCodec);
            avcodec_parameters_to_context(is->vCodecC, is->FC->streams[mMediastate.videoStream]->codecpar);
            //qDebug()<<"open video decoder";
            if (pCodec == NULL) {
                printf("PCodec not found.\n");
                continue;
            }
            is->swsC = sws_getContext(is->vCodecC->width, is->vCodecC->height,
                                      is->vCodecC->pix_fmt, is->vCodecC->width, is->vCodecC->height,
                                      AV_PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);
            ///打开视频解码器
            if (avcodec_open2(is->vCodecC, pCodec, NULL) < 0) {
                printf("Could not open video codec.\n");
                continue;
            }
            is->time_base = (double)is->FC->streams[mMediastate.videoStream]->time_base.num / (double)is->FC->streams[mMediastate.videoStream]->time_base.den * 1000.0;
            //is->video_st = pFormatCtx->streams[mMediastate.videoStream];
            //qDebug()<<"open video decoder";
            packet_queue_init(is->vqueue);
            ///创建一个线程专门用来解码视频
        }
        if (mMediastate.audioStream >= 0) {
            mMediastate.aStream = is->FC->streams[mMediastate.audioStream];
            mMediastate.audio_time_base = (double)is->aStream->time_base.num / (double)is->FC->streams[mMediastate.audioStream]->time_base.den * 1000.0;
            is->audioExist = true;
            aCodec = avcodec_find_decoder(is->FC->streams[mMediastate.audioStream]->codecpar->codec_id);
            if(is->aCodecC != NULL)
            {
                avcodec_free_context(&mMediastate.aCodecC);
            }
            is->aCodecC = avcodec_alloc_context3(aCodec);
            avcodec_parameters_to_context(is->aCodecC, is->FC->streams[mMediastate.audioStream]->codecpar);
            if (aCodec == NULL) {
                printf("aCodec not found.\n");
                continue;
            }

            ///打开音频解码器
            qDebug()<<"open audio decoder";
            if (avcodec_open2(is->aCodecC, aCodec, NULL) < 0) {
                printf("Could not open audio codec.\n");
                continue;;
            }
            qDebug()<<"open audio decoder success";
            packet_queue_init(is->aqueue);
            audio_stream_component_open(&mMediastate);
            //is->audio_st = pFormatCtx->streams[mMediastate.audioStream];
        }
        qDebug()<<"open video and audio decoder";
        is->Decoder = this;
        AVPacket *packet = av_packet_alloc(); //分配一个packet 用来存放读取的视频
        av_dump_format(is->FC, 0, file_path, 0); //输出视频信息
        qDebug()<<"ready get into while";
        is->audio_pts = av_gettime();
        while (1)
        {
            //这里做了个限制  当队列里面的数据超过某个大小的时候 就暂停读取  防止一下子就把视频读完了，导致的空间分配不足
            /* 这里audioq.size是指队列中的所有数据包带的音频数据的总量或者视频数据总量，并不是包的数量 */
            //这个值可以稍微写大一些
            if (is->aqueue->size > MAX_AUDIO_SIZE || is->vqueue->size > MAX_VIDEO_SIZE) {
                SDL_Delay(10);
                continue;
            }
            if (av_read_frame(is->FC, packet) < 0)
            {
                break; //这里认为视频读取完了
            }
            if (packet->stream_index == mMediastate.videoStream)
            {
                packet_queue_put(is->vqueue, packet);
                //这里我们将数据存入队列 因此不调用 av_free_packet 释放
            }
            else if( packet->stream_index == mMediastate.audioStream )
            {
                packet_queue_put(is->aqueue, packet);
                // qDebug()<<"aqueue put next";
                //这里我们将数据存入队列 因此不调用 av_free_packet 释放
            }
            else
            {
                // Free the packet that was allocated by av_read_frame
                av_packet_unref(packet);
            }
        }
        while(1)
        {
            SDL_Delay(10);
        }
        avformat_close_input(&is->FC);
        av_packet_free(&packet);

    }
}
