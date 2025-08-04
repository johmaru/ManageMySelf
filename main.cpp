#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QIcon>
#include <QTranslator>
#include <QLocale>
#include <QDebug>
#include <QFile>
#include <QString>
#include <QStringLiteral>
#include <qqmlcontext.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QSettings>
#include <QQuickStyle>
#include <QQmlComponent>
#include <QQuickWindow>

#include "fs/SqLiteBase.h"
#include "fs/global_settings.h"

int main(int argc, char *argv[]) {

    QGuiApplication a(argc, argv);

    a.setOrganizationName("Johma");
    a.setApplicationName("ManageMySelf");

    QQmlApplicationEngine engine;

    engine.addImportPath("qrc:/");
    engine.addImportPath("C:/Qt/6.9.1/mingw_64/qml");

    auto *settings = new GlobalSettings(&engine);

    const QString filePath = settings->getFilePath();

    if (filePath.isEmpty() || !settings->loadFromFile(filePath)) {
        qWarning() << "Could not load settings from" << filePath << ". Using default settings and creating a new file.";
        if (!filePath.isEmpty()) {
            if (!settings->saveToFile(filePath)) {
                qWarning() << "Failed to save initial settings file to" << filePath;
            }
        }
    }

    SqLiteBase sqlite;

    if (sqlite.checkMainDatabaseAndCreate() != 0) {
        qWarning() << "Failed to check or create the main database.";
        return -1; // Exit if the database cannot be created or checked
    } else {
        qInfo() << "Main database is ready.";
    }



    settings->initialize(&engine);

    engine.rootContext()->setContextProperty("settings", settings);

    const QUrl url(QStringLiteral("qrc:/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &a, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "FATAL: Failed to load QML. The list of root objects is empty.";
        qCritical() << "This usually means the resource was not found (check qrc and CMakeLists.txt)";
        qCritical() << "or the QML file itself contains a syntax error.";
        
        const auto importPaths = engine.importPathList();
        qCritical() << "Current QML import paths:";
        for (const auto &path : importPaths) {
            qCritical() << "  " << path;
        }
        
        return -1;
    }

    return a.exec();
}