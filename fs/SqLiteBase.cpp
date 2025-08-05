#include "../fs/SqLiteBase.h"
#include "global_settings.h"
#include <SQLiteCpp/Database.h>
#include <qcontainerfwd.h>
#include <QStandardPaths>
#include <QDebug>
#include <QDir>
#include <SQLiteCpp/SQLiteCpp.h>

[[nodiscard]] QString SqLiteBase::getMainDatabasePath() const {
    const QString returnValue = "";

    const QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documentsPath.isEmpty()) {
        qWarning() << "Cannot get the path for document folder";
        return "";
    }

    QDir settingsDir(documentsPath);
    if (!settingsDir.exists()) {
        qWarning() << "Documents directory does not exist:" << documentsPath;
        return "";
    }

    settingsDir.cd(GlobalSettings::SETTINGS_DIR_NAME);

    QString filePath = settingsDir.filePath("ManageMySelf.db");

    return filePath;
}

int SqLiteBase::checkMainDatabaseAndCreate() const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot check or create the database.";
        return -1; // パスが空の場合のエラーコード
    }

    QDir dbDir(QFileInfo(dbPath).absolutePath());
    if (!dbDir.exists()) {
        qInfo() << "Creating database directory:" << dbDir.absolutePath();
        return -2; // ディレクトリが存在しない場合のエラーコード
    }

    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        qInfo() << "Database checked or created successfully at:" << dbPath;
        
        db.exec("CREATE TABLE IF NOT EXISTS recent_files ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "path TEXT NOT NULL UNIQUE, "
            "last_opened_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

        qInfo() << "Recent files table ensured in the database.";

        db.exec("CREATE TABLE IF NOT EXISTS workspaces ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL UNIQUE, "
            "path TEXT NOT NULL UNIQUE, "
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

        qInfo() << "Workspaces table ensured in the database.";

    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking or creating the database:" << e.what();
        return -3; // SQLiteエラーの場合のエラーコード
    }

    return 0; // 成功
}

int SqLiteBase::addRecentFile(const QString &filePath) const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot add recent file.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO recent_files (path, last_opened_at) VALUES (?, CURRENT_TIMESTAMP) "
                                "ON CONFLICT(path) DO UPDATE SET last_opened_at=excluded.last_opened_at");
        query.bind(1, filePath.toStdString());
        query.exec();
        qInfo() << "Recent file added:" << filePath;
        return 0; // 成功
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while adding recent file:" << e.what();
        return -2; // SQLiteエラーの場合のエラーコード
    }
}

int SqLiteBase::ExistCheckWorkspaceAtName(const QString &name) const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot check workspace existence.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM workspaces WHERE name = ?");
        query.bind(1, name.toStdString());
        
        if (query.executeStep()) {
            return query.getColumn(0).getInt(); // 存在する場合は1、存在しない場合は0
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking workspace existence:" << e.what();
    }

    return -2; // エラーコード
}

int SqLiteBase::addWorkspace(const QString &name, const QString &path) const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot add workspace.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO workspaces (name, path) VALUES (?, ?) "
                                "ON CONFLICT(name) DO UPDATE SET path=excluded.path");
        query.bind(1, name.toStdString());
        query.bind(2, path.toStdString());
        query.exec();
        qInfo() << "Workspace added:" << name << "at" << path;
        return 0; // 成功
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while adding workspace:" << e.what();
        return -2; // SQLiteエラーの場合のエラーコード
    }
}

QStringList SqLiteBase::getRecentFiles(int limit) const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot retrieve recent files.";
        return QStringList(); // パスが空の場合は空のリストを返す
    }

    QStringList recentFiles;
    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT path FROM recent_files ORDER BY last_opened_at DESC LIMIT ?");
        query.bind(1, limit);
        
        while (query.executeStep()) {
            recentFiles.append(QString::fromStdString(query.getColumn(0).getText()));
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while retrieving recent files:" << e.what();
    }

    return recentFiles;
}

QStringList SqLiteBase::getWorkspaces() const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot retrieve workspaces.";
        return QStringList(); // パスが空の場合は空のリストを返す
    }

    QStringList workspaces;
    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT name FROM workspaces ORDER BY created_at DESC");
        
        while (query.executeStep()) {
            workspaces.append(QString::fromStdString(query.getColumn(0).getText()));
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while retrieving workspaces:" << e.what();
    }

    return workspaces;
}

QStringList SqLiteBase::getWorkspaceWithName(const QString &name) const {
    QString dbPath = getMainDatabasePath();
    if (dbPath.isEmpty()) {
        qWarning() << "Database path is empty. Cannot retrieve workspace.";
        return QStringList(); // パスが空の場合は空のリストを返す
    }

    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT path FROM workspaces WHERE name = ?");
        query.bind(1, name.toStdString());
        
        if (query.executeStep()) {
            QStringList workspace;
            workspace.append(name);
            workspace.append(QString::fromStdString(query.getColumn(0).getText()));
            return workspace;
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while retrieving workspace:" << e.what();
    }

    return QStringList(); // ワークスペースが見つからない場合は空のリストを返す
}