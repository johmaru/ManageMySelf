#include <QString>
#include <QFile>
#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QTranslator>
#include <QLocale>

#include "fs/global_settings.h"

int main(int argc, char *argv[]) {

    QApplication a(argc, argv);

    QQmlApplicationEngine engine;

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

    // 翻訳ファイルのロード
    QTranslator translator;
    QString locale = settings->getLanguage();
    qDebug() << locale;
    QString translationFile = QString(":/i18n/ManageMySelf_%1.qm").arg(locale);

    if (QFile::exists(":/i18n/ManageMySelf_" + locale + ".qm")) {
        if (translator.load(translationFile)) {
            a.installTranslator(&translator);
            qDebug() << "Loaded translation file:" << translationFile;
        } else {
            qDebug() << "Failed to load translation file:" << translationFile;
        }
    } else {
        qDebug() << "Translation file does not exist:" << translationFile;
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