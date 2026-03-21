#include "videodecoder.h"
#include <QDebug>

extern "C" {
#include <libavutil/pixdesc.h>
}

VideoDecoder::VideoDecoder(QObject *parent) : QObject(parent) {}

VideoDecoder::~VideoDecoder()
{
    close();
}

void VideoDecoder::close()
{
    if (m_codecCtx)  { avcodec_free_context(&m_codecCtx); }
    if (m_hwDevCtx)  { av_buffer_unref(&m_hwDevCtx); }
    if (m_fmt)       { avformat_close_input(&m_fmt); }
    m_videoStreamIdx = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Автодетект лучшего HW бэкенда
// ─────────────────────────────────────────────────────────────────────────────

AVHWDeviceType VideoDecoder::detectBestHWDevice()
{
    // Приоритет: CUDA (NVIDIA) → D3D12VA (Win, любой GPU) →
    //            VAAPI (Linux AMD/Intel) → VideoToolbox (macOS) → CPU
    const AVHWDeviceType priority[] = {
        AV_HWDEVICE_TYPE_CUDA,
        AV_HWDEVICE_TYPE_D3D12VA,
        AV_HWDEVICE_TYPE_DXVA2,
        AV_HWDEVICE_TYPE_VAAPI,
        AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
        AV_HWDEVICE_TYPE_NONE
    };

    for (auto type : priority) {
        if (type == AV_HWDEVICE_TYPE_NONE) break;

        AVBufferRef *ctx = nullptr;
        int ret = av_hwdevice_ctx_create(&ctx, type, nullptr, nullptr, 0);
        if (ret >= 0) {
            av_buffer_unref(&ctx);
            qDebug() << "[VideoDecoder] HW device available:"
                     << av_hwdevice_get_type_name(type);
            return type;
        }
    }

    qDebug() << "[VideoDecoder] No HW device found, using CPU";
    return AV_HWDEVICE_TYPE_NONE;
}

bool VideoDecoder::initHWDecoder(AVHWDeviceType type)
{
    int ret = av_hwdevice_ctx_create(&m_hwDevCtx, type, nullptr, nullptr, 0);
    if (ret < 0) {
        qWarning() << "[VideoDecoder] Failed to create HW device context";
        return false;
    }
    m_codecCtx->hw_device_ctx = av_buffer_ref(m_hwDevCtx);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// open()
// ─────────────────────────────────────────────────────────────────────────────

bool VideoDecoder::open(const QString &path, QString &errorOut)
{
    close();
    m_stopRequested = false;

    // Открываем файл
    int ret = avformat_open_input(&m_fmt,
                                  path.toUtf8().constData(),
                                  nullptr, nullptr);
    if (ret < 0) {
        errorOut = "Cannot open file: " + path;
        return false;
    }

    if (avformat_find_stream_info(m_fmt, nullptr) < 0) {
        errorOut = "Cannot find stream info";
        avformat_close_input(&m_fmt);
        return false;
    }

    // Ищем видеопоток
    const AVCodec *codec = nullptr;
    m_videoStreamIdx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_VIDEO,
                                           -1, -1, &codec, 0);
    if (m_videoStreamIdx < 0 || !codec) {
        errorOut = "No video stream found";
        avformat_close_input(&m_fmt);
        return false;
    }

    AVStream *stream = m_fmt->streams[m_videoStreamIdx];

    // Заполняем VideoInfo
    m_info.width  = stream->codecpar->width;
    m_info.height = stream->codecpar->height;
    m_info.codec  = QString::fromUtf8(codec->name).toUpper();
    m_info.fps    = av_q2d(stream->avg_frame_rate);
    m_info.duration = m_fmt->duration / (double)AV_TIME_BASE;

    // Подсчёт кадров
    if (stream->nb_frames > 0) {
        m_info.totalFrames = static_cast<int>(stream->nb_frames);
    } else {
        // Оцениваем по длительности и fps
        m_info.totalFrames = static_cast<int>(m_info.duration * m_info.fps);
    }

    // Проверяем наличие аудио
    m_info.hasAudio = av_find_best_stream(m_fmt, AVMEDIA_TYPE_AUDIO,
                                          -1, -1, nullptr, 0) >= 0;

    // Создаём контекст декодера
    m_codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codecCtx, stream->codecpar);

    // Пробуем HW декодирование
    AVHWDeviceType hwType = detectBestHWDevice();
    if (hwType != AV_HWDEVICE_TYPE_NONE) {
        if (initHWDecoder(hwType)) {
            m_info.hwDevice = QString::fromUtf8(
                                  av_hwdevice_get_type_name(hwType)).toUpper();
            qDebug() << "[VideoDecoder] Using HW decode:" << m_info.hwDevice;
        }
    } else {
        m_info.hwDevice = "CPU";
    }

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        errorOut = "Cannot open codec";
        close();
        return false;
    }

    qDebug() << "[VideoDecoder] Opened:" << path;
    qDebug() << "  Size:   " << m_info.width << "x" << m_info.height;
    qDebug() << "  FPS:    " << m_info.fps;
    qDebug() << "  Frames: " << m_info.totalFrames;
    qDebug() << "  HW:     " << m_info.hwDevice;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// decode() — основной цикл
