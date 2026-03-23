#pragma once

#include <QObject>
#include <QString>
#include <QThread>
#include <QDateTime>
#include "videodecoder.h"
#include "pythonupscaler.h"
#include "videoencoder.h"
#include "framefilter.h"

// Настройки всего пайплайна
struct PipelineSettings {
    // Входной файл
    QString inputPath;

    // Выходная папка и имя
    QString outputDir;

    // Апскейл
    QString model    = "realesrgan-x4plus";
    int     scale    = 4;
    QString device   = "auto";   // auto / cuda / cpu

    // Кодирование
    int     quality  = 80;       // 0-100
    bool    copyAudio = true;

    // Python
    QString pythonExe;           // путь к python/python3
    QString scriptPath;          // путь к upscaler.py
    FilterParams filters;
};

class PipelineManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool   busy        READ busy        NOTIFY busyChanged)
    Q_PROPERTY(int    progress    READ progress    NOTIFY progressChanged)
    Q_PROPERTY(QString status     READ status      NOTIFY statusChanged)
    Q_PROPERTY(QString eta        READ eta         NOTIFY etaChanged)
    Q_PROPERTY(QString hwDecoder  READ hwDecoder   NOTIFY hwDecoderChanged)
    Q_PROPERTY(QString hwEncoder  READ hwEncoder   NOTIFY hwEncoderChanged)

public:
    explicit PipelineManager(QObject *parent = nullptr);
    ~PipelineManager();

    bool    busy()       const { return m_busy; }
    int     progress()   const { return m_progress; }
    QString status()     const { return m_status; }
    QString eta()        const { return m_eta; }
    QString hwDecoder()  const { return m_hwDecoder; }
    QString hwEncoder()  const { return m_hwEncoder; }

    // Запустить пайплайн (не блокирует UI — всё в отдельном потоке)
    Q_INVOKABLE void start(const PipelineSettings &settings);

    // Остановить
    Q_INVOKABLE void cancel();

    // Удобный invokable для QML — принимает параметры напрямую
    Q_INVOKABLE void startFromQml(const QString &inputPath,
                                  const QString &outputDir,
                                  const QString &model,
                                  int scale,
                                  int quality,
                                  const QString &device,
                                  const FilterParams &filters = FilterParams{});

signals:
    void busyChanged();
    void progressChanged();
    void statusChanged();
    void etaChanged();
    void hwDecoderChanged();
    void hwEncoderChanged();

    void finished(QString outputPath);
    void failed(QString error);

private:
    void runPipeline(PipelineSettings settings);

    void setBusy(bool v);
    void setProgress(int v);
    void setStatus(const QString &v);
    void setEta(const QString &v);
    void setHwDecoder(const QString &v);
    void setHwEncoder(const QString &v);
    void updateEta(int processed, int total, qint64 startMs);

    bool        m_busy      = false;
    int         m_progress  = 0;
    QString     m_status;
    QString     m_eta;
    QString     m_hwDecoder;
    QString     m_hwEncoder;

    FrameFilter m_filter;
    // Флаг отмены — читается из рабочего потока
    std::atomic<bool> m_cancelRequested{false};

    QThread         *m_workerThread  = nullptr;
    VideoDecoder    *m_decoder       = nullptr;
    PythonUpscaler  *m_upscaler      = nullptr;
    VideoEncoder    *m_encoder       = nullptr;
};
