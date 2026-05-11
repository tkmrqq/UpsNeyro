#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QVariantMap>

#include "projectmanager.h"

class TestProjectManager : public QObject
{
    Q_OBJECT

private slots:
    void saveLoadRoundTrip();
    void saveRejectsEmptyPath();
    void loadRejectsInvalidJson();
    void loadSupportsFileUrl();
    void mergeWithDefaultsAddsOutputDir();
};

void TestProjectManager::saveLoadRoundTrip()
{
    ProjectManager pm;

    QVariantMap payload;
    payload.insert(QStringLiteral("outputDir"), QStringLiteral("/tmp/out"));
    payload.insert(QStringLiteral("resolution"), QStringLiteral("4K"));

    QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/upsneyro_test_XXXXXX.json"));
    tmp.setAutoRemove(false);
    QVERIFY(tmp.open());
    const QString path = tmp.fileName();
    tmp.close();
    QFile::remove(path);

    QVERIFY(pm.saveSession(path, payload));

    const QVariantMap loaded = pm.loadSession(path);
    QCOMPARE(loaded.value(QStringLiteral("resolution")).toString(), QStringLiteral("4K"));
    QCOMPARE(loaded.value(QStringLiteral("outputDir")).toString(), QStringLiteral("/tmp/out"));
}

void TestProjectManager::saveRejectsEmptyPath()
{
    ProjectManager pm;
    QSignalSpy errorSpy(&pm, &ProjectManager::error);
    const bool ok = pm.saveSession(QString(), QVariantMap{});
    QVERIFY(!ok);
    QCOMPARE(errorSpy.size(), 1);
}

void TestProjectManager::loadRejectsInvalidJson()
{
    ProjectManager pm;
    QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/upsneyro_bad_XXXXXX.json"));
    tmp.setAutoRemove(false);
    QVERIFY(tmp.open());
    const QString path = tmp.fileName();
    tmp.write("not-json");
    tmp.close();

    QSignalSpy errorSpy(&pm, &ProjectManager::error);
    const QVariantMap loaded = pm.loadSession(path);
    QVERIFY(loaded.isEmpty());
    QCOMPARE(errorSpy.size(), 1);
}

void TestProjectManager::loadSupportsFileUrl()
{
    ProjectManager pm;
    QVariantMap payload;
    payload.insert(QStringLiteral("resolution"), QStringLiteral("1080p"));

    QTemporaryFile tmp(QDir::tempPath() + QStringLiteral("/upsneyro_fileurl_XXXXXX.json"));
    tmp.setAutoRemove(false);
    QVERIFY(tmp.open());
    const QString localPath = tmp.fileName();
    tmp.close();
    QFile::remove(localPath);

    QVERIFY(pm.saveSession(localPath, payload));

    const QString fileUrl = QUrl::fromLocalFile(localPath).toString();
    const QVariantMap loaded = pm.loadSession(fileUrl);
    QCOMPARE(loaded.value(QStringLiteral("resolution")).toString(), QStringLiteral("1080p"));
}

void TestProjectManager::mergeWithDefaultsAddsOutputDir()
{
    ProjectManager pm;
    const QVariantMap merged = pm.mergeWithDefaults(QVariantMap{});
    QVERIFY(merged.contains(QStringLiteral("outputDir")));
    QCOMPARE(merged.value(QStringLiteral("outputDir")).toString(), QString());
}

QTEST_MAIN(TestProjectManager)
#include "test_projectmanager.moc"
