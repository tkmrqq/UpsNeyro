#include "upscalemanager.h"

#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QtConcurrent>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UpscaleManager::UpscaleManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "[UpscaleManager] created, model:" << modelName();

    connect(&m_pipeline, &PipelineManager::busyChanged,
            this, &UpscaleManager::upscaleBusyChanged);
    connect(&m_pipeline, &PipelineManager::progressChanged,
            this, &UpscaleManager::upscaleProgressChanged);
    connect(&m_pipeline, &PipelineManager::statusChanged,
            this, &UpscaleManager::upscaleStatusChanged);
    connect(&m_pipeline, &PipelineManager::etaChanged,
            this, &UpscaleManager::upscaleEtaChanged);
    connect(&m_pipeline, &PipelineManager::hwDecoderChanged,
            this, &UpscaleManager::hwDecoderChanged);
    connect(&m_pipeline, &PipelineManager::hwEncoderChanged,
            this, &UpscaleManager::hwEncoderChanged);
    connect(&m_pipeline, &PipelineManager::finished,
            this, &UpscaleManager::upscaleFinished);
    connect(&m_pipeline, &PipelineManager::failed,
            this, &UpscaleManager::upscaleFailed);

    // ── realesrgan превью: finished ───────────────────────────────────────────
    connect(&m_upscaleProc, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                setPreviewProgress(100);
                if (status != QProcess::NormalExit || exitCode != 0) {
                    QString err = QString::fromLocal8Bit(m_upscaleProc.readAllStandardError());
                    qDebug().noquote() << "[UpscaleManager] Preview upscaler stderr:\n" << err;
                    setPreviewStatus(QStringLiteral("Upscale failed"));
                    setPreviewBusy(false);
                    emit previewFailed(QStringLiteral("Upscale failed (see debug output)"));
                    return;
                }
                setPreviewStatus(QStringLiteral("Done"));
                setPreviewBusy(false);
                emit previewReady(
                    QUrl::fromLocalFile(m_framePath).toString(),
                    QUrl::fromLocalFile(m_upscaledFramePath).toString()
                    );
            });

    connect(&m_upscaleProc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError err) {
                qWarning() << "[UpscaleManager] Preview upscaler error:" << err;
                setPreviewBusy(false);
                emit previewFailed(QStringLiteral("realesrgan-ncnn-vulkan not found"));
            });

    connect(&m_upscaleProc, &QProcess::readyReadStandardError, this, [this]() {
        const QString line =
            QString::fromLocal8Bit(m_upscaleProc.readAllStandardError()).trimmed();
        if (line.isEmpty()) return;
        if (line.endsWith(u'%')) {
            bool ok = false;
            const double pct = line.chopped(1).toDouble(&ok);
            if (ok) setPreviewProgress(40 + static_cast<int>(pct * 0.55));
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// startUpscaling — запуск полного пайплайна
// ─────────────────────────────────────────────────────────────────────────────

void UpscaleManager::startUpscaling(const QString &videoPath,
                                    const QString &outputDir)
{
    const QString clean = cleanVideoPath(videoPath);
    m_pipeline.startFromQml(
        clean,
        outputDir,
        modelName(),
        scaleForResolution(),
        m_outputQuality,
        QStringLiteral("auto")
        );
}

// ─────────────────────────────────────────────────────────────────────────────
// cancelUpscaling
// ─────────────────────────────────────────────────────────────────────────────
void UpscaleManager::cancelUpscaling()
{
    m_pipeline.cancel();
}

// ─────────────────────────────────────────────────────────────────────────────
// startPreview (без изменений)
// ─────────────────────────────────────────────────────────────────────────────

void UpscaleManager::startPreview(const QString &videoPath, double positionSec)
{
    if (m_previewBusy) return;

    if (videoPath.isEmpty()) {
        emit previewFailed(QStringLiteral("No video selected"));
        return;
    }

    const QString tmpDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + QStringLiteral("/upsneyro_preview");
    QDir().mkpath(tmpDir);

    m_framePath         = tmpDir + QStringLiteral("/frame_orig.png");
    m_upscaledFramePath = tmpDir + QStringLiteral("/frame_upscaled.png");

    setPreviewBusy(true);
    setPreviewProgress(0);
    setPreviewStatus(QStringLiteral("Capturing frame..."));

    const QString clean = cleanVideoPath(videoPath);

    if (clean == m_lastVideoPath
        && qAbs(positionSec - m_lastPositionSec) < 0.05
        && QFile::exists(m_upscaledFramePath))
    {
        qDebug() << "[UpscaleManager] Using cached preview";
        setPreviewBusy(false);
        emit previewReady(
            QUrl::fromLocalFile(m_framePath).toString(),
            QUrl::fromLocalFile(m_upscaledFramePath).toString()
            );
        return;
    }

    m_lastVideoPath   = clean;
    m_lastPositionSec = positionSec;

    QFile::remove(m_framePath);
    QFile::remove(m_upscaledFramePath);

    setPreviewProgress(10);

    QtConcurrent::run([this, clean, positionSec]() {
        QString error;
        bool ok = m_frameCapture.captureFrame(clean, positionSec, m_framePath, error);

        if (ok) {
            QMetaObject::invokeMethod(this, [this]() {
                    setPreviewStatus(QStringLiteral("Upscaling frame..."));
                    setPreviewProgress(40);
                    runUpscalerForPreview();
                }, Qt::QueuedConnection);
        } else {
            qDebug().noquote() << "[UpscaleManager] Frame capture error:" << error;
            QMetaObject::invokeMethod(this, [this, error]() {
                    setPreviewStatus(QStringLiteral("Frame capture failed"));
                    setPreviewBusy(false);
                    emit previewFailed(error);
                }, Qt::QueuedConnection);
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Slots
// ─────────────────────────────────────────────────────────────────────────────

void UpscaleManager::setMode(UpscaleMode m)
{
    if (m_mode == m) return;
    m_mode = m;
    qDebug() << "[UpscaleManager] mode:" << modelName();
    emit modeChanged();
}

void UpscaleManager::setResolution(const QString &r)
{
    if (m_resolution == r) return;
    m_resolution = r;
    emit resolutionChanged();
}

void UpscaleManager::setDenoise(bool d)
{
    if (m_denoise == d) return;
    m_denoise = d;
    emit denoiseChanged();
}

void UpscaleManager::setOutputQuality(int q)
{
    if (m_outputQuality == q) return;
    m_outputQuality = q;
    emit outputQualityChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void UpscaleManager::setPreviewBusy(bool b)
{
    if (m_previewBusy == b) return;
    m_previewBusy = b;
    emit previewBusyChanged();
}

void UpscaleManager::setPreviewStatus(const QString &s)
{
    if (m_previewStatus == s) return;
    m_previewStatus = s;
    emit previewStatusChanged();
}

void UpscaleManager::setPreviewProgress(int p)
{
    if (m_previewProgress == p) return;
    m_previewProgress = p;
    emit previewProgressChanged();
}

void UpscaleManager::runUpscalerForPreview()
{
    const int     scale    = scaleForResolution();
    const QString binary   = upscalerBinaryPath();
    const QString modelDir = QStringLiteral(REALESRGAN_DIR) + QStringLiteral("/models");
    const QString model    = modelNameForMode(m_mode);

    qDebug() << "[UpscaleManager] runUpscalerForPreview, model:" << model << "scale:" << scale;

    m_upscaleProc.start(binary, {
                                    QStringLiteral("-i"), m_framePath,
                                    QStringLiteral("-o"), m_upscaledFramePath,
                                    QStringLiteral("-s"), QString::number(scale),
                                    QStringLiteral("-n"), model,
                                    QStringLiteral("-m"), modelDir
                                });
}

QString UpscaleManager::modelNameForMode(UpscaleMode mode) const
{
    switch (mode) {
    case FastMode:     return QStringLiteral("realesr-animevideov3");
    case BalancedMode: return QStringLiteral("realesrgan-x4plus");
    case QualityMode:  return QStringLiteral("realesrgan-x4plus-anime");
    default:           return QStringLiteral("realesrgan-x4plus");
    }
}

QString UpscaleManager::upscalerBinaryPath() const
{
    QString path = QStringLiteral(REALESRGAN_DIR) + QStringLiteral("/realesrgan-ncnn-vulkan");
#ifdef Q_OS_WIN
    path += QStringLiteral(".exe");
#endif
    return path;
}

QString UpscaleManager::cleanVideoPath(const QString &videoPath) const
{
    QString clean;
    QUrl url(videoPath);
    if (url.isLocalFile()) {
        clean = url.toLocalFile();
    } else if (videoPath.startsWith(u'/')) {
        clean = videoPath;
    } else {
        clean = u'/' + videoPath;
    }
#ifdef Q_OS_WIN
    if (clean.length() >= 3 && clean[0] == u'/' && clean[2] == u':')
        clean = clean.mid(1);
#endif
    return clean;
}

int UpscaleManager::scaleForResolution() const
{
    if (m_resolution == QStringLiteral("1080p") ||
        m_resolution == QStringLiteral("2K"))
        return 2;
    return 4; // 4K, 8K
}
