#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QString>

/**
 * Сохранение и загрузка сессии (пути, выходная папка, ключевые настройки) в JSON.
 */
class ProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectManager(QObject *parent = nullptr);

    Q_INVOKABLE bool saveSession(const QString &filePath, const QVariantMap &data);
    Q_INVOKABLE QVariantMap loadSession(const QString &filePath);

    Q_INVOKABLE QVariantMap mergeWithDefaults(const QVariantMap &loaded) const;

signals:
    void sessionSaved(const QString &filePath);
    void sessionLoaded(const QString &filePath);
    void error(const QString &message);

private:
    static constexpr int SessionFormatVersion = 1;
};

#endif // PROJECTMANAGER_H
