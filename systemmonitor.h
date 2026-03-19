#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QProcess>

class SystemMonitor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(double cpuLoad READ cpuLoad NOTIFY cpuLoadChanged)
    Q_PROPERTY(double ramLoad READ ramLoad NOTIFY ramLoadChanged)
    Q_PROPERTY(QString ramText READ ramText NOTIFY ramLoadChanged)
    Q_PROPERTY(double gpuLoad READ gpuLoad NOTIFY gpuLoadChanged)
    Q_PROPERTY(double vramLoad READ vramLoad NOTIFY vramLoadChanged)
    Q_PROPERTY(QString vramText READ vramText NOTIFY vramLoadChanged)
    Q_PROPERTY(QString cpuName READ cpuName NOTIFY cpuNameChanged)
    Q_PROPERTY(QString gpuName READ gpuName NOTIFY gpuNameChanged)

public:
    explicit SystemMonitor(QObject *parent = nullptr);

    double cpuLoad() const { return m_cpuLoad; }
    double ramLoad() const { return m_ramLoad; }
    QString ramText() const { return m_ramText; }
    double gpuLoad() const { return m_gpuLoad; }
    double vramLoad() const { return m_vramLoad; }
    QString vramText() const { return m_vramText; }
    QString cpuName() const { return m_cpuName; }
    QString gpuName() const { return m_gpuName; }

signals:
    void cpuLoadChanged();
    void ramLoadChanged();
    void gpuLoadChanged();
    void vramLoadChanged();
    void cpuNameChanged();
    void gpuNameChanged();

private slots:
    void updateStats();

private:
    void fetchHardwareNames();
    void updateCpuUsage();
    void updateRamUsage();
    void updateGpuUsage();
    void detectGpuName();

    double m_cpuLoad = 0.0;
    double m_ramLoad = 0.0;
    QString m_ramText = "";

    double m_gpuLoad = 0.0;
    double m_vramLoad = 0.0;
    QString m_vramText = "0.0 GB / 0.0 GB";

    QTimer *m_timer;
    // Переменные для вычисления дельты загрузки CPU
    quint64 m_prevTotal = 0;
    quint64 m_prevIdle = 0;

    // Пока захардкодим названия, позже можно будет получать их из системы
    QString m_cpuName = "CPU";
    QString m_gpuName = "GPU";
};

#endif // SYSTEMMONITOR_H
