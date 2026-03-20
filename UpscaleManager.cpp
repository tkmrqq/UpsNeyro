#include "UpscaleManager.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QDebug>

UpscaleManager::UpscaleManager(QObject *parent)
    : QObject(parent)
{
    qDebug() << "[UpscaleManager] created";
    // ── FFmpeg: захват кадра ──────────────────────────────────────────────────
    connect(&m_ffmpegProc, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                if (status != QProcess::NormalExit || exitCode != 0) {
                QString err = QString::fromLocal8Bit(m_ffmpegProc.readAllStandardError());
                qDebug().noquote() << "[UpscaleManager] FFmpeg stderr:\n" << err;
                setPreviewStatus(QStringLiteral("Frame capture failed"));
                setPreviewBusy(false);
                emit previewFailed(QStringLiteral("Frame capture failed (see debug output)"));
                    return;
                }
                // Кадр захвачен — запускаем апскейл
                setPreviewStatus(QStringLiteral("Upscaling frame..."));
                setPreviewProgress(40);
                runUpscalerForPreview();
            });

    connect(&m_ffmpegProc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError /*err*/) {
                setPreviewStatus(QStringLiteral("FFmpeg not found or crashed"));
                setPreviewBusy(false);
                emit previewFailed(m_previewStatus);
            });

    // ── realesrgan: апскейл кадра ─────────────────────────────────────────────
    connect(&m_upscaleProc, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus status) {
                setPreviewProgress(100);
                if (status != QProcess::NormalExit || exitCode != 0) {
                    QString err = QString::fromLocal8Bit(m_upscaleProc.readAllStandardError());
                    qDebug().noquote() << "[UpscaleManager] Upscaler stderr:\n" << err;
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
            [this](QProcess::ProcessError /*err*/) {
                setPreviewStatus(
                    QStringLiteral("Upscaler not found. Check realesrgan-ncnn-vulkan in PATH"));
                setPreviewBusy(false);
                emit previewFailed(m_previewStatus);
            });

    // Читаем stderr апскейлера — он пишет прогресс в формате "xx.xx%"
    connect(&m_upscaleProc, &QProcess::readyReadStandardError, this, [this]() {
        const QString line =
            QString::fromLocal8Bit(m_upscaleProc.readAllStandardError()).trimmed();
        if (line.endsWith(u'%')) {
            bool ok = false;
            const double pct = line.chopped(1).toDouble(&ok);
            if (ok) {
                // Апскейл занимает диапазон 40–95%
                setPreviewProgress(40 + static_cast<int>(pct * 0.55));
            }
        }
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Public invokables
// ─────────────────────────────────────────────────────────────────────────────

void UpscaleManager::startUpscaling(const QString &videoPath, const QString &outputDir)
{
    qDebug() << "Starting upscale task!";
    qDebug() << "  Video:"      << videoPath;
    qDebug() << "  Output:"     << outputDir;
    qDebug() << "  Target Res:" << m_resolution;
    qDebug() << "  AI Mode:"    << m_mode;
    qDebug() << "  Denoise:"    << m_denoise;

    // TODO: запустить полный пайплайн через QProcess (FFmpeg + realesrgan)
}

void UpscaleManager::startPreview(const QString &videoPath, double positionSec)
{
    if (m_previewBusy)
        return;

    if (videoPath.isEmpty()) {
        emit previewFailed(QStringLiteral("No video selected"));
        return;
    }

    // Папка для временных файлов
    const QString tmpDir =
        QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + QStringLiteral("/upsneyro_preview");
    QDir().mkpath(tmpDir);

    m_framePath         = tmpDir + QStringLiteral("/frame_orig.png");
    m_upscaledFramePath = tmpDir + QStringLiteral("/frame_upscaled.png");

    // Удаляем старые файлы, чтобы QML не показал кэш
    QFile::remove(m_framePath);
    QFile::remove(m_upscaledFramePath);

    setPreviewBusy(true);
    setPreviewProgress(0);
    setPreviewStatus(QStringLiteral("Capturing frame..."));

    // Убираем схему file:/// если пришла из QML
    QString cleanPath;
    QUrl url(videoPath);
    if (url.isLocalFile()) {
        // Пришло как "file:///home/..." — конвертируем правильно
        cleanPath = url.toLocalFile();
    } else if (videoPath.startsWith("/")) {
        // Уже абсолютный путь
        cleanPath = videoPath;
    } else {
        // Относительный путь вроде "home/vladik/..." — добавляем слеш
        cleanPath = "/" + videoPath;
    }

    qDebug() << "[UpscaleManager] cleanPath:" << cleanPath;

    // Если тот же файл и та же позиция (с точностью 0.5с) — пропускаем захват
    if (cleanPath == m_lastVideoPath
        && qAbs(positionSec - m_lastPositionSec) < 0.5
        && QFile::exists(m_upscaledFramePath))
    {
        qDebug() << "[UpscaleManager] Using cached preview";
        emit previewReady(
            QUrl::fromLocalFile(m_framePath).toString(),
            QUrl::fromLocalFile(m_upscaledFramePath).toString()
            );
        return;
    }

    // Сохраняем для следующего вызова
    m_lastVideoPath    = cleanPath;
    m_lastPositionSec  = positionSec;

    // Удаляем старые файлы и запускаем FFmpeg
    QFile::remove(m_framePath);
    QFile::remove(m_upscaledFramePath);

    // ffmpeg -y -ss <pos> -i <input> -frames:v 1 -q:v 2 <output>
    const QStringList args = {
        QStringLiteral("-y"),
        QStringLiteral("-ss"), QString::number(positionSec, 'f', 3),
        QStringLiteral("-i"),  cleanPath,
        QStringLiteral("-frames:v"), QStringLiteral("1"),
        QStringLiteral("-q:v"),      QStringLiteral("2"),
        m_framePath
    };

    qDebug() << "FFmpeg args:" << args;
    m_ffmpegProc.start(QStringLiteral("ffmpeg"), args);
    setPreviewProgress(10);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public slots
// ─────────────────────────────────────────────────────────────────────────────

void UpscaleManager::setMode(UpscaleMode m)
{
    if (m_mode == m) return;
    m_mode = m;
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
    // realesrgan поддерживает максимум ×4; для 1080p/2K достаточно ×2
    int scale = 4;
    if (m_resolution == QStringLiteral("1080p") ||
        m_resolution == QStringLiteral("2K"))
    {
        scale = 2;
    }

    const QStringList args = {
        QStringLiteral("-i"), m_framePath,
        QStringLiteral("-o"), m_upscaledFramePath,
        QStringLiteral("-s"), QString::number(scale)
    };

    qDebug() << "Upscaler args:" << args;
    m_upscaleProc.start(QStringLiteral("realesrgan-ncnn-vulkan"), args);
}