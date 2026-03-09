#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include "systemmonitor.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("UpsNeyro");
    QCoreApplication::setOrganizationDomain("upsneyro.com");
    QCoreApplication::setApplicationName("AIVideoEnhancer");

    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Material");
    qmlRegisterType<SystemMonitor>("UpsNeyro2", 1, 0, "SystemMonitor");

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/UpsNeyro2/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