// ─────────────────────────────────────────────────────────────────────────────

bool VideoDecoder::decode(FrameCallback onFrame, QString &errorOut)
{
    if (!m_fmt || !m_codecCtx || m_videoStreamIdx < 0) {
        errorOut = "Decoder not opened";
        return false;
    }

    AVPacket  *pkt      = av_packet_alloc();
    AVFrame   *frame    = av_frame_alloc();  // декодированный (может быть HW)
    AVFrame   *swFrame  = av_frame_alloc();  // SW копия если HW
    AVFrame   *rgbFrame = av_frame_alloc();  // RGB для апскейлера

    int w = m_info.width;
    int h = m_info.height;

    // Буфер для RGB кадра
    int bufSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1);
    uint8_t *rgbBuf = static_cast<uint8_t*>(av_malloc(bufSize));
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize,
                         rgbBuf, AV_PIX_FMT_RGB24, w, h, 1);

    SwsContext *swsCtx = nullptr;
    int frameIdx = 0;
    bool success = true;

    while (!m_stopRequested && av_read_frame(m_fmt, pkt) >= 0) {
        if (pkt->stream_index != m_videoStreamIdx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(m_codecCtx, pkt) < 0) {
            av_packet_unref(pkt);
            continue;
        }

        while (!m_stopRequested) {
            int ret = avcodec_receive_frame(m_codecCtx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
            if (ret < 0) { success = false; break; }

            AVFrame *srcFrame = frame;

            // Если кадр в HW памяти — копируем в RAM
            if (frame->format == AV_PIX_FMT_CUDA    ||
                frame->format == AV_PIX_FMT_D3D12   ||
                frame->format == AV_PIX_FMT_DXVA2_VLD ||
                frame->format == AV_PIX_FMT_VAAPI)
            {
                if (av_hwframe_transfer_data(swFrame, frame, 0) < 0) {
                    qWarning() << "[VideoDecoder] HW frame transfer failed";
                    continue;
                }
                swFrame->pts = frame->pts;
                srcFrame = swFrame;
            }

            // Конвертируем в RGB24
            AVPixelFormat srcFmt = static_cast<AVPixelFormat>(srcFrame->format);
            if (!swsCtx) {
                swsCtx = sws_getContext(
                    w, h, srcFmt,
                    w, h, AV_PIX_FMT_RGB24,
                    SWS_BILINEAR, nullptr, nullptr, nullptr
                    );
            }

            sws_scale(swsCtx,
                      srcFrame->data, srcFrame->linesize, 0, h,
                      rgbFrame->data, rgbFrame->linesize);

            // Передаём кадр в колбэк
            onFrame(rgbBuf, bufSize, frameIdx++);
        }

        av_packet_unref(pkt);
    }

    // Flush декодера
    avcodec_send_packet(m_codecCtx, nullptr);
    while (!m_stopRequested) {
        int ret = avcodec_receive_frame(m_codecCtx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) break;
        if (ret < 0) break;

        AVFrame *srcFrame = frame;
        if (frame->format == AV_PIX_FMT_CUDA ||
            frame->format == AV_PIX_FMT_D3D12 ||
            frame->format == AV_PIX_FMT_DXVA2_VLD ||
            frame->format == AV_PIX_FMT_VAAPI)
        {
            if (av_hwframe_transfer_data(swFrame, frame, 0) >= 0) {
                swFrame->pts = frame->pts;
                srcFrame = swFrame;
            }
        }

        AVPixelFormat srcFmt = static_cast<AVPixelFormat>(srcFrame->format);
        if (!swsCtx) {
            swsCtx = sws_getContext(w, h, srcFmt, w, h, AV_PIX_FMT_RGB24,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
        }
        sws_scale(swsCtx,
                  srcFrame->data, srcFrame->linesize, 0, h,
                  rgbFrame->data, rgbFrame->linesize);
        onFrame(rgbBuf, bufSize, frameIdx++);
    }

    // Освобождаем ресурсы
    if (swsCtx) sws_freeContext(swsCtx);
    av_free(rgbBuf);
    av_frame_free(&rgbFrame);
    av_frame_free(&swFrame);
    av_frame_free(&frame);
    av_packet_free(&pkt);

    qDebug() << "[VideoDecoder] Decoded" << frameIdx << "frames";
    return success && !m_stopRequested;
}

void VideoDecoder::stop()
{
    m_stopRequested = true;
}
