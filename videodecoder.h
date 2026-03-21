#pragma once

#include <QObject>
#include <QString>
#include <functional>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// Информация о видео
struct VideoInfo {
    int     width       = 0;
    int     height      = 0;
    int     totalFrames = 0;
    double  fps         = 0.0;
    double  duration    = 0.0;   // секунды
    bool    hasAudio    = false;
    QString codec;
    QString hwDevice;            // какой hw бэкенд используется
};

// Колбэк на каждый декодированный кадр
// data   — указатель на RGB24 пиксели
// size   — размер буфера в байтах (w * h * 3)
// frameIdx — номер кадра (0-based)
using FrameCallback = std::function<void(const uint8_t *data, int size, int frameIdx)>;

class VideoDecoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoDecoder(QObject *parent = nullptr);
    ~VideoDecoder();

    // Открыть файл, определить hw бэкенд, заполнить VideoInfo
    bool open(const QString &path, QString &errorOut);

    // Информация о видео (доступна после open())
    VideoInfo info() const { return m_info; }

    // Запустить декодирование всех кадров
    // onFrame вызывается в рабочем потоке для каждого кадра
    // Возвращает true если дошли до конца без ошибок
    bool decode(FrameCallback onFrame, QString &errorOut);

    // Остановить декодирование (thread-safe)
    void stop();

    void close();

private:
    AVHWDeviceType  detectBestHWDevice();
    bool            initHWDecoder(AVHWDeviceType type);
    bool            transferFrameToRGB(AVFrame *frame,
                            AVFrame *rgbFrame,
                            SwsContext *&swsCtx);

    // libav контексты
    AVFormatContext *m_fmt       = nullptr;
    AVCodecContext  *m_codecCtx  = nullptr;
    AVBufferRef     *m_hwDevCtx  = nullptr;
    int              m_videoStreamIdx = -1;

    VideoInfo   m_info;
    bool        m_stopRequested = false;
};
