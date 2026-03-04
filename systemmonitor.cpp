#include "systemmonitor.h"
#include <QFile>
#include <QTextStream>
#include <QtGlobal> // Для макросов Q_OS_WIN и Q_OS_LINUX

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::updateStats);
    m_timer->start(1000); // Раз в 1 секунду (оптимально для расчета CPU)

    // Делаем первый замер сразу
    updateStats();
}

void SystemMonitor::updateStats() {
    updateRamUsage();
    updateCpuUsage();
}

void SystemMonitor::updateRamUsage() {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        double totalPhysMem = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double usedPhysMem = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);

        m_ramLoad = memInfo.dwMemoryLoad;
        m_ramText = QString::number(usedPhysMem, 'f', 1) + " GB / " + QString::number(totalPhysMem, 'f', 1) + " GB";
        emit ramLoadChanged();
    }
#elif defined(Q_OS_LINUX)
    QFile file("/proc/meminfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line;
        double memTotal = 0;
        double memAvailable = 0;

        while (in.readLineInto(&line)) {
            if (line.startsWith("MemTotal:")) {
                memTotal = line.section(' ', -2, -2).toDouble() / (1024.0 * 1024.0); // В Гигабайты
            } else if (line.startsWith("MemAvailable:")) {
                memAvailable = line.section(' ', -2, -2).toDouble() / (1024.0 * 1024.0);
            }
        }
        file.close();

        if (memTotal > 0) {
            double usedMem = memTotal - memAvailable;
            m_ramLoad = (usedMem / memTotal) * 100.0;
            m_ramText = QString::number(usedMem, 'f', 1) + " GB / " + QString::number(memTotal, 'f', 1) + " GB";
            emit ramLoadChanged();
        }
    }
#endif
}

// Вспомогательная функция для Windows CPU
#ifdef Q_OS_WIN
static quint64 fileTimeToInt64(const FILETIME &ft) {
    return (static_cast<quint64>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}
#endif

void SystemMonitor::updateCpuUsage() {
#ifdef Q_OS_WIN
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        quint64 idle = fileTimeToInt64(idleTime);
        quint64 total = fileTimeToInt64(kernelTime) + fileTimeToInt64(userTime);

        // Если это не первый запуск
        if (m_prevTotal != 0) {
            quint64 diffTotal = total - m_prevTotal;
            quint64 diffIdle = idle - m_prevIdle;
            if (diffTotal > 0) {
                m_cpuLoad = 100.0 - (diffIdle * 100.0) / diffTotal;
                emit cpuLoadChanged();
            }
        }
        // Запоминаем для следующего раза
        m_prevTotal = total;
        m_prevIdle = idle;
    }
#elif defined(Q_OS_LINUX)
    QFile file("/proc/stat");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line = in.readLine(); // Первая строка содержит общую статистику CPU
        file.close();

        if (line.startsWith("cpu ")) {
            QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (parts.size() > 4) {
                // Поля: cpu user nice system idle iowait irq softirq steal
                quint64 idle = parts[4].toULongLong() + (parts.size() > 5 ? parts[5].toULongLong() : 0);
                quint64 nonIdle = parts[1].toULongLong() + parts[2].toULongLong() + parts[3].toULongLong() +
                                  (parts.size() > 6 ? parts[6].toULongLong() : 0) +
                                  (parts.size() > 7 ? parts[7].toULongLong() : 0) +
                                  (parts.size() > 8 ? parts[8].toULongLong() : 0);
                quint64 total = idle + nonIdle;

                if (m_prevTotal != 0) {
                    quint64 diffTotal = total - m_prevTotal;
                    quint64 diffIdle = idle - m_prevIdle;
                    if (diffTotal > 0) {
                        m_cpuLoad = 100.0 - (diffIdle * 100.0) / diffTotal;
                        emit cpuLoadChanged();
                    }
                }
                m_prevTotal = total;
                m_prevIdle = idle;
            }
        }
    }
#endif
}
