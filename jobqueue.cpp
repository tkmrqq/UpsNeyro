#include "jobqueue.h"
#include <QFileInfo>

JobQueue::JobQueue(UpscaleManager *upscaleManager,
                   SettingsManager *settingsManager,
                   QObject *parent)
    : QObject(parent)
    , m_upscale(upscaleManager)
    , m_settings(settingsManager)
{
    connect(m_upscale, &UpscaleManager::upscaleFinished, this, &JobQueue::onUpscaleFinished);
    connect(m_upscale, &UpscaleManager::upscaleFailed,   this, &JobQueue::onUpscaleFailed);

    // Пробрасываем прогресс в текущее задание
    connect(m_upscale, &UpscaleManager::upscaleProgressChanged, this, &JobQueue::onProgressChanged);
    connect(m_upscale, &UpscaleManager::upscaleStatusChanged,   this, &JobQueue::onStatusChanged);
    connect(m_upscale, &UpscaleManager::upscaleEtaChanged,      this, &JobQueue::onEtaChanged);
}

JobQueue::JobQueue(QObject *parent)
    : QObject(parent)
    , m_upscale(nullptr)
    , m_settings(nullptr)
{
    // пустой — коннекты делаются в setUpscaleManager()
}

QList<QObject*> JobQueue::jobObjects() const
{
    QList<QObject*> result;
    for (auto *j : m_jobs) result.append(j);
    return result;
}

int JobQueue::pending() const
{
    int n = 0;
    for (auto *j : m_jobs)
        if (j->status() == ExportJob::Queued || j->status() == ExportJob::Running)
            ++n;
    return n;
}

void JobQueue::addJob(const QString &inputPath)
{
    auto *job = new ExportJob(inputPath, m_settings->outputDir(), this);

    // Снимаем текущие настройки upscaleManager
    job->resolution    = m_upscale->resolution();
    job->upscaleMode   = static_cast<int>(m_upscale->mode());
    job->denoise       = m_upscale->denoise();
    job->outputQuality = m_upscale->outputQuality();
    job->filterParams  = m_upscale->filters()->currentParams();

    m_jobs.append(job);
    emit jobsChanged();
}

void JobQueue::removeJob(int index)
{
    if (index < 0 || index >= m_jobs.size()) return;
    if (index == m_currentIndex && m_running) return; // нельзя удалить текущее

    m_jobs.takeAt(index)->deleteLater();

    // Корректируем m_currentIndex
    if (index < m_currentIndex)
        --m_currentIndex;

    emit jobsChanged();
}

void JobQueue::moveUp(int index)
{
    if (index <= 0 || index >= m_jobs.size()) return;
    if (index == m_currentIndex || index - 1 == m_currentIndex) return;
    m_jobs.swapItemsAt(index, index - 1);
    emit jobsChanged();
}

void JobQueue::moveDown(int index)
{
    if (index < 0 || index >= m_jobs.size() - 1) return;
    if (index == m_currentIndex || index + 1 == m_currentIndex) return;
    m_jobs.swapItemsAt(index, index + 1);
    emit jobsChanged();
}

void JobQueue::clearFinished()
{
    QList<ExportJob*> toRemove;
    m_jobs.erase(
        std::remove_if(m_jobs.begin(), m_jobs.end(), [&toRemove](ExportJob *j) {
            bool done = j->status() == ExportJob::Done ||
                        j->status() == ExportJob::Failed ||
                        j->status() == ExportJob::Cancelled;
            if (done) toRemove.append(j);
            return done;
        }),
        m_jobs.end()
        );
    for (auto *j : toRemove) j->deleteLater();
    emit jobsChanged();
}

void JobQueue::start()
{
    if (m_running) return;
    m_cancelAll = false;
    m_running   = true;
    emit runningChanged();
    processNext();
}

void JobQueue::cancelCurrent()
{
    if (!m_running || m_currentIndex < 0) return;
    m_upscale->cancelUpscaling();
    // Результат придёт в onUpscaleFailed → пометим как Cancelled и идём дальше
}

