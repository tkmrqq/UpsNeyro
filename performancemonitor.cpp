#include "performancemonitor.h"
#include <algorithm>

PerformanceMonitor::PerformanceMonitor(QObject *parent)
    : QObject(parent)
{
    m_fpsTimer.start();
}

void PerformanceMonitor::recordFrame(const FrameTimings &timings)
{
    QMutexLocker lock(&m_mutex);

    m_window.enqueue(timings);
    if (m_window.size() > WINDOW_SIZE)
        m_window.dequeue();

    m_totalFrames++;
    recalculate();

    emit statsUpdated();
}

void PerformanceMonitor::recalculate()
{
    // Вызывается под локом m_mutex
    if (m_window.isEmpty())
        return;

    const int count = m_window.size();

    qint64 sumDecode = 0, sumUpscale = 0, sumFilter = 0, sumEncode = 0;
    for (const auto &f : m_window)
    {
        sumDecode += f.decodeUs;
        sumUpscale += f.upscaleUs;
        sumFilter += f.filterUs;
        sumEncode += f.encodeUs;
    }

    m_avgDecodeMs = (sumDecode / count) / 1000.0f;
    m_avgUpscaleMs = (sumUpscale / count) / 1000.0f;
    m_avgFilterMs = (sumFilter / count) / 1000.0f;
    m_avgEncodeMs = (sumEncode / count) / 1000.0f;

    // FPS по реальному времени последних WINDOW_SIZE кадров
    const float windowSec = m_fpsTimer.elapsed() / 1000.0f;
    if (windowSec > 0.0f)
        m_currentFps = count / windowSec;

    // Bottleneck — стадия с максимальным средним временем
    struct Stage
    {
        QString name;
        float ms;
    };
    const std::initializer_list<Stage> stages = {
        {"decode", m_avgDecodeMs},
        {"upscale", m_avgUpscaleMs},
        {"filter", m_avgFilterMs},
        {"encode", m_avgEncodeMs},
    };
    m_bottleneck = std::max_element(
                       stages.begin(), stages.end(),
                       [](const Stage &a, const Stage &b)
                       { return a.ms < b.ms; })
                       ->name;
}

void PerformanceMonitor::reset()
{
    QMutexLocker lock(&m_mutex);
    m_window.clear();
    m_totalFrames = 0;
    m_currentFps = 0.0f;
    m_avgUpscaleMs = 0.0f;
    m_avgDecodeMs = 0.0f;
    m_avgEncodeMs = 0.0f;
    m_avgFilterMs = 0.0f;
    m_bottleneck.clear();
    m_fpsTimer.restart();
    emit statsUpdated();
}

float PerformanceMonitor::currentFps() const
{
    QMutexLocker l(&m_mutex);
    return m_currentFps;
}
float PerformanceMonitor::avgUpscaleMs() const
{
    QMutexLocker l(&m_mutex);
    return m_avgUpscaleMs;
}
float PerformanceMonitor::avgDecodeMs() const
{
    QMutexLocker l(&m_mutex);
    return m_avgDecodeMs;
}
float PerformanceMonitor::avgEncodeMs() const
{
    QMutexLocker l(&m_mutex);
    return m_avgEncodeMs;
}
float PerformanceMonitor::avgFilterMs() const
{
    QMutexLocker l(&m_mutex);
    return m_avgFilterMs;
}
QString PerformanceMonitor::bottleneck() const
{
    QMutexLocker l(&m_mutex);
    return m_bottleneck;
}
int PerformanceMonitor::processedFrames() const
{
    QMutexLocker l(&m_mutex);
    return m_totalFrames;
}
