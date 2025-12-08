#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <iostream>

#include "i2cscanner.hpp"

int main(int argc, char *argv[]) {
    std::cout << "I2C scanner GUI" << std::endl;

    QGuiApplication app(argc, argv);

    I2CScanner scanner;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("i2cScanner", &scanner);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qt-qml", "Main");

    return app.exec();
}