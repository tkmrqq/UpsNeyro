#include "pythonupscaler.h"

#include <QDebug>
#include <QThread>
#include <QProcessEnvironment>
#include <cstring>
#include <vector>

PythonUpscaler::PythonUpscaler(QObject *parent)
    : QObject(parent)
    , m_shm(QStringLiteral("upsneyro_shm"))
{
    // stderr всегда выводим в дебаг
    connect(&m_pyProcess, &QProcess::readyReadStandardError, this, [this]() {
        qDebug() << "[Python stderr]"
                 << m_pyProcess.readAllStandardError().trimmed();
    });
    // stdout НЕ подключаем через сигнал — читаем вручную в start()
}

PythonUpscaler::~PythonUpscaler()
{
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// start()
// ─────────────────────────────────────────────────────────────────────────────

bool PythonUpscaler::start(const QString &pythonExe,
                           const QString &scriptPath,
                           const QString &model,
                           int scale,
                           const QString &device,
                           QString &errorOut)
{
    if (m_running) stop();

    m_scale = scale;

    const int maxSrcW = 1920;
    const int maxSrcH = 1080;
    const int maxDstW = maxSrcW * scale;
    const int maxDstH = maxSrcH * scale;
    const int shmSize = SHM_HEADER_SIZE
                        + maxSrcW * maxSrcH * 3
                        + maxDstW * maxDstH * 3;

    // Удаляем старый блок если есть
    if (m_shm.isAttached()) m_shm.detach();
    {
        QSharedMemory old(m_shm.key());
        if (old.attach()) old.detach();
    }

    if (!m_shm.create(shmSize)) {
        if (m_shm.error() == QSharedMemory::AlreadyExists) {
            if (!m_shm.attach()) {
                errorOut = "Cannot attach shared memory: " + m_shm.errorString();
                return false;
            }
        } else {
            errorOut = "Cannot create shared memory: " + m_shm.errorString();
            return false;
        }
    }

    setState(ShmState::Idle);

    // Принудительно unbuffered stdout для Python
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_pyProcess.setProcessEnvironment(env);

    // -u = unbuffered stdout/stderr
    QStringList args = {
        QStringLiteral("-u"),
        scriptPath,
        QStringLiteral("--shm-key"),  m_shm.nativeKey(),
        QStringLiteral("--shm-size"), QString::number(shmSize),
        QStringLiteral("--model"),    model,
        QStringLiteral("--scale"),    QString::number(scale),
        QStringLiteral("--device"),   device
    };

    qDebug() << "[PythonUpscaler] Starting:" << pythonExe << args;

    // Читаем stdout вручную — НЕ через сигнал
    m_pyProcess.setReadChannel(QProcess::StandardOutput);
    m_pyProcess.start(pythonExe, args);

    if (!m_pyProcess.waitForStarted(5000)) {
        errorOut = "Cannot start Python: " + m_pyProcess.errorString();
        m_shm.detach();
        return false;
    }

    // Ждём "READY" через waitForReadyRead — надёжнее чем polling
    qDebug() << "[PythonUpscaler] Waiting for READY...";
    bool ready = false;
    QString accumulated;

    // Ждём до 120 секунд (загрузка модели может занять время)
    for (int i = 0; i < 120; ++i) {
        if (!m_pyProcess.waitForReadyRead(1000)) {
            // Таймаут 1 сек — проверяем жив ли процесс
            if (m_pyProcess.state() != QProcess::Running) {
                errorOut = "Python process died during startup";
                m_shm.detach();
                return false;
            }
            continue;
        }

        accumulated += QString::fromUtf8(
            m_pyProcess.readAllStandardOutput());
        qDebug() << "[PythonUpscaler] stdout:" << accumulated.trimmed();

        if (accumulated.contains(QStringLiteral("READY"))) {
            ready = true;
            break;
        }
    }

    if (!ready) {
        errorOut = "Python did not send READY (timeout)";
        m_pyProcess.kill();
        m_shm.detach();
        return false;
    }

    // После READY подключаем stdout к дебагу для логов прогресса
    connect(&m_pyProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        qDebug() << "[Python]" << m_pyProcess.readAllStandardOutput().trimmed();
    });

    m_running = true;
    qDebug() << "[PythonUpscaler] Python ready!";
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// processFrame()
// ─────────────────────────────────────────────────────────────────────────────

bool PythonUpscaler::processFrame(const uint8_t *inRGB, int inW, int inH,
                                  std::vector<uint8_t> &outRGB,
                                  int &outW, int &outH,
                                  QString &errorOut)
{
    if (!m_running || !m_shm.isAttached()) {
        errorOut = "Upscaler not running";
        return false;
    }

    outW = inW * m_scale;
    outH = inH * m_scale;

    const int inSize  = inW * inH * 3;
    const int outSize = outW * outH * 3;

    if (SHM_HEADER_SIZE + inSize + outSize > m_shm.size()) {
        errorOut = QString("SHM too small: need %1, have %2")
                       .arg(SHM_HEADER_SIZE + inSize + outSize)
                       .arg(m_shm.size());
        return false;
    }

    uint8_t *shmPtr = static_cast<uint8_t*>(m_shm.data());

    writeHeader(inW, inH, outW, outH);
    std::memcpy(shmPtr + SHM_HEADER_SIZE, inRGB, inSize);
    setState(ShmState::CppReady);

    // Ждём PyReady
    const int timeoutMs = 60000;
    const int stepMs    = 5;
    int elapsed = 0;

    while (elapsed < timeoutMs) {
        QThread::msleep(stepMs);
        elapsed += stepMs;

        if (m_pyProcess.state() != QProcess::Running) {
            errorOut = "Python process died during frame processing";
            m_running = false;
            return false;
        }

        if (getState() == ShmState::PyReady) break;
    }

    if (getState() != ShmState::PyReady) {
        errorOut = "Timeout waiting for Python";
        return false;
    }

    outRGB.resize(outSize);
    std::memcpy(outRGB.data(), shmPtr + SHM_HEADER_SIZE + inSize, outSize);
    setState(ShmState::Idle);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// stop()
// ─────────────────────────────────────────────────────────────────────────────

void PythonUpscaler::stop()
{
    if (!m_running) return;

    qDebug() << "[PythonUpscaler] Stopping...";
    setState(ShmState::Stop);

    m_pyProcess.waitForFinished(3000);
    if (m_pyProcess.state() != QProcess::NotRunning)
        m_pyProcess.kill();

    if (m_shm.isAttached())
        m_shm.detach();

    m_running = false;
}

bool PythonUpscaler::isRunning() const
{
    return m_running && m_pyProcess.state() == QProcess::Running;
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

void PythonUpscaler::writeHeader(int srcW, int srcH, int dstW, int dstH)
{
    uint8_t *p = static_cast<uint8_t*>(m_shm.data());
    auto writeU32 = [&](int offset, uint32_t val) {
        std::memcpy(p + offset, &val, 4);
    };
    writeU32(1,  static_cast<uint32_t>(srcW));
    writeU32(5,  static_cast<uint32_t>(srcH));
    writeU32(9,  static_cast<uint32_t>(dstW));
    writeU32(13, static_cast<uint32_t>(dstH));
}

void PythonUpscaler::setState(ShmState state)
{
    uint8_t *p = static_cast<uint8_t*>(m_shm.data());
    p[0] = static_cast<uint8_t>(state);
}

ShmState PythonUpscaler::getState() const
{
    const uint8_t *p = static_cast<const uint8_t*>(m_shm.constData());
    return static_cast<ShmState>(p[0]);
}
