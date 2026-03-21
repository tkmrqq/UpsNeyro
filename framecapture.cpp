#include "framecapture.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <QImage>
#include <QDebug>

FrameCapture::FrameCapture(QObject *parent) : QObject(parent) {}

bool FrameCapture::captureFrame(const QString &videoPath,
                                double positionSec,
                                const QString &outImagePath,
                                QString &errorOut)
{
    AVFormatContext *fmt = nullptr;

    if (avformat_open_input(&fmt, videoPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        errorOut = "Cannot open file: " + videoPath;
        return false;
    }

    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        errorOut = "Cannot find stream info";
        return false;
    }

    int videoStreamIdx = -1;
    const AVCodec *codec = nullptr;
    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec = avcodec_find_decoder(fmt->streams[i]->codecpar->codec_id);
            if (codec) {
                videoStreamIdx = i;
                break;
            }
        }
    }

    if (videoStreamIdx < 0) {
        avformat_close_input(&fmt);
        errorOut = "No video stream found";
        return false;
    }

    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, fmt->streams[videoStreamIdx]->codecpar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmt);
        errorOut = "Cannot open codec";
        return false;
    }

    // Seek к ближайшему keyframe до нужной позиции
    int64_t seekTs = (int64_t)(positionSec * AV_TIME_BASE);
    av_seek_frame(fmt, -1, seekTs, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);

    AVPacket *pkt      = av_packet_alloc();
    AVFrame  *frame    = av_frame_alloc();
    AVFrame  *rgbFrame = av_frame_alloc();

    int w = codecCtx->width;
    int h = codecCtx->height;

    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    uint8_t *buf = (uint8_t*)av_malloc(bufSize);
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize,
                         buf, AV_PIX_FMT_RGB24, w, h, 1);

    SwsContext *swsCtx = sws_getContext(
        w, h, codecCtx->pix_fmt,
        w, h, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
        );

    // time_base потока — нужен для перевода pts → секунды
    AVRational timeBase = fmt->streams[videoStreamIdx]->time_base;
    bool saved = false;

    while (av_read_frame(fmt, pkt) >= 0 && !saved) {
        if (pkt->stream_index != videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(codecCtx, pkt) < 0) {
            av_packet_unref(pkt);
            continue;
        }

        while (avcodec_receive_frame(codecCtx, frame) == 0 && !saved) {

            // Считаем реальное время этого кадра в секундах
            int64_t pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts
                                                         : frame->best_effort_timestamp;
            double frameSec = pts * av_q2d(timeBase);

            // Пропускаем кадры которые идут РАНЬШЕ нужной позиции
            // Допуск 1 кадр назад (~одна длительность кадра)
            double frameDuration = (codecCtx->framerate.num > 0)
                                       ? 1.0 / av_q2d(codecCtx->framerate)
                                       : 0.04; // fallback: ~25fps

            if (frameSec < positionSec - frameDuration) {
                qDebug() << "[FrameCapture] skip frame pts=" << frameSec
                         << "target=" << positionSec;
                continue;
            }

            qDebug() << "[FrameCapture] captured frame pts=" << frameSec
                     << "target=" << positionSec;

            // Конвертируем в RGB и сохраняем
            sws_scale(swsCtx,
                      frame->data, frame->linesize, 0, h,
                      rgbFrame->data, rgbFrame->linesize);

            QImage img(rgbFrame->data[0], w, h,
                       rgbFrame->linesize[0],
                       QImage::Format_RGB888);

            saved = img.save(outImagePath, "PNG");
            if (!saved)
                errorOut = "Cannot save image to: " + outImagePath;
        }

        av_packet_unref(pkt);
    }

    sws_freeContext(swsCtx);
    av_free(buf);
    av_frame_free(&rgbFrame);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmt);

    if (!saved && errorOut.isEmpty())
        errorOut = "No frame decoded at position " + QString::number(positionSec, 'f', 3) + "s";

    return saved;
}
