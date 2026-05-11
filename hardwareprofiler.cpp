#include "hardwareprofiler.h"
#include <QSysInfo>
#include <QProcess>
#include <QtGlobal>

HardwareProfiler::HardwareProfiler(QObject *parent)
    : QObject(parent)
{
    refresh();
}

void HardwareProfiler::detectCudaAvailability()
{
    bool cudaLikely = false;
#ifdef Q_OS_WIN
    QProcess p;
    p.start("where", QStringList() << "nvidia-smi");
    if (p.waitForFinished(800) && p.exitCode() == 0)
        cudaLikely = true;
#else
    QProcess p;
    p.start("which", QStringList() << "nvidia-smi");
    if (p.waitForFinished(800) && p.exitCode() == 0)
        cudaLikely = true;
#endif
    if (cudaLikely) {
        m_recommendedDevice = QStringLiteral("cuda");
        m_recommendationHint = tr("NVIDIA GPU detected — CUDA acceleration for upscale and filters.");
    } else {
        m_recommendedDevice = QStringLiteral("cpu");
        m_recommendationHint = tr("No NVIDIA driver tools in PATH — using CPU (slower). Install CUDA toolkit if you have an NVIDIA GPU.");
    }
}

void HardwareProfiler::refresh()
{
    m_cpuSummary = QSysInfo::prettyProductName() + QLatin1String(" · ") + QSysInfo::currentCpuArchitecture();
    m_gpuSummary = tr("Run-time GPU load is shown in Resource Monitor; static detection below.");

    detectCudaAvailability();

    emit profileChanged();
}
