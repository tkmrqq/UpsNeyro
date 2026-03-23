#include "videoencoder.h"
#include <QDebug>

extern "C" {
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
}

VideoEncoder::VideoEncoder(QObject *parent) : QObject(parent) {}

VideoEncoder::~VideoEncoder()
{
    close();
}

void VideoEncoder::close()
{
    if (m_swsCtx)      { sws_freeContext(m_swsCtx);      m_swsCtx    = nullptr; }
    if (m_codecCtx)    { avcodec_free_context(&m_codecCtx); }
    if (m_hwDevCtx)    { av_buffer_unref(&m_hwDevCtx);   }
    if (m_srcAudioCtx) { avcodec_free_context(&m_srcAudioCtx); }
    if (m_srcAudioFmt) { avformat_close_input(&m_srcAudioFmt); }
    if (m_fmtCtx) {
        if (m_fmtCtx->pb) avio_closep(&m_fmtCtx->pb);
        avformat_free_context(m_fmtCtx);
        m_fmtCtx = nullptr;
    }
    m_frameIdx = 0;
    m_isHW     = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// findBestEncoder — NVENC → AMF → QSV → libx264
// ─────────────────────────────────────────────────────────────────────────────

// В findBestEncoder() — добавь проверку через реальное открытие кодека:
const AVCodec* VideoEncoder::findBestEncoder()
{
    const char* encoders[] = {
        // "h264_nvenc",
        // "h264_amf",
        // "h264_qsv",
        "libx264",
        nullptr
    };

    for (int i = 0; encoders[i]; ++i) {
        const AVCodec *codec = avcodec_find_encoder_by_name(encoders[i]);
        if (!codec) continue;

        // Для CPU кодека — сразу берём
        if (strcmp(encoders[i], "libx264") == 0) {
            m_isHW = false;
            m_codecName = QString::fromUtf8(encoders[i]);
            qDebug() << "[VideoEncoder] Using encoder:" << encoders[i];
            return codec;
        }

        // Для HW — проверяем через попытку открыть контекст
        AVCodecContext *testCtx = avcodec_alloc_context3(codec);
        testCtx->width    = 640;
        testCtx->height   = 480;
        testCtx->pix_fmt  = AV_PIX_FMT_NV12;
        testCtx->time_base = av_make_q(1, 25);

        if (avcodec_open2(testCtx, codec, nullptr) == 0) {
            avcodec_free_context(&testCtx);
            m_isHW = true;
            m_codecName = QString::fromUtf8(encoders[i]);
            qDebug() << "[VideoEncoder] Using encoder:" << encoders[i];
            return codec;
        }
        avcodec_free_context(&testCtx);
        qDebug() << "[VideoEncoder] Encoder not working:" << encoders[i];
    }
    return nullptr;
}


bool VideoEncoder::initHWEncoder(const AVCodec *codec)
{
    // Определяем тип HW устройства по кодеку
    AVHWDeviceType hwType = AV_HWDEVICE_TYPE_NONE;

    if (m_codecName == "h264_nvenc") hwType = AV_HWDEVICE_TYPE_CUDA;
    if (m_codecName == "h264_amf")   hwType = AV_HWDEVICE_TYPE_D3D11VA;
    if (m_codecName == "h264_qsv")   hwType = AV_HWDEVICE_TYPE_QSV;

    if (hwType == AV_HWDEVICE_TYPE_NONE) return false;

    if (av_hwdevice_ctx_create(&m_hwDevCtx, hwType,
                               nullptr, nullptr, 0) < 0) {
        qWarning() << "[VideoEncoder] Cannot create HW device for encoder";
        return false;
    }

    // Создаём HW frames context
    AVBufferRef *hwFramesRef = av_hwframe_ctx_alloc(m_hwDevCtx);
    if (!hwFramesRef) return false;

    AVHWFramesContext *hwFramesCtx =
        reinterpret_cast<AVHWFramesContext*>(hwFramesRef->data);

    hwFramesCtx->format     = (m_codecName == "h264_nvenc")
                              ? AV_PIX_FMT_CUDA : AV_PIX_FMT_D3D11;
    hwFramesCtx->sw_format  = AV_PIX_FMT_NV12;
    hwFramesCtx->width      = m_settings.width;
    hwFramesCtx->height     = m_settings.height;
    hwFramesCtx->initial_pool_size = 4;

    if (av_hwframe_ctx_init(hwFramesRef) < 0) {
        av_buffer_unref(&hwFramesRef);
        return false;
    }

    m_codecCtx->hw_frames_ctx = av_buffer_ref(hwFramesRef);
    av_buffer_unref(&hwFramesRef);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// open()
// ─────────────────────────────────────────────────────────────────────────────

bool VideoEncoder::open(const QString &outputPath,
                        const EncodeSettings &settings,
                        QString &errorOut)
{
    close();
    m_settings = settings;

    // Создаём output format context
    int ret = avformat_alloc_output_context2(
        &m_fmtCtx, nullptr, nullptr,
        outputPath.toUtf8().constData());

    if (ret < 0 || !m_fmtCtx) {
        errorOut = "Cannot allocate output context";
        return false;
    }

    // ── Видео поток ───────────────────────────────────────────────────────────
    const AVCodec *codec = findBestEncoder();
    if (!codec) {
        errorOut = "No suitable video encoder found";
        close();
        return false;
    }

    m_videoStream = avformat_new_stream(m_fmtCtx, nullptr);
    if (!m_videoStream) {
        errorOut = "Cannot create video stream";
        close();
        return false;
    }

    m_codecCtx = avcodec_alloc_context3(codec);

    m_codecCtx->width     = settings.width;
    m_codecCtx->height    = settings.height;
    AVRational fpsRat = av_d2q(settings.fps, 100000);
    m_codecCtx->framerate = fpsRat;
    m_codecCtx->time_base = av_inv_q(fpsRat);
    m_codecCtx->pix_fmt   = m_isHW ? AV_PIX_FMT_NV12 : AV_PIX_FMT_YUV420P;

    // CRF из quality (100 → CRF 0, 0 → CRF 51)
    int crf = static_cast<int>(51.0 * (1.0 - settings.quality / 100.0));

    if (m_isHW) {
        // HW энкодеры используют -qp вместо -crf
        av_opt_set_int(m_codecCtx->priv_data, "qp", crf, 0);
        av_opt_set(m_codecCtx->priv_data, "preset", "p4", 0); // NVENC balanced
    } else {
        av_opt_set_int(m_codecCtx->priv_data, "crf", crf, 0);
        av_opt_set(m_codecCtx->priv_data, "preset", "medium", 0);
    }

    // Для mp4 нужен global_header
    if (m_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        m_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Инициализируем HW если нужно
    if (m_isHW) {
        if (!initHWEncoder(codec)) {
            qWarning() << "[VideoEncoder] HW init failed, falling back to CPU";
            m_isHW = false;
            m_codecName = "libx264";
            codec = avcodec_find_encoder_by_name("libx264");
            if (!codec) {
                errorOut = "libx264 not found";
                close();
                return false;
            }
            m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
            av_opt_set_int(m_codecCtx->priv_data, "crf", crf, 0);
            av_opt_set(m_codecCtx->priv_data, "preset", "medium", 0);
        }
    }

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0) {
        errorOut = "Cannot open video encoder";
        close();
        return false;
    }

    avcodec_parameters_from_context(m_videoStream->codecpar, m_codecCtx);
    m_videoStream->time_base = m_codecCtx->time_base;

    // ── Аудио поток (копируем из оригинала) ──────────────────────────────────
    if (settings.copyAudio && !settings.audioSourcePath.isEmpty()) {
        if (avformat_open_input(&m_srcAudioFmt,
                                settings.audioSourcePath.toUtf8().constData(),
                                nullptr, nullptr) >= 0)
        {
            avformat_find_stream_info(m_srcAudioFmt, nullptr);

            m_srcAudioStream = av_find_best_stream(
                m_srcAudioFmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);

            if (m_srcAudioStream >= 0) {
                m_audioStream = avformat_new_stream(m_fmtCtx, nullptr);
                avcodec_parameters_copy(
                    m_audioStream->codecpar,
                    m_srcAudioFmt->streams[m_srcAudioStream]->codecpar);
                m_audioStream->time_base =
                    m_srcAudioFmt->streams[m_srcAudioStream]->time_base;
                qDebug() << "[VideoEncoder] Audio stream added";
            }
        }
    }

    // ── Открываем файл и пишем заголовок ─────────────────────────────────────
    if (!(m_fmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&m_fmtCtx->pb,
                      outputPath.toUtf8().constData(),
                      AVIO_FLAG_WRITE) < 0)
        {
            errorOut = "Cannot open output file: " + outputPath;
            close();
            return false;
        }
    }

    if (avformat_write_header(m_fmtCtx, nullptr) < 0) {
        errorOut = "Cannot write file header";
        close();
        return false;
    }

    // Инициализируем SwsContext RGB24 → YUV420P/NV12
    AVPixelFormat dstFmt = m_isHW ? AV_PIX_FMT_NV12 : AV_PIX_FMT_YUV420P;
    m_swsCtx = sws_getContext(
        settings.width, settings.height, AV_PIX_FMT_RGB24,
        settings.width, settings.height, dstFmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr
        );

    qDebug() << "[VideoEncoder] Opened:" << outputPath;
    qDebug() << "  Codec:  " << m_codecName;
    qDebug() << "  Size:   " << settings.width << "x" << settings.height;
    qDebug() << "  FPS:    " << settings.fps;
    qDebug() << "  CRF:    " << crf;
    qDebug() << "  HW:     " << m_isHW;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// encodeFrame()
// ─────────────────────────────────────────────────────────────────────────────

bool VideoEncoder::encodeFrame(const uint8_t *rgbData, int width, int height,
                               QString &errorOut)
{
    if (!m_fmtCtx || !m_codecCtx) {
        errorOut = "Encoder not opened";
        return false;
    }

    // Создаём SW кадр из RGB данных
    AVFrame *swFrame = av_frame_alloc();
    swFrame->format = m_isHW ? AV_PIX_FMT_NV12 : AV_PIX_FMT_YUV420P;
    swFrame->width  = width;
    swFrame->height = height;
    av_frame_get_buffer(swFrame, 0);

    // Конвертируем RGB → YUV
    const uint8_t *srcSlice[1] = { rgbData };
    int srcStride[1] = { width * 3 };

    sws_scale(m_swsCtx,
              srcSlice, srcStride, 0, height,
              swFrame->data, swFrame->linesize);

    swFrame->pts = av_rescale_q(
        m_frameIdx++,
        av_make_q(1, static_cast<int>(m_settings.fps + 0.5)),  // кадр за кадром
        m_codecCtx->time_base
    );

    AVFrame *encFrame = swFrame;
    AVFrame *hwFrame  = nullptr;

    // Если HW — загружаем кадр в GPU память
    if (m_isHW && m_codecCtx->hw_frames_ctx) {
        hwFrame = av_frame_alloc();
        av_hwframe_get_buffer(m_codecCtx->hw_frames_ctx, hwFrame, 0);
        av_hwframe_transfer_data(hwFrame, swFrame, 0);
        hwFrame->pts = swFrame->pts;
        encFrame = hwFrame;
    }

    // Отправляем в кодер
    int ret = avcodec_send_frame(m_codecCtx, encFrame);

    if (hwFrame) av_frame_free(&hwFrame);
    av_frame_free(&swFrame);

    if (ret < 0) {
        errorOut = "Error sending frame to encoder";
        return false;
    }

    // Получаем закодированные пакеты
    AVPacket *pkt = av_packet_alloc();
    while (avcodec_receive_packet(m_codecCtx, pkt) == 0) {
        pkt->stream_index = m_videoStream->index;
        av_packet_rescale_ts(pkt, m_codecCtx->time_base,
                             m_videoStream->time_base);
        av_interleaved_write_frame(m_fmtCtx, pkt);
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// finalize() — flush + аудио + trailer
// ─────────────────────────────────────────────────────────────────────────────

bool VideoEncoder::finalize(QString &errorOut)
{
    if (!m_fmtCtx || !m_codecCtx) {
        errorOut = "Encoder not opened";
        return false;
    }

    // Flush видео кодера
    avcodec_send_frame(m_codecCtx, nullptr);
    AVPacket *pkt = av_packet_alloc();
    while (avcodec_receive_packet(m_codecCtx, pkt) == 0) {
        pkt->stream_index = m_videoStream->index;
        av_packet_rescale_ts(pkt, m_codecCtx->time_base,
                             m_videoStream->time_base);
        av_interleaved_write_frame(m_fmtCtx, pkt);
        av_packet_unref(pkt);
    }

    // Копируем аудио пакеты из оригинала
    if (m_srcAudioFmt && m_srcAudioStream >= 0 && m_audioStream) {
        qDebug() << "[VideoEncoder] Copying audio...";
        av_seek_frame(m_srcAudioFmt, m_srcAudioStream, 0, AVSEEK_FLAG_BACKWARD);

        while (av_read_frame(m_srcAudioFmt, pkt) >= 0) {
            if (pkt->stream_index != m_srcAudioStream) {
                av_packet_unref(pkt);
                continue;
            }
            pkt->stream_index = m_audioStream->index;
            av_packet_rescale_ts(
                pkt,
                m_srcAudioFmt->streams[m_srcAudioStream]->time_base,
                m_audioStream->time_base);
            av_interleaved_write_frame(m_fmtCtx, pkt);
            av_packet_unref(pkt);
        }
    }

    av_packet_free(&pkt);

    // Пишем trailer
    if (av_write_trailer(m_fmtCtx) < 0) {
        errorOut = "Cannot write file trailer";
        return false;
    }

    qDebug() << "[VideoEncoder] Finalized," << m_frameIdx << "frames written";
    return true;
}
