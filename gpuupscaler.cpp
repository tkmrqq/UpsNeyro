#include "gpuupscaler.h"
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

GpuUpscaler::GpuUpscaler(QObject *parent)
    : QObject(parent)
{
    connect(&m_proc, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                setBusy(false);
                if (status != QProcess::NormalExit || exitCode != 0) {
                    setLastError(QStringLiteral("Upscale failed: %1")
                                     .arg(QString::fromLocal8Bit(m_proc.readAllStandardError())));
                    emit upscaleFinished(false);
                    return;
                }

                // ожидаем, что мы сами заранее выбрали путь к выходному файлу
                emit outputImageChanged();
                emit upscaleFinished(true);
            });

    connect(&m_proc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError err) {
                setBusy(false);
                setLastError(QStringLiteral("Process error: %1").arg(err));
                emit upscaleFinished(false);
            });
}

void GpuUpscaler::setInputImage(const QUrl &url)
{
    if (m_inputImage == url)
        return;
    m_inputImage = url;
    emit inputImageChanged();
}

void GpuUpscaler::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}

void GpuUpscaler::setLastError(const QString &err)
{
    if (m_lastError == err) return;
    m_lastError = err;
    emit lastErrorChanged();
}

void GpuUpscaler::startUpscale(int scale)
{
    if (m_busy)
        return;

    setLastError(QString());

    if (!m_inputImage.isValid()) {
        setLastError(QStringLiteral("Input image not set"));
        emit upscaleFinished(false);
        return;
    }

    // Локальный путь к входному файлу (file:/// → обычный путь)
    const QString inPath = m_inputImage.toLocalFile();
    if (inPath.isEmpty()) {
        setLastError(QStringLiteral("Invalid input path"));
        emit upscaleFinished(false);
        return;
    }

    QFileInfo inInfo(inPath);
    if (!inInfo.exists()) {
        setLastError(QStringLiteral("Input file does not exist"));
        emit upscaleFinished(false);
        return;
    }

    // Папка для выходных файлов (например, ~/.local/share/YourApp/upsamples)
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(baseDir + "/upsamples");

    QString outPath = baseDir + "/upsamples/" + inInfo.completeBaseName()
                      + QStringLiteral("_x%1.png").arg(scale);

    m_outputImage = QUrl::fromLocalFile(outPath);

    // Команда realesrgan-ncnn-vulkan должна быть в PATH
    QString program = QStringLiteral("realesrgan-ncnn-vulkan");
    QStringList args;
    args << "-i" << inPath
         << "-o" << outPath
         << "-s" << QString::number(scale);

    qDebug() << "Starting upscaler:"
             << "program =" << program
             << "args =" << args;

    setBusy(true);
    m_proc.start(program, args);
}
