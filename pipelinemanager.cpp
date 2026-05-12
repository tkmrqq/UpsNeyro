#include "pipelinemanager.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QtConcurrent>
#include "logger.h"
#include "targetresolution.h"

namespace {

#ifdef QT_NO_DEBUG
/** Release layout: <appDir>/python/python.exe (optional portable tree). */
QString resolveReleasePythonExe(const QString &appDir)
{
    const QString bundled =
        QDir(appDir).filePath(QStringLiteral("python/python.exe"));
    if (QFileInfo::exists(bundled))
        return QDir::toNativeSeparators(QFileInfo(bundled).absoluteFilePath());

    const QString onPath = QStandardPaths::findExecutable(QStringLiteral("python"));
    if (!onPath.isEmpty()) {
        qWarning() << "[Pipeline] Bundled Python not found at" << bundled
                   << "— using python from PATH:" << onPath;
        return onPath;
    }

    qWarning() << "[Pipeline] Python not found. Install portable Python under"
                << bundled << "or add python.exe to PATH.";
    return QDir::toNativeSeparators(QFileInfo(bundled).absoluteFilePath());
}
#endif

} // namespace

PipelineManager::PipelineManager(QObject *parent)
    : QObject(parent)
{
}

PipelineManager::~PipelineManager()
{
    cancel();
}

// ─────────────────────────────────────────────────────────────────────────────
// startFromQml — удобный метод для вызова из QML
// ─────────────────────────────────────────────────────────────────────────────

void PipelineManager::startFromQml(const QString &inputPath,
                                   const QString &outputDir,
                                   const QString &model,
                                   const QString &targetResolution,
                                   int quality,
                                   const QString &device,
                                   const FilterParams &filters)
{
    PipelineSettings s;
    s.inputPath = inputPath;
    s.outputDir = outputDir;
    s.model = model;
    s.targetResolution = targetResolution;
    s.quality = quality;
    s.device = device;
    s.copyAudio = true;
    s.filters = filters;

#ifdef QT_NO_DEBUG
    // Release: скрипт и веса рядом с exe (CMake copy_directory); Python — portable
    // в <appDir>/python/ или из PATH (см. resolveReleasePythonExe).
    const QString appDir = QCoreApplication::applicationDirPath();
    s.pythonExe = resolveReleasePythonExe(appDir);
    s.scriptPath = QDir(appDir).filePath(QStringLiteral("ai_engine/upscaler.py"));
#else
    // Debug: пути из CMake дефайнов (абсолютные пути проекта)
    s.pythonExe = QStringLiteral(PYTHON_EXE);
    s.scriptPath = QStringLiteral(AI_ENGINE_DIR) + "/upscaler.py";
#endif

    start(s);
}

// ─────────────────────────────────────────────────────────────────────────────
// start()
// ─────────────────────────────────────────────────────────────────────────────

void PipelineManager::start(const PipelineSettings &settings)
{
    if (m_busy)
    {
        qWarning() << "[PipelineManager] Already running";
        Logger::instance()->warning("[Pipeline] Already running, ignoring start");
        return;
    }
    Logger::instance()->info(QString("[Pipeline] Starting: %1 → %2")
        .arg(settings.inputPath, settings.outputDir));

    m_cancelRequested = false;
    setBusy(true);
    setProgress(0);
    setStatus(QStringLiteral("Initializing..."));
    setEta(QStringLiteral(""));

    // Запускаем пайплайн в отдельном потоке чтобы не фризить UI
    QtConcurrent::run([this, settings]()
                      { runPipeline(settings); });
}

// ─────────────────────────────────────────────────────────────────────────────
// cancel()
// ─────────────────────────────────────────────────────────────────────────────

