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
#include <QLibraryInfo>

#include "fs/SqLiteBase.h"
#include "fs/global_settings.h"
#include <QCommandLineParser>

int main(int argc, char *argv[]) {

    QGuiApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(true);

	QCommandLineParser parser;
	parser.setApplicationDescription("ManageMySelf - Personal Management Application");
	parser.addHelpOption();

    QCommandLineOption parentWinIdOpt(QStringLiteral("parent-winid"),
        QStringLiteral("Parent window WId (owner)"),
        QStringLiteral("id"));
    QCommandLineOption identOpt(("ident"),
        QStringLiteral("Unique identifier for the application instance"),
		QStringLiteral("n"));
    QCommandLineOption workspaceOpt(QStringLiteral("workspace"),
        QStringLiteral("Workspace path"), QStringLiteral("path"));
    QCommandLineOption scopeOpt(QStringLiteral("scope"), QStringLiteral("Graph scope"), QStringLiteral("n"));
    QCommandLineOption filterOpt(QStringLiteral("filter"), QStringLiteral("Graph filter"), QStringLiteral("n"));
	QCommandLineOption toOpt(QStringLiteral("to"), QStringLiteral("Graph to date"), QStringLiteral("n"));
	QCommandLineOption fromOpt(QStringLiteral("from"), QStringLiteral("Graph from date"), QStringLiteral("n"));
    parser.addOption(parentWinIdOpt);
    parser.addOption(workspaceOpt);
    parser.addOption(scopeOpt);
    parser.addOption(filterOpt);
    parser.addOption(identOpt);
	parser.addOption(toOpt);
	parser.addOption(fromOpt);
    parser.process(a);


	const QString parentIdStr = parser.value(parentWinIdOpt);
	const QString workspacePathArg = parser.value(workspaceOpt);
	const int scopeArgs = parser.value(scopeOpt).toInt();
	const int filterArgs = parser.value(filterOpt).toInt();
	const QString identStr = parser.value(identOpt);
	const QString toStr = parser.value(toOpt);
	const QString fromStr = parser.value(fromOpt);

    a.setOrganizationName("Johma");
    a.setApplicationName("ManageMySelf");

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("initialWorkspacePath"), workspacePathArg);
	engine.rootContext()->setContextProperty(QStringLiteral("initialScope"), scopeArgs);
	engine.rootContext()->setContextProperty(QStringLiteral("initialFilter"), filterArgs);
	engine.rootContext()->setContextProperty(QStringLiteral("initialTo"), toStr);
	engine.rootContext()->setContextProperty(QStringLiteral("initialFrom"), fromStr);
	engine.rootContext()->setContextProperty(QStringLiteral("applicationIdent"), identStr.isEmpty() ? QStringLiteral("main") : identStr);
    engine.addImportPath("qrc:/");
  
    const QString envQmlPath = qEnvironmentVariable("QT_QML_IMPORT_PATH");
    if (!envQmlPath.isEmpty()) {
        engine.addImportPath(envQmlPath);
    } else {
        const QString qtQml = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
        if (!qtQml.isEmpty()) engine.addImportPath(qtQml);
    }

    auto *settings = new GlobalSettings(&engine);

    const QString filePath = settings->getFilePath();

    if (!filePath.isEmpty() || settings->loadFromFile(filePath)) {
        if (settings->migrationJson(filePath) != 0) {
            qWarning() << "Failed to migrate settings file:" << filePath;
        }
    }

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

	auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().front());
    std::unique_ptr<QWindow> foreignOwner;

    if (!parentIdStr.isEmpty() && win)
    {
        bool ok = false;
		quint64 id = parentIdStr.toULongLong(&ok, 0);
        if (ok) {
            QWindow *parent = QWindow::fromWinId((WId)id);
            if (parent) {
                foreignOwner.reset(parent);
                win->setTransientParent(parent);
                win->setFlags(win->flags() | Qt::Window);
                QObject::connect(win, &QQuickWindow::closing, win,
                    [&foreignOwner](QQuickCloseEvent*) {foreignOwner.reset(); });

                QObject::connect(&a, &QCoreApplication::aboutToQuit, &a,
                    [&foreignOwner] {foreignOwner.reset(); });
            } else {
                qWarning() << "Failed to find window with WId" << id << "to set as parent.";
            }
        } else {
            qWarning() << "Invalid parent window ID format:" << parentIdStr;
		}
    }

    return a.exec();
}