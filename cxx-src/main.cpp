#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "SerialManager.h"
#include "libft4222.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    SerialManager serialManager;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("serialManager", &serialManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qt-qml", "Main");

    return app.exec();
}
