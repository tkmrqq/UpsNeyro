#ifndef UPSCALEMANAGER_H
#define UPSCALEMANAGER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QUrl>
#include <QtConcurrent>
#include "framecapture.h"
#include "pipelinemanager.h"

class UpscaleManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(UpscaleMode mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(QString resolution READ resolution WRITE setResolution NOTIFY resolutionChanged)
    Q_PROPERTY(bool denoise READ denoise WRITE setDenoise NOTIFY denoiseChanged)
    Q_PROPERTY(int outputQuality READ outputQuality WRITE setOutputQuality NOTIFY outputQualityChanged)

    //curr model
    Q_PROPERTY(QString modelName READ modelName NOTIFY modeChanged)

    //preview state
    Q_PROPERTY(bool previewBusy READ previewBusy NOTIFY previewBusyChanged)
    Q_PROPERTY(QString previewStatus READ previewStatus NOTIFY previewStatusChanged)
    Q_PROPERTY(int previewProgress READ previewProgress NOTIFY previewProgressChanged)

    //upscale state
    Q_PROPERTY(bool upscaleBusy READ upscaleBusy NOTIFY upscaleBusyChanged)
    Q_PROPERTY(QString upscaleStatus READ upscaleStatus NOTIFY upscaleStatusChanged)
    Q_PROPERTY(int upscaleProgress READ upscaleProgress NOTIFY upscaleProgressChanged)
    Q_PROPERTY(QString upscaleEta READ upscaleEta NOTIFY upscaleEtaChanged)

    Q_PROPERTY(QString hwDecoder      READ hwDecoder       NOTIFY hwDecoderChanged)
    Q_PROPERTY(QString hwEncoder      READ hwEncoder       NOTIFY hwEncoderChanged)

public:
    enum UpscaleMode {
        FastMode = 0, //realesr-animevideov3
        BalancedMode = 1, //realesrgan-x4plus
        QualityMode = 2 //realesrgan-x4plus-anime
    };
    Q_ENUM(UpscaleMode)

    explicit UpscaleManager(QObject *parent = nullptr);

    //settings
    UpscaleMode mode()        const { return m_mode; }
    QString     resolution()  const { return m_resolution; }
    bool        denoise()     const { return m_denoise; }
    int         outputQuality() const { return m_outputQuality; }
    QString     modelName()     const { return modelNameForMode(m_mode); }
    //preview
    bool        previewBusy() const { return m_previewBusy; }
    QString     previewStatus()   const { return m_previewStatus; }
    int         previewProgress() const { return m_previewProgress; }
    //upscale
    bool    upscaleBusy()     const { return m_pipeline.busy(); }
    int     upscaleProgress() const { return m_pipeline.progress(); }
    QString upscaleStatus()   const { return m_pipeline.status(); }
    QString upscaleEta()      const { return m_pipeline.eta(); }
    QString hwDecoder()       const { return m_pipeline.hwDecoder(); }
    QString hwEncoder()       const { return m_pipeline.hwEncoder(); }

    Q_INVOKABLE void startUpscaling(const QString &videoPath, const QString &outputDir);
    Q_INVOKABLE void cancelUpscaling();
    Q_INVOKABLE void startPreview(const QString &videoPath, double positionSec);

public slots:
    void setMode(UpscaleMode m);
    void setResolution(const QString &r);
    void setDenoise(bool d);
    void setOutputQuality(int q);

signals:
    void modeChanged();
    void resolutionChanged();
    void denoiseChanged();
    void outputQualityChanged();
    //preview
    void previewBusyChanged();
    void previewStatusChanged();
    void previewProgressChanged();
    void previewReady(QString originalUrl, QString upscaledUrl);
    void previewFailed(QString error);
    //upscale
    void upscaleBusyChanged();
    void upscaleStatusChanged();
    void upscaleProgressChanged();
    void upscaleEtaChanged();
    void upscaleFinished(QString outputPath);
    void upscaleFailed(QString error);

    void hwDecoderChanged();
    void hwEncoderChanged();

private:
    void setPreviewBusy(bool b);
    void setPreviewStatus(const QString &s);
    void setPreviewProgress(int p);
    void runUpscalerForPreview();

    QString modelNameForMode(UpscaleMode mode) const;
    QString upscalerBinaryPath() const;
    QString cleanVideoPath(const QString &videoPath) const;
    int     scaleForResolution() const;

    // Настройки апскейла
    UpscaleMode m_mode       = BalancedMode;
    QString     m_resolution = QStringLiteral("4K");
    bool        m_denoise    = true;
    int         m_outputQuality = 80;   // CRF-подобный 0-100

    // Состояние превью
    bool    m_previewBusy     = false;
    QString m_previewStatus;
    int     m_previewProgress = 0;
    QString m_framePath;
    QString m_upscaledFramePath;
    QString m_lastVideoPath;
    double  m_lastPositionSec = -1.0;

    // Процессы
    QProcess m_upscaleProc;      // realesrgan (превью)

    FrameCapture m_frameCapture;
    PipelineManager m_pipeline;

};

#endif // UPSCALEMANAGER_H