void JobQueue::cancelAll()
{
    m_cancelAll = true;
    // Пометить все Queued как Cancelled сразу
    for (auto *j : m_jobs) {
        if (j->status() == ExportJob::Queued)
            j->setStatus(ExportJob::Cancelled);
    }
    cancelCurrent();
    emit jobsChanged();
}

// ── private ──────────────────────────────────────────────────────────────

void JobQueue::processNext()
{
    // Найти следующий Queued
    m_currentIndex = -1;
    for (int i = 0; i < m_jobs.size(); ++i) {
        if (m_jobs[i]->status() == ExportJob::Queued) {
            m_currentIndex = i;
            break;
        }
    }

    if (m_currentIndex < 0 || m_cancelAll) {
        m_running      = false;
        m_currentIndex = -1;
        emit runningChanged();
        emit currentChanged();
        emit allFinished();
        return;
    }

    emit currentChanged();

    ExportJob *job = m_jobs[m_currentIndex];
    job->setStatus(ExportJob::Running);
    job->setProgress(0);
    applyJobSettings(job);

    m_upscale->startUpscaling(job->inputPath(), job->outputDir());
}

void JobQueue::applyJobSettings(ExportJob *job)
{
    m_upscale->setMode(static_cast<UpscaleManager::UpscaleMode>(job->upscaleMode));
    m_upscale->setResolution(job->resolution);
    m_upscale->setDenoise(job->denoise);
    m_upscale->setOutputQuality(job->outputQuality);
    m_upscale->filters()->setParams(job->filterParams);
}

void JobQueue::onUpscaleFinished(const QString &outputPath)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_jobs.size()) return;

    ExportJob *job = m_jobs[m_currentIndex];
    job->setOutputPath(outputPath);
    job->setStatus(ExportJob::Done);
    job->setProgress(100);

    emit jobFinished(m_currentIndex, outputPath);
    emit jobsChanged();

    processNext();
}

void JobQueue::onUpscaleFailed(const QString &error)
{
    if (m_currentIndex < 0 || m_currentIndex >= m_jobs.size()) return;

    ExportJob *job = m_jobs[m_currentIndex];
    job->setStatus(m_cancelAll ? ExportJob::Cancelled : ExportJob::Failed);
    job->setStatusText(error);

    emit jobFailed(m_currentIndex, error);
    emit jobsChanged();

    processNext();
}

void JobQueue::onProgressChanged()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_jobs.size()) return;
    m_jobs[m_currentIndex]->setProgress(m_upscale->upscaleProgress());
}

void JobQueue::onStatusChanged()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_jobs.size()) return;
    m_jobs[m_currentIndex]->setStatusText(m_upscale->upscaleStatus());
}

void JobQueue::onEtaChanged()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_jobs.size()) return;
    m_jobs[m_currentIndex]->setEta(m_upscale->upscaleEta());
}

void JobQueue::setUpscaleManager(UpscaleManager *m)
{
    if (m_upscale == m) return;

    // Отключить старые коннекты если был предыдущий менеджер
    if (m_upscale) {
        disconnect(m_upscale, nullptr, this, nullptr);
    }

    m_upscale = m;

    if (m_upscale) {
        connect(m_upscale, &UpscaleManager::upscaleFinished,       this, &JobQueue::onUpscaleFinished);
        connect(m_upscale, &UpscaleManager::upscaleFailed,         this, &JobQueue::onUpscaleFailed);
        connect(m_upscale, &UpscaleManager::upscaleProgressChanged,this, &JobQueue::onProgressChanged);
        connect(m_upscale, &UpscaleManager::upscaleStatusChanged,  this, &JobQueue::onStatusChanged);
        connect(m_upscale, &UpscaleManager::upscaleEtaChanged,     this, &JobQueue::onEtaChanged);
    }

    emit upscaleManagerChanged();
}

void JobQueue::setSettingsManager(SettingsManager *m)
{
    if (m_settings == m) return;
    m_settings = m;
    emit settingsManagerChanged();
}
