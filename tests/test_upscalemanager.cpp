#include <QtTest/QtTest>
#include <QSignalSpy>
#include <QUrl>

#include "upscalemanager.h"
#include "updatechecker.h"

class TestUpscaleManager : public QObject
{
    Q_OBJECT

private slots:
    void startPreviewRejectsEmptyInput();
    void modePropertyRoundTrip();
    void resolutionPropertyRoundTrip();
    void manifestUrlChangeEmitsSignal();
};

void TestUpscaleManager::startPreviewRejectsEmptyInput()
{
    UpscaleManager um;
    QSignalSpy failedSpy(&um, &UpscaleManager::previewFailed);
    QSignalSpy busySpy(&um, &UpscaleManager::previewBusyChanged);

    um.startPreview(QString(), 0.0);

    QCOMPARE(failedSpy.size(), 1);
    QCOMPARE(failedSpy.at(0).at(0).toString(), QStringLiteral("No video selected"));
    QVERIFY(!um.previewBusy());
    QCOMPARE(busySpy.size(), 0);
}

void TestUpscaleManager::modePropertyRoundTrip()
{
    UpscaleManager um;
    QSignalSpy modeSpy(&um, &UpscaleManager::modeChanged);

    um.setMode(UpscaleManager::FastMode);
    QCOMPARE(um.mode(), UpscaleManager::FastMode);

    um.setMode(UpscaleManager::QualityMode);
    QCOMPARE(um.mode(), UpscaleManager::QualityMode);

    QCOMPARE(modeSpy.size(), 2);

    // Повторная установка того же значения не должна испускать сигнал.
    um.setMode(UpscaleManager::QualityMode);
    QCOMPARE(modeSpy.size(), 2);
}

void TestUpscaleManager::resolutionPropertyRoundTrip()
{
    UpscaleManager um;
    QSignalSpy resolutionSpy(&um, &UpscaleManager::resolutionChanged);

    um.setResolution(QStringLiteral("1080p"));
    QCOMPARE(um.resolution(), QStringLiteral("1080p"));

    um.setResolution(QStringLiteral("4K"));
    QCOMPARE(um.resolution(), QStringLiteral("4K"));

    QCOMPARE(resolutionSpy.size(), 2);

    // Повторная установка того же значения не должна испускать сигнал.
    um.setResolution(QStringLiteral("4K"));
    QCOMPARE(resolutionSpy.size(), 2);
}

void TestUpscaleManager::manifestUrlChangeEmitsSignal()
{
    UpdateChecker uc;
    QSignalSpy spy(&uc, &UpdateChecker::manifestUrlChanged);

    const QUrl manifestUrl(
        QStringLiteral("https://api.github.com/repos/owner/repo/releases/latest"));

    uc.setManifestUrl(manifestUrl);
    QCOMPARE(spy.size(), 1);

    // Повторная установка того же URL не должна испускать сигнал.
    uc.setManifestUrl(manifestUrl);
    QCOMPARE(spy.size(), 1);
}

QTEST_MAIN(TestUpscaleManager)
#include "test_upscalemanager.moc"
