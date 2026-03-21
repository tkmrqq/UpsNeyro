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

// Настройки кодирования
struct EncodeSettings {
    int     width      = 0;
    int     height     = 0;
    double  fps        = 25.0;
    int     quality    = 80;    // 0-100, конвертируется в CRF
    bool    copyAudio  = true;
    QString audioSourcePath;    // путь к оригинальному видео для аудио
};

class VideoEncoder : public QObject
{
    Q_OBJECT

public:
    explicit VideoEncoder(QObject *parent = nullptr);
    ~VideoEncoder();

    // Открыть выходной файл и инициализировать кодек
    // Автоматически выбирает hw encoder: NVENC → AMF → QSV → CPU x264
    bool open(const QString &outputPath,
              const EncodeSettings &settings,
              QString &errorOut);

    // Закодировать один RGB24 кадр
    bool encodeFrame(const uint8_t *rgbData, int width, int height,
                     QString &errorOut);

    // Завершить кодирование (flush + финализация файла)
    bool finalize(QString &errorOut);

    void close();

    // Какой кодек используется
    QString codecName() const { return m_codecName; }

private:
    const AVCodec* findBestEncoder();
    bool           initHWEncoder(const AVCodec *codec);

    AVFormatContext *m_fmtCtx    = nullptr;
    AVCodecContext  *m_codecCtx  = nullptr;
    AVBufferRef     *m_hwDevCtx  = nullptr;
    AVStream        *m_videoStream = nullptr;
    AVStream        *m_audioStream = nullptr;

    // Для копирования аудио из оригинала
    AVFormatContext *m_srcAudioFmt  = nullptr;
    AVCodecContext  *m_srcAudioCtx  = nullptr;
    int              m_srcAudioStream = -1;

    SwsContext      *m_swsCtx    = nullptr;

    EncodeSettings   m_settings;
    QString          m_codecName;
    int64_t          m_frameIdx  = 0;
    bool             m_isHW      = false;
};
