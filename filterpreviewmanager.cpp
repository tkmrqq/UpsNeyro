#include "filterpreviewmanager.h"
#include "filtermanager.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QDateTime>
#include <QFutureWatcher>
#include <QtConcurrent>

FilterPreviewManager::FilterPreviewManager(QObject *parent)
    : QObject(parent)
{
    m_initialized = m_frameFilter.init();
}

void FilterPreviewManager::generate(const QString &videoPath,
                                    double positionSec,
                                    FilterManager *fm)
{
    if (m_busy || videoPath.isEmpty() || !fm)
        return;

    setBusy(true);
    setStatus(tr("Захват кадра..."));

    const QString tempDir = QStandardPaths::writableLocation(
                                QStandardPaths::TempLocation) +
                            "/upsneyro_filter_preview";
    QDir().mkpath(tempDir);

    m_rawFramePath = tempDir + "/filter_preview_orig.png";
    m_filteredFramePath = tempDir + "/filter_preview_filtered.png";

    // Захватываем кадр асинхронно
    // Исправление: captureFrame принимает 4 параметра — добавляем errorOut
    QString captureError; // ← локальная переменная для передачи по ссылке не работает в лямбде
                          //   поэтому используем std::string или захватываем по значению

    // Лучший вариант — не использовать QtConcurrent с лямбдой и ссылкой,
    // а запустить через QThread или напрямую (захват быстрый, ~100ms)
    QString errorOut;
    const bool ok = m_frameCapture.captureFrame(videoPath, positionSec,
                                                m_rawFramePath, errorOut);
    if (!ok)
    {
        setBusy(false);
        setStatus(tr("Ошибка захвата: %1").arg(errorOut));
        emit previewFailed(errorOut);
        return;
    }

    qDebug() << "[Preview] Frame captured, starting upscale...";

    setStatus(tr("Применяю фильтры..."));
    applyFilters(fm);
}

void FilterPreviewManager::refresh(FilterManager *fm)
{
    if (m_busy || m_rawFramePath.isEmpty() || !fm)
        return;
    setBusy(true);
    setStatus(tr("Обновляю фильтры..."));
    applyFilters(fm);
}

void FilterPreviewManager::applyFilters(FilterManager *fm)
{
    QFile::remove(m_filteredFramePath);
    QFile::copy(m_rawFramePath, m_filteredFramePath);

    QImage img(m_filteredFramePath);
    if (img.isNull())
    {
        setBusy(false);
        setStatus(tr("Ошибка чтения кадра"));
        emit previewFailed(tr("Не удалось прочитать кадр"));
        return;
    }

    img = img.convertToFormat(QImage::Format_RGB888);
    FilterParams params = fm->currentParams();
    m_frameFilter.apply(img.bits(), img.width(), img.height(), params);

    if (!img.save(m_filteredFramePath))
    {
        setBusy(false);
        setStatus(tr("Ошибка сохранения"));
        emit previewFailed(tr("Не удалось сохранить превью"));
        return;
    }

    const QString ts = QString::number(QDateTime::currentMSecsSinceEpoch());
    m_originalUrl = QUrl::fromLocalFile(m_rawFramePath);
    m_filteredUrl = QUrl(QUrl::fromLocalFile(m_filteredFramePath).toString() + "?t=" + ts);

    // Захватываем готовые URL в локальные переменные для лямбды
    QUrl origUrl = m_originalUrl;
    QUrl filteredUrl = m_filteredUrl;

    setStatus(tr("Готово"));

    QMetaObject::invokeMethod(this, [this, origUrl, filteredUrl]()
                              {
            setBusy(false);
            emit previewReady(origUrl, filteredUrl); }, Qt::QueuedConnection);
}

void FilterPreviewManager::clear()
{
    m_rawFramePath.clear();
    m_filteredFramePath.clear();
    m_originalUrl.clear();
    m_filteredUrl.clear();
    setStatus({});
    emit previewReady(m_originalUrl, m_filteredUrl);
}

void FilterPreviewManager::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    emit busyChanged();
}

void FilterPreviewManager::setStatus(const QString &s)
{
    if (m_status == s)
        return;
    m_status = s;
    emit statusChanged();
}
