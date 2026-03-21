#pragma once

#include <QObject>
#include <QProcess>
#include <QSharedMemory>
#include <QString>
#include <QMutex>
#include <QWaitCondition>

// ─────────────────────────────────────────────────────────────────────────────
// Протокол shared memory
//
// Структура в начале блока памяти (header):
//
//  [0]  uint8_t  state
//         0 = IDLE      — никто не работает
//         1 = C++_READY — C++ записал кадр, ждёт Python
//         2 = PY_READY  — Python записал результат, ждёт C++
//         3 = STOP      — завершение работы
//
//  [1..4]  uint32_t  srcWidth
//  [5..8]  uint32_t  srcHeight
//  [9..12] uint32_t  dstWidth   (= srcWidth  * scale)
//  [13..16] uint32_t dstHeight  (= srcHeight * scale)
//
//  [HEADER_SIZE .. HEADER_SIZE + srcW*srcH*3]        — входной RGB кадр
//  [HEADER_SIZE + srcW*srcH*3 .. ]                   — выходной RGB кадр
//
// ─────────────────────────────────────────────────────────────────────────────

constexpr int SHM_HEADER_SIZE = 64;  // с запасом

enum class ShmState : uint8_t {
    Idle     = 0,
    CppReady = 1,
    PyReady  = 2,
    Stop     = 3
};

class PythonUpscaler : public QObject
{
    Q_OBJECT

public:
    explicit PythonUpscaler(QObject *parent = nullptr);
    ~PythonUpscaler();

    // Запустить Python процесс и создать shared memory
    // pythonExe  — путь к python/python3
    // scriptPath — путь к upscaler.py
    // model      — имя модели (realesrgan-x4plus и т.д.)
    // scale      — коэффициент увеличения (2 или 4)
    // device     — "cuda" / "cpu" / "auto"
    bool start(const QString &pythonExe,
               const QString &scriptPath,
               const QString &model,
               int scale,
               const QString &device,
               QString &errorOut);

    // Остановить Python процесс
    void stop();

    bool isRunning() const;

    // Отправить кадр на апскейл и получить результат (синхронно, блокирует поток)
    // inRGB  — входной  буфер RGB24 (w * h * 3)
    // outRGB — выходной буфер RGB24 (w*scale * h*scale * 3), аллоцируется внутри
    // Возвращает false при ошибке
    bool processFrame(const uint8_t *inRGB, int inW, int inH,
                      std::vector<uint8_t> &outRGB,
                      int &outW, int &outH,
                      QString &errorOut);

signals:
    void errorOccurred(QString error);

private:
    void writeHeader(int srcW, int srcH, int dstW, int dstH);
    void setState(ShmState state);
    ShmState getState() const;
    void waitForState(ShmState expected, int timeoutMs = 30000);

    QSharedMemory m_shm;
    QProcess      m_pyProcess;
    int           m_scale  = 4;
    int           m_srcW   = 0;
    int           m_srcH   = 0;
    bool          m_running = false;
};
