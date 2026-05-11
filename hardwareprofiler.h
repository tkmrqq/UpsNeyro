#ifndef HARDWAREPROFILER_H
#define HARDWAREPROFILER_H

#include <QObject>
#include <QString>

/**
 * Профилирование железа и рекомендации (CUDA / CPU / auto) для пайплайна.
 * Дополняет SystemMonitor (метрики в UI) более статическим анализом.
 */
class HardwareProfiler : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString cpuSummary READ cpuSummary NOTIFY profileChanged)
    Q_PROPERTY(QString gpuSummary READ gpuSummary NOTIFY profileChanged)
    Q_PROPERTY(QString recommendedDevice READ recommendedDevice NOTIFY profileChanged)
    Q_PROPERTY(QString recommendationHint READ recommendationHint NOTIFY profileChanged)

public:
    explicit HardwareProfiler(QObject *parent = nullptr);

    QString cpuSummary() const { return m_cpuSummary; }
    QString gpuSummary() const { return m_gpuSummary; }
    QString recommendedDevice() const { return m_recommendedDevice; }
    QString recommendationHint() const { return m_recommendationHint; }

    Q_INVOKABLE void refresh();

signals:
    void profileChanged();

private:
    void detectCudaAvailability();

    QString m_cpuSummary;
    QString m_gpuSummary;
    QString m_recommendedDevice;
    QString m_recommendationHint;
};

#endif // HARDWAREPROFILER_H