void PipelineManager::cancel()
{
    if (!m_busy)
        return;

    qDebug() << "[PipelineManager] Cancelling...";
    Logger::instance()->info("[Pipeline] Cancel requested");
    m_cancelRequested = true;

    if (m_decoder)
        m_decoder->stop();
    if (m_upscaler)
        m_upscaler->stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// runPipeline() — выполняется в рабочем потоке
// ─────────────────────────────────────────────────────────────────────────────

void PipelineManager::runPipeline(PipelineSettings settings)
{
    auto fail = [this](const QString &err)
    {
        qWarning() << "[PipelineManager] FAILED:" << err;
        Logger::instance()->error("[PipelineManager] FAILED: " + err);
        QMetaObject::invokeMethod(this, [this, err]()
                                  {
                setStatus(QStringLiteral("Failed"));
                setBusy(false);
                emit failed(err); }, Qt::QueuedConnection);
    };

    auto updateUI = [this](int progress, const QString &status)
    {
        QMetaObject::invokeMethod(this, [this, progress, status]()
                                  {
                setProgress(progress);
                setStatus(status); }, Qt::QueuedConnection);
    };

    // ── Шаг 1: Открываем декодер ──────────────────────────────────────────────
    updateUI(0, QStringLiteral("Opening video..."));

    VideoDecoder decoder;
    m_decoder = &decoder;
    QString error;

    if (!decoder.open(settings.inputPath, error))
    {
        m_decoder = nullptr;
        Logger::instance()->error("[PipelineManager] Decoder failed to open: " + error);
        return fail(error);
    }

    VideoInfo info = decoder.info();

    QMetaObject::invokeMethod(this, [this, info]()
                              { setHwDecoder(info.hwDevice); }, Qt::QueuedConnection);

    qDebug() << "[PipelineManager] Video info:"
             << info.width << "x" << info.height
             << info.fps << "fps"
             << info.totalFrames << "frames";

    int scale = settings.scale;
    if (!settings.targetResolution.isEmpty())
        scale = upscaleScaleForTargetPreset(settings.targetResolution,
                                            info.width, info.height);
    scale = qBound(2, scale, 4);
    qDebug() << "[PipelineManager] Upscale scale:" << scale
             << "preset:" << settings.targetResolution;

    // ── Шаг 2: Запускаем Python апскейлер ────────────────────────────────────
    updateUI(2, QStringLiteral("Loading AI model..."));

    PythonUpscaler upscaler;
    m_upscaler = &upscaler;

    if (!upscaler.start(settings.pythonExe, settings.scriptPath,
                        settings.model, scale,
                        settings.device,
                        info.width, info.height,
                        error))
    {
        m_decoder = nullptr;
        m_upscaler = nullptr;
        decoder.close();
        Logger::instance()->error("[PipelineManager] Upscaler fail: " + error);
        return fail(error);
    }

    // ── Шаг 3: Открываем энкодер ──────────────────────────────────────────────
    updateUI(5, QStringLiteral("Initializing encoder..."));

    // Формируем путь к выходному файлу
    QFileInfo fi(settings.inputPath);
    QString outName = fi.baseName() + QStringLiteral("_")
                      + QString::number(info.width * scale) + QStringLiteral("p_")
                      + settings.model + QStringLiteral(".mp4");

    QString outDir = settings.outputDir;
    if (outDir.isEmpty())
        outDir = fi.absolutePath();
    QDir().mkpath(outDir);
    if (!QDir(outDir).exists())
    {
        return fail(QStringLiteral("Output directory does not exist: ") + outDir);
    }

    QString outputPath = outDir + QStringLiteral("/") + outName;

    EncodeSettings encSettings;
    encSettings.width = info.width * scale;
    encSettings.height = info.height * scale;
    encSettings.fps = info.fps;
    encSettings.quality = settings.quality;
    encSettings.copyAudio = settings.copyAudio;
    encSettings.audioSourcePath = settings.inputPath;

    VideoEncoder encoder;
    m_encoder = &encoder;

    if (!encoder.open(outputPath, encSettings, error))
    {
        m_decoder = nullptr;
        m_upscaler = nullptr;
        m_encoder = nullptr;
        upscaler.stop();
        decoder.close();
        return fail(error);
    }

    m_filter.init();
    qDebug() << "[PipelineManager] Filter backend:" << m_filter.backendName();

    QMetaObject::invokeMethod(this, [this, &encoder]()
                              { setHwEncoder(encoder.codecName()); }, Qt::QueuedConnection);

    // ── Шаг 4: Основной цикл декод → апскейл → кодирование ───────────────────
    updateUI(8, QStringLiteral("Processing..."));

    const int total = info.totalFrames > 0 ? info.totalFrames : 1;
    int processed = 0;
    qint64 startMs = QDateTime::currentMSecsSinceEpoch();
    bool pipeOk = true;

    if (m_perfMonitor)
        m_perfMonitor->reset();

    QElapsedTimer stageTimer;

    auto onFrame = [&](const uint8_t *rgb, int size, int frameIdx)
    {
        if (m_cancelRequested || !pipeOk)
            return;

        FrameTimings t;

        // Апскейл кадра через shared memory
        std::vector<uint8_t> outRGB;
        int outW = 0, outH = 0;
        QString frameError;

        // Upscale
        stageTimer.restart();
        if (!upscaler.processFrame(rgb, info.width, info.height,
                                   outRGB, outW, outH, frameError))
        {
            if (m_cancelRequested || frameError == QLatin1String("Cancelled"))
                return;
            qWarning() << "[PipelineManager] Frame" << frameIdx
                       << "upscale failed:" << frameError;
            pipeOk = false;
            return;
        }
        t.upscaleUs = stageTimer.nsecsElapsed() / 1000;

        // Filters
        stageTimer.restart();
        if (!settings.filters.isIdentity())
        {
            m_filter.apply(outRGB.data(), outW, outH, settings.filters);
        }
        t.filterUs = stageTimer.nsecsElapsed() / 1000;

        // Кодируем апскейленный кадр
        stageTimer.restart();
        if (!encoder.encodeFrame(outRGB.data(), outW, outH, frameError))
        {
            qWarning() << "[PipelineManager] Frame" << frameIdx
                       << "encode failed:" << frameError;
            pipeOk = false;
            return;
        }
        t.encodeUs = stageTimer.nsecsElapsed() / 1000;

        if (m_perfMonitor)
            m_perfMonitor->recordFrame(t);

        processed++;

        // Обновляем UI каждые 5 кадров чтобы не спамить сигналами
        if (processed % 5 == 0 || processed == 1)
        {
            int pct = 8 + static_cast<int>(processed * 90.0 / total);
            pct = qMin(pct, 98);

            // ETA берём из perfMonitor
            QString etaStr;
            if (m_perfMonitor && m_perfMonitor->currentFps() > 0.0f)
            {
                int remainSec = static_cast<int>(
                    (total - processed) / m_perfMonitor->currentFps());
                etaStr = QString("%1:%2")
                             .arg(remainSec / 60, 2, 10, QChar('0'))
                             .arg(remainSec % 60, 2, 10, QChar('0'));
            }

            QString statusStr = QString("Upscaling frame %1 / %2")
                                    .arg(processed)
                                    .arg(total);

            QMetaObject::invokeMethod(this, [this, pct, statusStr, etaStr]()
                                      {
                    setProgress(pct);
                    setStatus(statusStr);
                    setEta(etaStr); }, Qt::QueuedConnection);
        }
    };

    // Запускаем декодирование (блокирует поток до конца видео)
    decoder.decode(onFrame, error);

    m_decoder = nullptr;

    // ── Шаг 5: Финализация ────────────────────────────────────────────────────
    if (m_cancelRequested)
    {
        upscaler.stop();
        encoder.close();
        m_upscaler = nullptr;
        m_encoder = nullptr;

        QMetaObject::invokeMethod(this, [this]()
                                  {
                setStatus(QStringLiteral("Cancelled"));
                setProgress(0);
                setEta(QStringLiteral(""));
                setBusy(false);
                emit failed(QStringLiteral("Cancelled"));
            },
                                  Qt::QueuedConnection);
        return;
    }

    if (!pipeOk)
    {
        upscaler.stop();
        encoder.close();
        m_upscaler = nullptr;
        m_encoder = nullptr;
        return fail(QStringLiteral("Pipeline error during processing"));
    }

    updateUI(99, QStringLiteral("Finalizing..."));

    upscaler.stop();
    m_upscaler = nullptr;

    if (!encoder.finalize(error))
    {
        m_encoder = nullptr;
        return fail(error);
    }
    m_encoder = nullptr;

    qDebug() << "[PipelineManager] Done! Output:" << outputPath;
    Logger::instance()->info(QString("[Pipeline] Done! Output: %1").arg(outputPath));

    QMetaObject::invokeMethod(this, [this, outputPath]()
                              {
            setProgress(100);
            setStatus(QStringLiteral("Done"));
            setEta(QStringLiteral(""));
            setBusy(false);
            emit finished(outputPath); }, Qt::QueuedConnection);
}

// ─────────────────────────────────────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────────────────────────────────────

void PipelineManager::setBusy(bool v)
{
    if (m_busy == v)
        return;
    m_busy = v;
    emit busyChanged();
}

void PipelineManager::setProgress(int v)
{
    if (m_progress == v)
        return;
    m_progress = v;
    emit progressChanged();
}

void PipelineManager::setStatus(const QString &v)
{
    if (m_status == v)
        return;
    m_status = v;
    emit statusChanged();
}

void PipelineManager::setEta(const QString &v)
{
    if (m_eta == v)
        return;
    m_eta = v;
    emit etaChanged();
}

void PipelineManager::setHwDecoder(const QString &v)
{
    if (m_hwDecoder == v)
        return;
    m_hwDecoder = v;
    emit hwDecoderChanged();
}

void PipelineManager::setHwEncoder(const QString &v)
{
    if (m_hwEncoder == v)
        return;
    m_hwEncoder = v;
    emit hwEncoderChanged();
}
