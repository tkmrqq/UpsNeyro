#include "exportjob.h"

ExportJob::ExportJob(const QString &inputPath, const QString &outputDir, QObject *parent)
    : QObject(parent)
    , m_inputPath(inputPath)
    , m_outputDir(outputDir)
{
}

QString ExportJob::inputPath() const { return m_inputPath; }
QString ExportJob::outputDir() const { return m_outputDir; }
ExportJob::JobStatus ExportJob::status() const { return m_status; }
int ExportJob::progress() const { return m_progress; }
QString ExportJob::statusText() const { return m_statusText; }
QString ExportJob::eta() const { return m_eta; }
QString ExportJob::outputPath() const { return m_outputPath; }

void ExportJob::setStatus(ExportJob::JobStatus s)
{
    if (m_status == s)
        return;
    m_status = s;
    emit statusChanged();
}

void ExportJob::setProgress(int p)
{
    if (m_progress == p)
        return;
    m_progress = p;
    emit progressChanged();
}

void ExportJob::setStatusText(const QString &t)
{
    if (m_statusText == t)
        return;
    m_statusText = t;
    emit statusTextChanged();
}

void ExportJob::setEta(const QString &e)
{
    if (m_eta == e)
        return;
    m_eta = e;
    emit etaChanged();
}

void ExportJob::setOutputPath(const QString &p)
{
    if (m_outputPath == p)
        return;
    m_outputPath = p;
    emit outputPathChanged();
}
