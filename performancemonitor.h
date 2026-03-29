#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QQueue>
#include <QMutex>
#include <QtQml/qqmlregistration.h>

// Замер времён одного кадра по стадиям пайплайна
struct FrameTimings
{
    qint64 decodeUs = 0;  // декодирование (FFmpeg)
    qint64 upscaleUs = 0; // апскейл (Python/CUDA)
    qint64 filterUs = 0;  // фильтры (FFmpeg filtergraph)
    qint64 encodeUs = 0;  // кодирование (FFmpeg)
};

class PerformanceMonitor : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(float currentFps READ currentFps NOTIFY statsUpdated)
    Q_PROPERTY(float avgUpscaleMs READ avgUpscaleMs NOTIFY statsUpdated)
    Q_PROPERTY(float avgDecodeMs READ avgDecodeMs NOTIFY statsUpdated)
    Q_PROPERTY(float avgEncodeMs READ avgEncodeMs NOTIFY statsUpdated)
    Q_PROPERTY(float avgFilterMs READ avgFilterMs NOTIFY statsUpdated)
    Q_PROPERTY(QString bottleneck READ bottleneck NOTIFY statsUpdated)
    Q_PROPERTY(int processedFrames READ processedFrames NOTIFY statsUpdated)

public:
    explicit PerformanceMonitor(QObject *parent = nullptr);

    // Вызывается из PipelineManager после завершения каждого кадра
    void recordFrame(const FrameTimings &timings);

    void reset();

    float currentFps() const;
    float avgUpscaleMs() const;
    float avgDecodeMs() const;
    float avgEncodeMs() const;
    float avgFilterMs() const;
    QString bottleneck() const; // "decode" | "upscale" | "encode" | "filter"
    int processedFrames() const;

signals:
    void statsUpdated();

private:
    void recalculate();

    static constexpr int WINDOW_SIZE = 30; // скользящее окно в кадрах

    mutable QMutex m_mutex;
    QQueue<FrameTimings> m_window;
    QElapsedTimer m_fpsTimer;

    // Кэш пересчитанных значений — обновляется в recalculate()
    float m_currentFps = 0.0f;
    float m_avgUpscaleMs = 0.0f;
    float m_avgDecodeMs = 0.0f;
    float m_avgEncodeMs = 0.0f;
    float m_avgFilterMs = 0.0f;
    QString m_bottleneck;
    int m_totalFrames = 0;
};

#endif // PERFORMANCEMONITOR_H
