#ifndef UPSCALEMANAGER_H
#define UPSCALEMANAGER_H

#include <QObject>
#include <QString>
#include <QDebug>

class UpscaleManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(UpscaleMode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(bool denoise READ denoise WRITE setDenoise NOTIFY denoiseChanged)

public:
    enum UpscaleMode {
        FastMode = 0,
        BalancedMode,
        QualityMode
    };
    Q_ENUM(UpscaleMode)

    explicit UpscaleManager(QObject *parent = nullptr) : QObject(parent) {}

    UpscaleMode mode() const { return m_mode; }
    QString resolution() const { return m_resolution; }
    bool denoise() const { return m_denoise; }

    //"Start Upscaling" вызывает этот метод
    Q_INVOKABLE void startUpscaling(const QString& videoPath, const QString& outputDir) {
        qDebug() << "Starting upscale task!";
        qDebug() << "Video:" << videoPath;
        qDebug() << "Output:" << outputDir;
        qDebug() << "Target Res:" << m_resolution;
        qDebug() << "AI Mode:" << m_mode;
        qDebug() << "Denoise:" << m_denoise;

        // Здесь invoke/вызов Python/FFmpeg скрипт через QProcess
    }

public slots:
    void setMode(UpscaleMode m) { if (m_mode == m) return; m_mode = m; emit modeChanged(); }
    void setResolution(const QString& r) { if (m_resolution == r) return; m_resolution = r; emit resolutionChanged(); }
    void setDenoise(bool d) { if (m_denoise == d) return; m_denoise = d; emit denoiseChanged(); }

signals:
    void modeChanged();
    void resolutionChanged();
    void denoiseChanged();

private:
    UpscaleMode m_mode = BalancedMode;
    QString m_resolution = "4K";
    bool m_denoise = true;
};

#endif // UPSCALEMANAGER_H
