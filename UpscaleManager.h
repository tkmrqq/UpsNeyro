#ifndef UPSCALEMANAGER_H
#define UPSCALEMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QUrl>

class UpscaleManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(UpscaleMode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(bool denoise READ denoise WRITE setDenoise NOTIFY denoiseChanged)

    Q_PROPERTY(bool previewBusy READ previewBusy NOTIFY previewBusyChanged)
    Q_PROPERTY(QString previewStatus READ previewStatus NOTIFY previewStatusChanged)
    Q_PROPERTY(int previewProgress READ previewProgress NOTIFY previewProgressChanged)

public:
    enum UpscaleMode {
        FastMode = 0,
        BalancedMode,
        QualityMode
    };
    Q_ENUM(UpscaleMode)

    explicit UpscaleManager(QObject *parent = nullptr);

    UpscaleMode mode()        const { return m_mode; }
    QString     resolution()  const { return m_resolution; }
    bool        denoise()     const { return m_denoise; }
    bool        previewBusy() const { return m_previewBusy; }
    QString     previewStatus()   const { return m_previewStatus; }
    int         previewProgress() const { return m_previewProgress; }

    Q_INVOKABLE void startUpscaling(const QString &videoPath, const QString &outputDir);
    Q_INVOKABLE void startPreview(const QString &videoPath, double positionSec);

public slots:
    void setMode(UpscaleMode m);
    void setResolution(const QString &r);
    void setDenoise(bool d);

signals:
    void modeChanged();
    void resolutionChanged();
    void denoiseChanged();

    void previewBusyChanged();
    void previewStatusChanged();
    void previewProgressChanged();

    void previewReady(QString originalUrl, QString upscaledUrl);
    void previewFailed(QString error);

private:
    void setPreviewBusy(bool b);
    void setPreviewStatus(const QString &s);
    void setPreviewProgress(int p);
    void runUpscalerForPreview();

    // Настройки апскейла
    UpscaleMode m_mode       = BalancedMode;
    QString     m_resolution = QStringLiteral("4K");
    bool        m_denoise    = true;

    // Состояние превью
    bool    m_previewBusy     = false;
    QString m_previewStatus;
    int     m_previewProgress = 0;

    // Временные пути кадров
    QString m_framePath;
    QString m_upscaledFramePath;

    // Два отдельных процесса: захват кадра и апскейл
    QProcess m_ffmpegProc;
    QProcess m_upscaleProc;

    QString m_lastVideoPath;
    double  m_lastPositionSec = -1.0;
};

#endif // UPSCALEMANAGER_H