#ifndef JOBQUEUE_H
#define JOBQUEUE_H

#include <QObject>
#include <QList>
#include "exportjob.h"
#include "upscalemanager.h"
#include "settingsmanager.h"

class JobQueue : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<QObject*> jobs    READ jobObjects NOTIFY jobsChanged)
    Q_PROPERTY(bool            running READ running    NOTIFY runningChanged)
    Q_PROPERTY(int             current READ current    NOTIFY currentChanged)
    // сколько осталось незавершённых
    Q_PROPERTY(int             pending READ pending    NOTIFY jobsChanged)

    Q_PROPERTY(UpscaleManager*  upscaleManager  READ upscaleManager  WRITE setUpscaleManager  NOTIFY upscaleManagerChanged)
    Q_PROPERTY(SettingsManager* settingsManager READ settingsManager WRITE setSettingsManager NOTIFY settingsManagerChanged)

public:
    explicit JobQueue(QObject *parent = nullptr);

    explicit JobQueue(UpscaleManager *um, SettingsManager *sm, QObject *parent = nullptr);

    UpscaleManager*  upscaleManager()  const { return m_upscale; }
    SettingsManager* settingsManager() const { return m_settings; }

    QList<QObject*> jobObjects() const;
    bool            running()    const { return m_running; }
    int             current()    const { return m_currentIndex; }
    int             pending()    const;

    // Добавить задание с текущими настройками upscaleManager
    Q_INVOKABLE void addJob(const QString &inputPath);
    Q_INVOKABLE void removeJob(int index);
    Q_INVOKABLE void moveUp(int index);
    Q_INVOKABLE void moveDown(int index);
    Q_INVOKABLE void clearFinished();

    Q_INVOKABLE void start();
    Q_INVOKABLE void cancelCurrent();
    Q_INVOKABLE void cancelAll();

    void setUpscaleManager(UpscaleManager *m);
    void setSettingsManager(SettingsManager *m);

signals:
    void jobsChanged();
    void runningChanged();
    void currentChanged();
    void allFinished();
    void jobFinished(int index, const QString &outputPath);
    void jobFailed(int index, const QString &error);

    void upscaleManagerChanged();
    void settingsManagerChanged();

private slots:
    void onUpscaleFinished(const QString &outputPath);
    void onUpscaleFailed(const QString &error);
    void onProgressChanged();
    void onStatusChanged();
    void onEtaChanged();

private:
    void processNext();
    void applyJobSettings(ExportJob *job);

    QList<ExportJob*> m_jobs;
    UpscaleManager   *m_upscale;
    SettingsManager  *m_settings;
    int               m_currentIndex = -1;
    bool              m_running      = false;
    bool              m_cancelAll    = false;
};

#endif // JOBQUEUE_H
