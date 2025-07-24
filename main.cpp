#include <iostream>
#include <QString>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>

#include "fs/global_settings.h"

int main(int argc, char *argv[]) {

    QGuiApplication a(argc, argv);

    QQmlApplicationEngine engine;

    auto *settings = new GlobalSettings(&engine);

    GlobalSettings fsSettings;

    const QString filePath = fsSettings.getFilePath();

    if (filePath.isEmpty() || !fsSettings.loadFromFile(filePath)) {
        qWarning() << "Could not load settings from" << filePath << ". Using default settings and creating a new file.";
        if (!filePath.isEmpty()) {
            if (!fsSettings.saveToFile(filePath)) {
                qWarning() << "Failed to save initial settings file to" << filePath;
            }
        }
    }


    engine.rootContext()->setContextProperty("settings", settings);

    const QUrl url(QStringLiteral("qrc:/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
    &a, [](const QUrl &url) {
        qWarning() << "Error: Cannot load QML file." << url;
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "FATAL: Failed to load QML. The list of root objects is empty.";
        qCritical() << "This usually means the resource was not found (check qrc and CMakeLists.txt)";
        qCritical() << "or the QML file itself contains a syntax error.";
        return -1;
    }

    return a.exec();
}