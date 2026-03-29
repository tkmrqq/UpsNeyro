#include "pipelinemanager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QtConcurrent>

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
                                   int scale,
                                   int quality,
                                   const QString &device,
                                   const FilterParams &filters)
{
    PipelineSettings s;
    s.inputPath = inputPath;
    s.outputDir = outputDir;
    s.model = model;
    s.scale = scale;
    s.quality = quality;
    s.device = device;
    s.copyAudio = true;
    s.filters = filters;

#ifdef QT_NO_DEBUG
    // Release: пути относительно папки exe
    QString appDir = QCoreApplication::applicationDirPath();
    s.pythonExe = appDir + "/python/python.exe";
    s.scriptPath = appDir + "/ai_engine/upscaler.py";
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
        return;
    }

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
        return fail(error);
    }

    VideoInfo info = decoder.info();

    QMetaObject::invokeMethod(this, [this, info]()
                              { setHwDecoder(info.hwDevice); }, Qt::QueuedConnection);

    qDebug() << "[PipelineManager] Video info:"
             << info.width << "x" << info.height
             << info.fps << "fps"
             << info.totalFrames << "frames";

    // ── Шаг 2: Запускаем Python апскейлер ────────────────────────────────────
    updateUI(2, QStringLiteral("Loading AI model..."));

    PythonUpscaler upscaler;
    m_upscaler = &upscaler;

    if (!upscaler.start(settings.pythonExe, settings.scriptPath,
                        settings.model, settings.scale,
                        settings.device, error))
    {
        m_decoder = nullptr;
        m_upscaler = nullptr;
        decoder.close();
        return fail(error);
    }

    // ── Шаг 3: Открываем энкодер ──────────────────────────────────────────────
    updateUI(5, QStringLiteral("Initializing encoder..."));

    // Формируем путь к выходному файлу
    QFileInfo fi(settings.inputPath);
    QString outName = fi.baseName() + QStringLiteral("_") + QString::number(info.width * settings.scale) + QStringLiteral("p_") + settings.model + QStringLiteral(".mp4");

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
    encSettings.width = info.width * settings.scale;
    encSettings.height = info.height * settings.scale;
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
                setBusy(false); }, Qt::QueuedConnection);
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
