#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "systemmonitor.h"
#include "filtermanager.h"
#include "upscalemanager.h"
#include "settingsmanager.h"
#include "gpuupscaler.h"
#include "performancemonitor.h"
#include "presetmanager.h"
#include "filterpreviewmanager.h"
#include "exportjob.h"
#include "jobqueue.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("UpsNeyro");
    QCoreApplication::setOrganizationDomain("upsneyro.com");
    QCoreApplication::setApplicationName("AIVideoEnhancer");

    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");

    qmlRegisterUncreatableType<PerformanceMonitor>(
        "UpsNeyro2", 1, 0,
        "PerformanceMonitor",
        "Created internally by UpscaleManager");
    qmlRegisterType<SystemMonitor>("UpsNeyro2", 1, 0, "SystemMonitor");
    qmlRegisterType<FilterManager>("UpsNeyro2", 1, 0, "FilterManager");
    qmlRegisterType<UpscaleManager>("UpsNeyro2", 1, 0, "UpscaleManager");
    qmlRegisterType<SettingsManager>("UpsNeyro2", 1, 0, "SettingsManager");
    qmlRegisterType<GpuUpscaler>("UpsNeyro2", 1, 0, "GpuUpscaler");
    qmlRegisterType<PresetManager>("UpsNeyro2", 1, 0, "PresetManager");
    qmlRegisterType<FilterPreviewManager>("UpsNeyro2", 1, 0, "FilterPreviewManager");
    qmlRegisterType<ExportJob>("UpsNeyro2", 1, 0, "ExportJob");
    qmlRegisterType<JobQueue> ("UpsNeyro2", 1, 0, "JobQueue");

    QQmlApplicationEngine engine;

    const QUrl url(QStringLiteral("qrc:/UpsNeyro2/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl)
        {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
