#include "video_decode.h"
#include <iostream>
#include <QDebug>

Video_decode::Video_decode()
{
    FileName.clear();
}

Video_decode::~Video_decode()
{

}

void Video_decode::saveFrame(AVFrame *pFrame, int width, int height,int index)
{
    QImage image(width, height, QImage::Format_BGR888);
    for (int y = 0; y < height; y++) {
        memcpy(image.scanLine(y), pFrame->data[0] + y * pFrame->linesize[0], width * 3);
    }
    image_queue.push_back(image);
}

void Video_decode::getFileName(QString fileName)
{
    FileName = fileName;
    std::cout<<fileName.toStdString().c_str()<<std::endl;
}

void Video_decode::run()
{
    while(1)
    {
        if(!FileName.isEmpty())
        {
            std::cout<<FileName.toStdString().c_str()<<std::endl;
            decode(FileName);
        }
    }
}

void Video_decode::decode(QString& fileName)
{
    AVFormatContext *format_context = NULL;
    AVCodecContext *codec_context = NULL;
    const AVCodec *codec = NULL;
    AVFrame *frame = NULL;
    AVFrame *pFrameRGB = NULL;
    AVPacket packet;
    struct SwsContext *sws_context = NULL;

    // 打开输入文件
    if (avformat_open_input(&format_context, fileName.toStdString().c_str(), NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", fileName.toStdString().c_str());
        return;
    }

    // 获取流信息
    if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        avformat_close_input(&format_context);
        return;
    }

    // 找到视频流
    int video_stream_index = -1;
    for (int i = 0; i < format_context->nb_streams; i++) {
        if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    qDebug()<<"time_base.den = "<<format_context->streams[video_stream_index]->time_base.den;
    qDebug()<<"time_base.num = "<<format_context->streams[video_stream_index]->time_base.num;
    qDebug()<<format_context->streams[video_stream_index]->pts_wrap_bits;
    Time_Base = (double)format_context->streams[video_stream_index]->time_base.num / (double)format_context->streams[video_stream_index]->time_base.den * 1000;
    if (video_stream_index == -1) {
        fprintf(stderr, "Could not find a video stream\n");
        avformat_close_input(&format_context);
        return;
    }

    // 获取解码器
    AVCodecParameters* codec_parameters = format_context->streams[video_stream_index]->codecpar;
    codec = avcodec_find_decoder(codec_parameters->codec_id);
    if (codec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        avformat_close_input(&format_context);
        return;
    }

    // 创建解码器上下文
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        fprintf(stderr, "Could not allocate codec context\n");
        avformat_close_input(&format_context);
        return;
    }

    // 复制参数到解码器上下文
    avcodec_parameters_to_context(codec_context, codec_parameters);

    // 打开解码器
    if (avcodec_open2(codec_context, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return;
    }
    qDebug()<<codec_context->framerate.den<<" "<<codec_context->framerate.num;
    qDebug()<<codec_context->pkt_timebase.den<<" "<<codec_context->pkt_timebase.num;

    // 分配帧
    frame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();
    if (!frame || !pFrameRGB) {
        fprintf(stderr, "Could not allocate frame\n");
        avcodec_free_context(&codec_context);
        avformat_close_input(&format_context);
        return;
    }
    int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB32, codec_context->width, codec_context->height, 1);
    uint8_t *buffer = (uint8_t *)av_malloc(num_bytes * sizeof(uint8_t));
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB32, codec_context->width, codec_context->height, 1);

    sws_context = sws_getContext(codec_context->width, codec_context->height, codec_context->pix_fmt,
                                 codec_context->width, codec_context->height, AV_PIX_FMT_RGB32,
                                 SWS_BILINEAR, NULL, NULL, NULL);

    int frame_count = 0;
    static struct SwsContext *img_convert_ctx;
    // 读取数据包
    currentTime = av_gettime();
    qDebug()<<"currentTime = "<<currentTime;
    while (av_read_frame(format_context, &packet) >= 0) {
        // 只处理视频流
        qDebug()<<packet.time_base.den<<" "<<packet.time_base.num;
        if (packet.stream_index == video_stream_index) {
            // 解码视频帧
            int response = avcodec_send_packet(codec_context, &packet);
            if (response < 0) {
                fprintf(stderr, "Error while sending a packet to the decoder\n");
                break;
            }

            while (response >= 0) {
                response = avcodec_receive_frame(codec_context, frame);
                if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
                    break;
                } else if (response < 0) {
                    fprintf(stderr, "Error while receiving a frame from the decoder\n");
                    break;
                }

                // 处理帧（保存为图像文件）
                sws_scale(sws_context, frame->data, frame->linesize, 0, codec_context->height,
                          pFrameRGB->data, pFrameRGB->linesize);
                QImage image(codec_context->width, codec_context->height, QImage::Format_RGBA8888);
                for (int y = 0; y < codec_context->height; y++) {
                    memcpy(image.scanLine(y), pFrameRGB->data[0] + y * pFrameRGB->linesize[0], codec_context->width * 4);
                }
                QImage bgraImage = image.convertToFormat(QImage::Format_ARGB32);
                static int k = 0;
                double delayTime = (double)(frame->pts - k) * Time_Base;
                qDebug()<<"delayTime "<<frame->pts;
                //qDebug()<<"timebase.num = "<<frame->time_base.num << " timebase.den = "<<frame->time_base.den;

                QThread::msleep(delayTime);
                emit sendOneFrame(bgraImage);

                k = frame->pts;

                //saveFrame(pFrameRGB, codec_context->width, codec_context->height, frame_count);
                //frame_count++;
            }
        }
        av_packet_unref(&packet);
    }

    // 清理
    av_frame_free(&frame);
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);

    printf("Processed %d frames.\n", frame_count);
    return;
}
