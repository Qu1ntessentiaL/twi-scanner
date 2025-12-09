#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>

#include "rtccontroller.hpp"

int main(int argc, char *argv[]) {
    std::cout << "RTC I2C GUI" << std::endl;

    QGuiApplication app(argc, argv);

    RTCController controller;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("rtcController", &controller);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qt-qml", "Main");

    return app.exec();
}