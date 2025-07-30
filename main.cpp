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

#include "fs/global_settings.h"

int main(int argc, char *argv[]) {

    QGuiApplication a(argc, argv);

    // アプリケーションの設定
    a.setOrganizationName("YourOrganization");
    a.setApplicationName("ManageMySelf");
    // a.setWindowIcon(QIcon(":/icon.png"));  // アイコンがある場合

    QQmlApplicationEngine engine;

    // QMLのインポートパスを設定
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

    // アプリケーション終了のシグナル接続
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &a, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    // QMLのロード
    engine.load(url);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "FATAL: Failed to load QML. The list of root objects is empty.";
        qCritical() << "This usually means the resource was not found (check qrc and CMakeLists.txt)";
        qCritical() << "or the QML file itself contains a syntax error.";
        
        // Print import paths for debugging
        const auto importPaths = engine.importPathList();
        qCritical() << "Current QML import paths:";
        for (const auto &path : importPaths) {
            qCritical() << "  " << path;
        }
        
        return -1;
    }

    return a.exec();
}