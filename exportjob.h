#ifndef EXPORTJOB_H
#define EXPORTJOB_H

#include <QObject>
#include <QString>
#include "filtermanager.h"

class ExportJob : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString inputPath READ inputPath CONSTANT)
    Q_PROPERTY(QString outputDir READ outputDir CONSTANT)
    Q_PROPERTY(JobStatus status READ status NOTIFY statusChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString eta READ eta NOTIFY etaChanged)
    Q_PROPERTY(QString outputPath READ outputPath NOTIFY outputPathChanged)

public:
    enum JobStatus {
        Queued    = 0,
        Running   = 1,
        Done      = 2,
        Failed    = 3,
        Cancelled = 4
    };
    Q_ENUM(JobStatus)

    explicit ExportJob(const QString &inputPath,
                       const QString &outputDir,
                       QObject *parent = nullptr);

    QString   inputPath()  const;
    QString   outputDir()  const;
    JobStatus status()     const;
    int       progress()   const;
    QString   statusText() const;
    QString   eta()        const;
    QString   outputPath() const;

    QString   resolution;
    int       upscaleMode  = 1;
    bool      denoise      = true;
    int       outputQuality = 80;
    FilterParams filterParams;
    /** How many times this job was re-queued after a failed run (not cancel). */
    int       retryRound = 0;

    void setStatus(JobStatus s);
    void setProgress(int p);
    void setStatusText(const QString &t);
    void setEta(const QString &e);
    void setOutputPath(const QString &p);

signals:
    void statusChanged();
    void progressChanged();
    void statusTextChanged();
    void etaChanged();
    void outputPathChanged();

private:
    QString   m_inputPath;
    QString   m_outputDir;
    JobStatus m_status    = Queued;
    int       m_progress  = 0;
    QString   m_statusText;
    QString   m_eta;
    QString   m_outputPath;
};

#endif // EXPORTJOB_H
