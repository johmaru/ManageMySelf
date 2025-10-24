#include "fs/SqLiteBase.h"
#include "fs/global_settings.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSettings>
#include <QString>
#include <QStringLiteral>
#include <QTranslator>
#include <memory>
#include <qqmlcontext.h>

int main(int argc, char* argv[]) {

    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(true);

    QCommandLineParser parser;
    parser.setApplicationDescription("ManageMySelf - Personal Management Application");
    parser.addHelpOption();

    QCommandLineOption parent_win_id_opt(QStringLiteral("parent-winid"),
                                         QStringLiteral("Parent window WId (owner)"),
                                         QStringLiteral("id"));
    QCommandLineOption ident_opt(("ident"),
                                 QStringLiteral("Unique identifier for the application instance"),
                                 QStringLiteral("n"));
    QCommandLineOption workspace_opt(QStringLiteral("workspace"), QStringLiteral("Workspace path"),
                                     QStringLiteral("path"));
    QCommandLineOption scope_opt(QStringLiteral("scope"), QStringLiteral("Graph scope"),
                                 QStringLiteral("n"));
    QCommandLineOption filter_opt(QStringLiteral("filter"), QStringLiteral("Graph filter"),
                                  QStringLiteral("n"));
    QCommandLineOption to_opt(QStringLiteral("to"), QStringLiteral("Graph to date"),
                              QStringLiteral("n"));
    QCommandLineOption from_opt(QStringLiteral("from"), QStringLiteral("Graph from date"),
                                QStringLiteral("n"));
    parser.addOption(parent_win_id_opt);
    parser.addOption(workspace_opt);
    parser.addOption(scope_opt);
    parser.addOption(filter_opt);
    parser.addOption(ident_opt);
    parser.addOption(to_opt);
    parser.addOption(from_opt);
    parser.process(app);

    const QString PARENT_ID_STR = parser.value(parent_win_id_opt);
    const QString WORKSPACE_PATH_ARG = parser.value(workspace_opt);
    const int SCOPE_ARGS = parser.value(scope_opt).toInt();
    const int FILTER_ARGS = parser.value(filter_opt).toInt();
    const QString IDENT_STR = parser.value(ident_opt);
    const QString TO_STR = parser.value(to_opt);
    const QString FROM_STR = parser.value(from_opt);

    QApplication::setOrganizationName("Johma");
    QApplication::setApplicationName("ManageMySelf");

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty(QStringLiteral("initialWorkspacePath"),
                                             WORKSPACE_PATH_ARG);
    engine.rootContext()->setContextProperty(QStringLiteral("initialScope"), SCOPE_ARGS);
    engine.rootContext()->setContextProperty(QStringLiteral("initialFilter"), FILTER_ARGS);
    engine.rootContext()->setContextProperty(QStringLiteral("initialTo"), TO_STR);
    engine.rootContext()->setContextProperty(QStringLiteral("initialFrom"), FROM_STR);
    engine.rootContext()->setContextProperty(QStringLiteral("applicationIdent"),
                                             IDENT_STR.isEmpty() ? QStringLiteral("main")
                                                                 : IDENT_STR);
    engine.addImportPath("qrc:/");

    const QString ENV_QML_PATH = qEnvironmentVariable("QT_QML_IMPORT_PATH");
    if (!ENV_QML_PATH.isEmpty()) {
        engine.addImportPath(ENV_QML_PATH);
    } else {
        const QString QT_QML = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);
        if (!QT_QML.isEmpty()) {
            engine.addImportPath(QT_QML);
        }
    }

    auto settings_holder = std::make_unique<GlobalSettings>(&engine);
    GlobalSettings* settings = settings_holder.get();

    const QString FILE_PATH = settings->getFilePath();

    if (!FILE_PATH.isEmpty() && settings->loadFromFile(FILE_PATH)) {
        if (settings->migrationJson(FILE_PATH) != 0) {
            qWarning() << "Failed to migrate settings file:" << FILE_PATH;
        }
    }

    if (FILE_PATH.isEmpty() || !settings->loadFromFile(FILE_PATH)) {
        qWarning() << "Could not load settings from" << FILE_PATH
                   << ". Using default settings and creating a new file.";
        if (!FILE_PATH.isEmpty()) {
            if (!settings->saveToFile(FILE_PATH)) {
                qWarning() << "Failed to save initial settings file to" << FILE_PATH;
            }
        }
    }

    SqLiteBase sqlite;

    if (sqlite.checkMainDatabaseAndCreate() != 0) {
        qWarning() << "Failed to check or create the main database.";
        return -1; // Exit if the database cannot be created or checked
    }
    qInfo() << "Main database is ready.";

    settings->initialize(&engine);

    engine.rootContext()->setContextProperty("settings", settings);

    settings = settings_holder.release();

    const QUrl URL(QStringLiteral("qrc:/Main.qml"));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [URL](QObject* obj, const QUrl& objUrl) {
            if (!obj && URL == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    engine.load(URL);

    if (engine.rootObjects().isEmpty()) {
        qCritical() << "FATAL: Failed to load QML. The list of root objects is empty.";
        qCritical()
            << "This usually means the resource was not found (check qrc and CMakeLists.txt)";
        qCritical() << "or the QML file itself contains a syntax error.";

        const auto IMPORT_PATHS = engine.importPathList();
        qCritical() << "Current QML import paths:";
        for (const auto& path : IMPORT_PATHS) {
            qCritical() << "  " << path;
        }

        return -1;
    }

    auto* win = qobject_cast<QQuickWindow*>(engine.rootObjects().front());
    std::unique_ptr<QWindow> foreign_owner;

    if (!PARENT_ID_STR.isEmpty() && (win != nullptr)) {
        bool ok = false;
        quint64 id = PARENT_ID_STR.toULongLong(&ok, 0);
        if (ok) {
            QWindow* parent = QWindow::fromWinId((WId) id);
            if (parent != nullptr) {
                foreign_owner.reset(parent);
                win->setTransientParent(parent);
                win->setFlags(win->flags() | Qt::Window);
                QObject::connect(win, &QQuickWindow::closing, win,
                                 [&foreign_owner](QQuickCloseEvent*) { foreign_owner.reset(); });

                QObject::connect(&app, &QCoreApplication::aboutToQuit, &app,
                                 [&foreign_owner] { foreign_owner.reset(); });
            } else {
                qWarning() << "Failed to find window with WId" << id << "to set as parent.";
            }
        } else {
            qWarning() << "Invalid parent window ID format:" << PARENT_ID_STR;
        }
    }

    return QApplication::exec();
}