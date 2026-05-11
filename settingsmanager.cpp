#include "settingsmanager.h"
#include "logger.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QProcess>
#include <QUrl>
#include <cstdio>

#ifdef Q_OS_WIN
#  include <windows.h>
#endif

static void playSystemCompletionSound()
{
#ifdef Q_OS_WIN
    MessageBeep(MB_OK);
#else
    fprintf(stderr, "\a");
    fflush(stderr);
#endif
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    QSettings settings;

    QString defaultVideoPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    if (defaultVideoPath.isEmpty())
        defaultVideoPath = QDir::homePath();

    m_outputDir = settings.value(QStringLiteral("Global/outputDir"), defaultVideoPath).toString();
    m_openFolderWhenFinished = settings.value(QStringLiteral("Global/openFolderWhenFinished"), true).toBool();
    m_playSoundOnComplete = settings.value(QStringLiteral("Global/playSoundOnComplete"), false).toBool();
    m_hardwareDecodePreview = settings.value(QStringLiteral("Global/hardwareDecodePreview"), false).toBool();
    m_logVerbose = settings.value(QStringLiteral("Global/logVerbose"), false).toBool();
    m_queueMaxRetries = settings.value(QStringLiteral("Global/queueMaxRetries"), 2).toInt();
    m_queueMaxRetries = qBound(0, m_queueMaxRetries, 5);
    m_updateManifestUrl = settings.value(QStringLiteral("Global/updateManifestUrl")).toString();

    m_exportSucceeded = qMax(0, settings.value(QStringLiteral("Stats/exportSucceeded"), 0).toInt());
    m_exportFailed = qMax(0, settings.value(QStringLiteral("Stats/exportFailed"), 0).toInt());
    m_exportCancelled = qMax(0, settings.value(QStringLiteral("Stats/exportCancelled"), 0).toInt());
    m_exportTotal = m_exportSucceeded + m_exportFailed + m_exportCancelled;

    Logger::instance()->setMinimumLogLevel(m_logVerbose ? Logger::Debug : Logger::Info);
}

void SettingsManager::setOutputDir(const QString &dir)
{
    if (m_outputDir == dir)
        return;
    m_outputDir = dir;
    QSettings().setValue(QStringLiteral("Global/outputDir"), dir);
    emit outputDirChanged();
}

void SettingsManager::setOpenFolderWhenFinished(bool val)
{
    if (m_openFolderWhenFinished == val)
        return;
    m_openFolderWhenFinished = val;
    QSettings().setValue(QStringLiteral("Global/openFolderWhenFinished"), val);
    emit openFolderWhenFinishedChanged();
}

void SettingsManager::setPlaySoundOnComplete(bool val)
{
    if (m_playSoundOnComplete == val)
        return;
    m_playSoundOnComplete = val;
    QSettings().setValue(QStringLiteral("Global/playSoundOnComplete"), val);
    emit playSoundOnCompleteChanged();
}

void SettingsManager::setHardwareDecodePreview(bool val)
{
    if (m_hardwareDecodePreview == val)
        return;
    m_hardwareDecodePreview = val;
    QSettings().setValue(QStringLiteral("Global/hardwareDecodePreview"), val);
    emit hardwareDecodePreviewChanged();
}

void SettingsManager::setLogVerbose(bool val)
{
    if (m_logVerbose == val)
        return;
    m_logVerbose = val;
    QSettings().setValue(QStringLiteral("Global/logVerbose"), val);
    Logger::instance()->setMinimumLogLevel(val ? Logger::Debug : Logger::Info);
    emit logVerboseChanged();
}

void SettingsManager::setQueueMaxRetries(int n)
{
    n = qBound(0, n, 5);
    if (m_queueMaxRetries == n)
        return;
    m_queueMaxRetries = n;
    QSettings().setValue(QStringLiteral("Global/queueMaxRetries"), n);
    emit queueMaxRetriesChanged();
}

void SettingsManager::setUpdateManifestUrl(const QString &url)
{
    if (m_updateManifestUrl == url)
        return;
    m_updateManifestUrl = url;
    QSettings().setValue(QStringLiteral("Global/updateManifestUrl"), url);
    emit updateManifestUrlChanged();
}

void SettingsManager::recordExportResult(bool succeeded, bool cancelled)
{
    if (succeeded) {
        ++m_exportSucceeded;
        QSettings().setValue(QStringLiteral("Stats/exportSucceeded"), m_exportSucceeded);
    } else if (cancelled) {
        ++m_exportCancelled;
        QSettings().setValue(QStringLiteral("Stats/exportCancelled"), m_exportCancelled);
    } else {
        ++m_exportFailed;
        QSettings().setValue(QStringLiteral("Stats/exportFailed"), m_exportFailed);
    }
    m_exportTotal = m_exportSucceeded + m_exportFailed + m_exportCancelled;
    emit exportStatsChanged();
}

void SettingsManager::resetExportStats()
{
    m_exportTotal = 0;
    m_exportSucceeded = 0;
    m_exportFailed = 0;
    m_exportCancelled = 0;

    QSettings s;
    s.setValue(QStringLiteral("Stats/exportSucceeded"), 0);
    s.setValue(QStringLiteral("Stats/exportFailed"), 0);
    s.setValue(QStringLiteral("Stats/exportCancelled"), 0);
    emit exportStatsChanged();
}

void SettingsManager::playCompletionSound() const
{
    playSystemCompletionSound();
}

void SettingsManager::openLogFileLocation() const
{
    revealOutputInFileManager(Logger::instance()->logFilePath());
}

void SettingsManager::revealOutputInFileManager(const QString &filePath) const
{
    if (filePath.isEmpty())
        return;
    const QFileInfo fi(filePath);
    const QString dir = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
#ifdef Q_OS_WIN
    // Explorer: highlight file when possible
    if (fi.isFile()) {
        const QString native = QDir::toNativeSeparators(fi.absoluteFilePath());
        QProcess::startDetached(QStringLiteral("explorer.exe"),
                                {QStringLiteral("/select,"), native});
        return;
    }
#endif
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}
