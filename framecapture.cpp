#include "framecapture.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
}

#include <QImage>
#include <QDebug>

FrameCapture::FrameCapture(QObject *parent) : QObject(parent) {}

static AVHWDeviceType pickPreviewHwDevice()
{
    const AVHWDeviceType priority[] = {
        AV_HWDEVICE_TYPE_CUDA,
        AV_HWDEVICE_TYPE_D3D12VA,
        AV_HWDEVICE_TYPE_DXVA2,
        AV_HWDEVICE_TYPE_VAAPI,
        AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
        AV_HWDEVICE_TYPE_NONE
    };

    for (auto type : priority) {
        if (type == AV_HWDEVICE_TYPE_NONE)
            break;
        AVBufferRef *ctx = nullptr;
        if (av_hwdevice_ctx_create(&ctx, type, nullptr, nullptr, 0) >= 0) {
            av_buffer_unref(&ctx);
            return type;
        }
    }
    return AV_HWDEVICE_TYPE_NONE;
}

bool FrameCapture::captureFrame(const QString &videoPath,
                                double positionSec,
                                const QString &outImagePath,
                                QString &errorOut,
                                bool preferHwDecoder)
{
    AVFormatContext *fmt = nullptr;

    if (avformat_open_input(&fmt, videoPath.toUtf8().constData(), nullptr, nullptr) < 0) {
        errorOut = QStringLiteral("Cannot open file: ") + videoPath;
        return false;
    }

    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        errorOut = QStringLiteral("Cannot find stream info");
        return false;
    }

    int videoStreamIdx = -1;
    const AVCodec *codec = nullptr;
    for (unsigned i = 0; i < fmt->nb_streams; i++) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec = avcodec_find_decoder(fmt->streams[i]->codecpar->codec_id);
            if (codec) {
                videoStreamIdx = static_cast<int>(i);
                break;
            }
        }
    }

    if (videoStreamIdx < 0) {
        avformat_close_input(&fmt);
        errorOut = QStringLiteral("No video stream");
        return false;
    }

    AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, fmt->streams[videoStreamIdx]->codecpar);

    AVBufferRef *hwDev = nullptr;
    if (preferHwDecoder) {
        const AVHWDeviceType hwType = pickPreviewHwDevice();
        if (hwType != AV_HWDEVICE_TYPE_NONE) {
            if (av_hwdevice_ctx_create(&hwDev, hwType, nullptr, nullptr, 0) >= 0)
                codecCtx->hw_device_ctx = av_buffer_ref(hwDev);
            else
                hwDev = nullptr;
        }
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        if (codecCtx->hw_device_ctx)
            av_buffer_unref(&codecCtx->hw_device_ctx);
        if (hwDev)
            av_buffer_unref(&hwDev);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmt);
        errorOut = QStringLiteral("Cannot open codec");
        return false;
    }

    int64_t seekTs = static_cast<int64_t>(positionSec * AV_TIME_BASE);
    av_seek_frame(fmt, -1, seekTs, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(codecCtx);

    AVPacket *pkt = av_packet_alloc();
    AVFrame *frame = av_frame_alloc();
    AVFrame *swFrame = av_frame_alloc();
    AVFrame *rgbFrame = av_frame_alloc();

    const int w = codecCtx->width;
    const int h = codecCtx->height;

    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    uint8_t *buf = static_cast<uint8_t *>(av_malloc(bufSize));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize,
                         buf, AV_PIX_FMT_RGB24, w, h, 1);

    SwsContext *swsCtx = nullptr;
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
            AVFrame *srcFrame = frame;
            if (frame->format == AV_PIX_FMT_CUDA || frame->format == AV_PIX_FMT_D3D12
                || frame->format == AV_PIX_FMT_DXVA2_VLD || frame->format == AV_PIX_FMT_VAAPI) {
                if (av_hwframe_transfer_data(swFrame, frame, 0) < 0) {
                    qWarning() << "[FrameCapture] HW frame transfer failed";
                    continue;
                }
                swFrame->pts = frame->pts;
                srcFrame = swFrame;
            }

            int64_t pts = (frame->pts != AV_NOPTS_VALUE) ? frame->pts : frame->best_effort_timestamp;
            double frameSec = pts * av_q2d(timeBase);

            double frameDuration = (codecCtx->framerate.num > 0)
                                       ? 1.0 / av_q2d(codecCtx->framerate)
                                       : 0.04;

            if (frameSec < positionSec - frameDuration)
                continue;

            const AVPixelFormat srcFmt = static_cast<AVPixelFormat>(srcFrame->format);
            if (!swsCtx) {
                swsCtx = sws_getContext(w, h, srcFmt, w, h, AV_PIX_FMT_RGB24,
                                        SWS_BILINEAR, nullptr, nullptr, nullptr);
            }

            sws_scale(swsCtx,
                      srcFrame->data, srcFrame->linesize, 0, h,
                      rgbFrame->data, rgbFrame->linesize);

            QImage img(rgbFrame->data[0], w, h, rgbFrame->linesize[0], QImage::Format_RGB888);
            saved = img.copy().save(outImagePath, "PNG");
            if (!saved)
                errorOut = QStringLiteral("Cannot save image to: ") + outImagePath;
        }

        av_packet_unref(pkt);
    }

    if (swsCtx)
        sws_freeContext(swsCtx);
    av_free(buf);
    av_frame_free(&rgbFrame);
    av_frame_free(&swFrame);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&codecCtx);
    if (hwDev)
        av_buffer_unref(&hwDev);
    avformat_close_input(&fmt);

    if (!saved && errorOut.isEmpty())
        errorOut = QStringLiteral("No frame decoded at position ")
                   + QString::number(positionSec, 'f', 3) + QStringLiteral("s");

    return saved;
}
