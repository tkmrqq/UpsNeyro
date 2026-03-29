#ifndef EXPORTJOB_H
#define EXPORTJOB_H

#include <QObject>
#include <QString>
#include "FilterManager.h"

class ExportJob : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString inputPath  READ inputPath  CONSTANT)
    Q_PROPERTY(QString outputDir  READ outputDir  CONSTANT)
    Q_PROPERTY(JobStatus status   READ status     NOTIFY statusChanged)
    Q_PROPERTY(int      progress  READ progress   NOTIFY progressChanged)
    Q_PROPERTY(QString  statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString  eta       READ eta        NOTIFY etaChanged)
    Q_PROPERTY(QString  outputPath READ outputPath NOTIFY outputPathChanged)

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
                       QObject *parent = nullptr)
        : QObject(parent)
        , m_inputPath(inputPath)
        , m_outputDir(outputDir)
    {}

    QString   inputPath()  const { return m_inputPath;  }
    QString   outputDir()  const { return m_outputDir;  }
    JobStatus status()     const { return m_status;     }
    int       progress()   const { return m_progress;   }
    QString   statusText() const { return m_statusText; }
    QString   eta()        const { return m_eta;        }
    QString   outputPath() const { return m_outputPath; }

    // Снимок настроек на момент постановки в очередь
    QString   resolution;
    int       upscaleMode  = 1;   // UpscaleManager::UpscaleMode
    bool      denoise      = true;
    int       outputQuality = 80;
    FilterParams filterParams;    // снимок FilterManager::currentParams()

    void setStatus(JobStatus s)      { if (m_status == s) return;     m_status = s;     emit statusChanged();     }
    void setProgress(int p)          { if (m_progress == p) return;   m_progress = p;   emit progressChanged();   }
    void setStatusText(const QString &t){ if (m_statusText == t) return; m_statusText = t; emit statusTextChanged(); }
    void setEta(const QString &e)    { if (m_eta == e) return;        m_eta = e;        emit etaChanged();        }
    void setOutputPath(const QString &p){ if (m_outputPath == p) return; m_outputPath = p; emit outputPathChanged(); }

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
