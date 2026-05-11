#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

/**
 * Проверка обновлений (GitHub Releases или свой endpoint).
 * По умолчанию — заглушка; задайте setManifestUrl для реальной проверки.
 */
class UpdateChecker : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString currentVersion READ currentVersion CONSTANT)
    Q_PROPERTY(QUrl manifestUrl READ manifestUrl WRITE setManifestUrl NOTIFY manifestUrlChanged)

public:
    explicit UpdateChecker(QObject *parent = nullptr);
    ~UpdateChecker() override;

    QString currentVersion() const;
    QUrl manifestUrl() const { return m_manifestUrl; }
    void setManifestUrl(const QUrl &url);

    Q_INVOKABLE void checkAsync();

signals:
    void manifestUrlChanged();
    void checkFinished(bool updateAvailable,
                       const QString &latestVersion,
                       const QString &releaseNotes,
                       const QUrl &downloadPage);

private slots:
    void onNetworkFinished();

private:
    QNetworkAccessManager *m_nam = nullptr;
    QNetworkReply *m_reply = nullptr;
    QUrl m_manifestUrl;
};

#endif // UPDATECHECKER_H
