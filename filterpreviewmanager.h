#ifndef FILTERPREVIEWMANAGER_H
#define FILTERPREVIEWMANAGER_H

#pragma once
#include <QObject>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include "framecapture.h"
#include "framefilter.h"

class FilterManager;

class FilterPreviewManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QUrl originalUrl READ originalUrl NOTIFY previewReady)
    Q_PROPERTY(QUrl filteredUrl READ filteredUrl NOTIFY previewReady)

public:
    explicit FilterPreviewManager(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    QUrl originalUrl() const { return m_originalUrl; }
    QUrl filteredUrl() const { return m_filteredUrl; }

    // Запустить превью: захватить кадр + применить текущие фильтры
    Q_INVOKABLE void generate(const QString &videoPath,
                              double positionSec,
                              FilterManager *fm);

    // Обновить только фильтры на уже захваченном кадре (без повторного захвата)
    Q_INVOKABLE void refresh(FilterManager *fm);

    Q_INVOKABLE void clear();

signals:
    void busyChanged();
    void statusChanged();
    void previewReady(QUrl originalUrl, QUrl filteredUrl);
    void previewFailed(QString error);

private:
    void applyFilters(FilterManager *fm);
    void setBusy(bool b);
    void setStatus(const QString &s);

    FrameCapture m_frameCapture;
    FrameFilter m_frameFilter;

    QString m_rawFramePath;      // захваченный кадр (оригинал)
    QString m_filteredFramePath; // результат после фильтров

    QUrl m_originalUrl;
    QUrl m_filteredUrl;
    bool m_busy = false;
    QString m_status;
    bool m_initialized = false;
};

#endif // FILTERPREVIEWMANAGER_H
