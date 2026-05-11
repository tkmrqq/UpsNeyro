#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>

class SettingsManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString outputDir READ outputDir WRITE setOutputDir NOTIFY outputDirChanged)
    Q_PROPERTY(bool openFolderWhenFinished READ openFolderWhenFinished WRITE setOpenFolderWhenFinished NOTIFY
                   openFolderWhenFinishedChanged)
    Q_PROPERTY(bool playSoundOnComplete READ playSoundOnComplete WRITE setPlaySoundOnComplete NOTIFY
                   playSoundOnCompleteChanged)
    Q_PROPERTY(bool hardwareDecodePreview READ hardwareDecodePreview WRITE setHardwareDecodePreview NOTIFY
                   hardwareDecodePreviewChanged)
    Q_PROPERTY(bool logVerbose READ logVerbose WRITE setLogVerbose NOTIFY logVerboseChanged)
    Q_PROPERTY(int queueMaxRetries READ queueMaxRetries WRITE setQueueMaxRetries NOTIFY queueMaxRetriesChanged)
    Q_PROPERTY(QString updateManifestUrl READ updateManifestUrl WRITE setUpdateManifestUrl NOTIFY
                   updateManifestUrlChanged)
    Q_PROPERTY(int exportTotal READ exportTotal NOTIFY exportStatsChanged)
    Q_PROPERTY(int exportSucceeded READ exportSucceeded NOTIFY exportStatsChanged)
    Q_PROPERTY(int exportFailed READ exportFailed NOTIFY exportStatsChanged)
    Q_PROPERTY(int exportCancelled READ exportCancelled NOTIFY exportStatsChanged)

public:
    explicit SettingsManager(QObject *parent = nullptr);

    QString outputDir() const { return m_outputDir; }
    bool openFolderWhenFinished() const { return m_openFolderWhenFinished; }
    bool playSoundOnComplete() const { return m_playSoundOnComplete; }
    bool hardwareDecodePreview() const { return m_hardwareDecodePreview; }
    bool logVerbose() const { return m_logVerbose; }
    int queueMaxRetries() const { return m_queueMaxRetries; }
    QString updateManifestUrl() const { return m_updateManifestUrl; }
    int exportTotal() const { return m_exportTotal; }
    int exportSucceeded() const { return m_exportSucceeded; }
    int exportFailed() const { return m_exportFailed; }
    int exportCancelled() const { return m_exportCancelled; }

    Q_INVOKABLE void playCompletionSound() const;
    Q_INVOKABLE void revealOutputInFileManager(const QString &filePath) const;
    Q_INVOKABLE void openLogFileLocation() const;
    Q_INVOKABLE void recordExportResult(bool succeeded, bool cancelled);
    Q_INVOKABLE void resetExportStats();

public slots:
    void setOutputDir(const QString &dir);
    void setOpenFolderWhenFinished(bool val);
    void setPlaySoundOnComplete(bool val);
    void setHardwareDecodePreview(bool val);
    void setLogVerbose(bool val);
    void setQueueMaxRetries(int n);
    void setUpdateManifestUrl(const QString &url);

signals:
    void outputDirChanged();
    void openFolderWhenFinishedChanged();
    void playSoundOnCompleteChanged();
    void hardwareDecodePreviewChanged();
    void logVerboseChanged();
    void queueMaxRetriesChanged();
    void updateManifestUrlChanged();
    void exportStatsChanged();

private:
    QString m_outputDir;
    bool m_openFolderWhenFinished = true;
    bool m_playSoundOnComplete = false;
    bool m_hardwareDecodePreview = false;
    bool m_logVerbose = false;
    int m_queueMaxRetries = 2;
    QString m_updateManifestUrl;
    int m_exportTotal = 0;
    int m_exportSucceeded = 0;
    int m_exportFailed = 0;
    int m_exportCancelled = 0;
};

#endif // SETTINGSMANAGER_H
