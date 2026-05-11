#include "updatechecker.h"
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVersionNumber>

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

UpdateChecker::~UpdateChecker()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
    }
}

QString UpdateChecker::currentVersion() const
{
    return QCoreApplication::applicationVersion();
}

void UpdateChecker::setManifestUrl(const QUrl &url)
{
    if (m_manifestUrl == url)
        return;
    m_manifestUrl = url;
    emit manifestUrlChanged();
}

void UpdateChecker::checkAsync()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    if (!m_manifestUrl.isValid()) {
        emit checkFinished(false, QString(), QString(), QUrl());
        return;
    }

    QNetworkRequest req(m_manifestUrl);
    req.setRawHeader("Accept", "application/json");
    req.setTransferTimeout(15000);
    m_reply = m_nam->get(req);
    connect(m_reply, &QNetworkReply::finished, this, &UpdateChecker::onNetworkFinished);
}

void UpdateChecker::onNetworkFinished()
{
    if (!m_reply)
        return;

    m_reply->deleteLater();
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFinished(false, QString(), QString(), QUrl());
        return;
    }

    const QByteArray data = reply->readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit checkFinished(false, QString(), QString(), QUrl());
        return;
    }

    const QJsonObject o = doc.object();
    const QString tag = o.value(QStringLiteral("tag_name")).toString();
    const QString body = o.value(QStringLiteral("body")).toString();
    const QString htmlUrl = o.value(QStringLiteral("html_url")).toString();

    const QString cur = currentVersion();
    QString cleanTag = tag;
    if (cleanTag.startsWith(QLatin1Char('v')))
        cleanTag = cleanTag.mid(1);

    const QVersionNumber remote = QVersionNumber::fromString(cleanTag);
    const QVersionNumber local = QVersionNumber::fromString(cur);
    const bool newer = !remote.isNull() && !local.isNull() && remote > local;
    emit checkFinished(newer, tag, body, QUrl(htmlUrl));
}
