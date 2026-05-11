#include "projectmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

static QString sessionLocalPath(const QString &filePath)
{
    if (filePath.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))
        return QUrl(filePath).toLocalFile();
    return filePath;
}

ProjectManager::ProjectManager(QObject *parent)
    : QObject(parent)
{
}

bool ProjectManager::saveSession(const QString &filePath, const QVariantMap &data)
{
    if (filePath.isEmpty()) {
        emit error(tr("Empty path"));
        return false;
    }

    QJsonObject root;
    root[QStringLiteral("format")] = SessionFormatVersion;
    root[QStringLiteral("app")] = QStringLiteral("UpsNeyro");
    root[QStringLiteral("payload")] = QJsonObject::fromVariantMap(data);

    const QJsonDocument doc(root);
    QFile f(sessionLocalPath(filePath));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit error(f.errorString());
        return false;
    }
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    emit sessionSaved(filePath);
    return true;
}

QVariantMap ProjectManager::loadSession(const QString &filePath)
{
    QVariantMap out;
    if (filePath.isEmpty()) {
        emit error(tr("Empty path"));
        return out;
    }

    QFile f(sessionLocalPath(filePath));
    if (!f.open(QIODevice::ReadOnly)) {
        emit error(f.errorString());
        return out;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();

    if (!doc.isObject()) {
        emit error(tr("Invalid session file"));
        return out;
    }

    const QJsonObject root = doc.object();
    if (root.value(QStringLiteral("format")).toInt() != SessionFormatVersion) {
        emit error(tr("Unsupported session format"));
        return out;
    }

    const QJsonObject payload = root.value(QStringLiteral("payload")).toObject();
    out = payload.toVariantMap();
    emit sessionLoaded(filePath);
    return out;
}

QVariantMap ProjectManager::mergeWithDefaults(const QVariantMap &loaded) const
{
    QVariantMap m = loaded;
    if (!m.contains(QStringLiteral("outputDir")))
        m.insert(QStringLiteral("outputDir"), QString());
    return m;
}
