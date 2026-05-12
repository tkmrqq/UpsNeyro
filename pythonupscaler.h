#pragma once

#include <QObject>
#include <QProcess>
#include <QSharedMemory>
#include <QString>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Протокол shared memory
//
//  [0]     uint8_t state (Idle / CppReady / PyReady / Stop)
//  [1..16] uint32_t srcW, srcH, dstW, dstH
//  [HEADER_SIZE ..] input RGB24, then output RGB24
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

    bool start(const QString &pythonExe,
               const QString &scriptPath,
               const QString &model,
               int scale,
               const QString &device,
               int maxSrcW,
               int maxSrcH,
               QString &errorOut);

    void stop();

    bool isRunning() const;

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

    QSharedMemory m_shm;
    QProcess      m_pyProcess;
    int           m_scale  = 4;
    bool          m_running = false;
};
