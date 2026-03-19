#include "systemmonitor.h"
#include <QFile>
#include <QTextStream>
#include <QSettings>
#include <QtGlobal> // Для макросов Q_OS_WIN и Q_OS_LINUX

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SystemMonitor::SystemMonitor(QObject *parent) : QObject(parent) {
    //фетчим железки при запуске
    fetchHardwareNames();
    detectGpuName();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemMonitor::updateStats);
    m_timer->start(1000); // Раз в 1 секунду (оптимально для расчета CPU)

    // Делаем первый замер сразу
    updateStats();
}

void SystemMonitor::fetchHardwareNames() {
#ifdef Q_OS_WIN
    // Читаем название процессора из реестра Windows
    QSettings settings("HKEY_LOCAL_MACHINE\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", QSettings::NativeFormat);
    m_cpuName = settings.value("ProcessorNameString", "Unknown CPU").toString().trimmed();
#elif defined(Q_OS_LINUX)
    // Читаем название процессора из /proc/cpuinfo
    QFile file("/proc/cpuinfo");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString line;
        while (in.readLineInto(&line)) {
            if (line.startsWith("model name")) {
                m_cpuName = line.section(':', 1).trimmed();
                break;
            }
        }
    }
#endif
    emit cpuNameChanged();
}

void SystemMonitor::detectGpuName() {
    m_gpuName = "Unknown GPU"; // Значение по умолчанию

    // 1. Быстрая проверка на NVIDIA
    QProcess process;
    process.start("nvidia-smi", QStringList() << "--query-gpu=name" << "--format=csv,noheader");
    if (process.waitForFinished(300)) {
        QString output = process.readAllStandardOutput().trimmed();
        if (!output.isEmpty() && !output.contains("failed") && !output.contains("not recognized")) {
            m_gpuName = output;
            emit gpuNameChanged();
            return; // Нашли NVIDIA, выходим
        }
    }

#ifdef Q_OS_WIN
    // 2. Windows: ищем AMD, Intel и отсеиваем виртуалки
    QProcess psProcess;
    psProcess.start("powershell", QStringList() << "-NoProfile" << "-Command" << "(Get-CimInstance Win32_VideoController).Name");

    if (psProcess.waitForFinished(2000)) {
        QString output = QString::fromUtf8(psProcess.readAllStandardOutput()).trimmed();

        // Как показал дебаг, строки разделены \r\n
        QStringList lines = output.split("\r\n", Qt::SkipEmptyParts);

        for (const QString& line : lines) {
            QString cleanLine = line.trimmed();

            // Игнорируем пустые строки и виртуальные адаптеры
            if (cleanLine.isEmpty() ||
                cleanLine.contains("Parsec", Qt::CaseInsensitive) ||
                cleanLine.contains("Virtual", Qt::CaseInsensitive) ||
                cleanLine.contains("Basic", Qt::CaseInsensitive)) {
                continue;
            }

            // Нашли нормальную карту
            m_gpuName = cleanLine;

            // Убираем мусор из названия для красоты
            m_gpuName.replace("(TM)", "");
            m_gpuName.replace("(R)", "");
            m_gpuName = m_gpuName.trimmed();
            break; // Берем первую подошедшую и выходим из цикла
        }
    }
#elif defined(Q_OS_LINUX)
    // 3. Linux: используем lspci
    QProcess lspciProcess;
    lspciProcess.start("sh", QStringList() << "-c" << "lspci | grep -i vga");
    if (lspciProcess.waitForFinished(1000)) {
        QString output = QString::fromUtf8(lspciProcess.readAllStandardOutput()).trimmed();
        int colonIdx = output.indexOf(": ");
        if (colonIdx != -1) {
            QString fullName = output.mid(colonIdx + 2).trimmed();
            int bracketEnd = fullName.indexOf(']');
            m_gpuName = (bracketEnd != -1) ? fullName.mid(bracketEnd + 1).trimmed() : fullName;
        }
    }
#endif

    emit gpuNameChanged();
}


void SystemMonitor::updateStats() {
    updateRamUsage();
    updateCpuUsage();
    updateGpuUsage();
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

void SystemMonitor::updateGpuUsage() {
    // Если у нас NVIDIA, мы можем легко получить загрузку
    if (m_gpuName.contains("NVIDIA", Qt::CaseInsensitive)) {
        QProcess process;
        process.start("nvidia-smi", QStringList() << "--query-gpu=utilization.gpu,memory.used,memory.total" << "--format=csv,noheader,nounits");

        if (process.waitForFinished(300)) {
            QString output = process.readAllStandardOutput().trimmed();
            QStringList parts = output.split(',');
            if (parts.size() >= 3) {
                m_gpuLoad = parts[0].trimmed().toDouble();
                double memUsed = parts[1].trimmed().toDouble();
                double memTotal = parts[2].trimmed().toDouble();
                m_vramLoad = memTotal > 0 ? (memUsed / memTotal) * 100.0 : 0.0;

                emit gpuLoadChanged();
                emit vramLoadChanged();
                return;
            }
        }
    }

    // Для AMD и Intel на Windows без сложных C++ библиотек (PDH/DXGI)
    // получить процент нагрузки через командную строку нормально не выйдет.
    // Поэтому пока отдаем 0, чтобы UI не скакал.
    m_gpuLoad = 0.0;
    m_vramLoad = 0.0;

    emit gpuLoadChanged();
    emit vramLoadChanged();
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
