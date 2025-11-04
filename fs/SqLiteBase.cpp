#include "../fs/SqLiteBase.h"

#include "global_settings.h"

#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/SQLiteCpp.h>
#include <qcontainerfwd.h>

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] QString SqLiteBase::getMainDatabasePath() const {
    const QString RETURN_VALUE = "";

    const QString DOCUMENTS_PATH =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (DOCUMENTS_PATH.isEmpty()) {
        qWarning() << "Cannot get the path for document folder";
        return "";
    }

    QDir settings_dir(DOCUMENTS_PATH);
    if (!settings_dir.exists()) {
        qWarning() << "Documents directory does not exist:" << DOCUMENTS_PATH;
        return "";
    }

    settings_dir.cd(GlobalSettings::SETTINGS_DIR_NAME);

    QString file_path = settings_dir.filePath("ManageMySelf.db");

    return file_path;
}

int SqLiteBase::checkMainDatabaseAndCreate() const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot check or create the database.";
        return -1; // パスが空の場合のエラーコード
    }

    QDir db_dir(QFileInfo(db_path).absolutePath());
    if (!db_dir.exists()) {
        qInfo() << "Creating database directory:" << db_dir.absolutePath();
        if (!db_dir.mkpath(".")) {
            qWarning() << "Failed to create database directory:" << db_dir.absolutePath();
            return -2; // ディレクトリ作成失敗
        }
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        qInfo() << "Database checked or created successfully at:" << db_path;

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

    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking or creating the database:" << e.what();
        return -3; // SQLiteエラーの場合のエラーコード
    }

    return 0; // 成功
}

int SqLiteBase::addRecentFile(const QString& filePath) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot add recent file.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(
            db, "INSERT INTO recent_files (path, last_opened_at) VALUES (?, CURRENT_TIMESTAMP) "
                "ON CONFLICT(path) DO UPDATE SET last_opened_at=excluded.last_opened_at");
        query.bind(1, filePath.toStdString());
        query.exec();
        qInfo() << "Recent file added:" << filePath;
        return 0; // 成功
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while adding recent file:" << e.what();
        return -2; // SQLiteエラーの場合のエラーコード
    }
}

int SqLiteBase::existCheckWorkspaceAtName(const QString& name) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot check workspace existence.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM workspaces WHERE name = ?");
        query.bind(1, name.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt(); // 存在する場合は1、存在しない場合は0
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking workspace existence:" << e.what();
    }

    return -2; // エラーコード
}

int SqLiteBase::existCheckWorkspaceAtPath(const QString& path) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot check workspace existence.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM workspaces WHERE path = ?");
        query.bind(1, path.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt(); // 存在する場合は1、存在しない場合は0
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking workspace existence:" << e.what();
    }

    return -2; // エラーコード
}

int SqLiteBase::addWorkspace(const QString& name, const QString& path) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot add workspace.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO workspaces (name, path) VALUES (?, ?) "
                                    "ON CONFLICT(name) DO UPDATE SET path=excluded.path");
        query.bind(1, name.toStdString());
        query.bind(2, path.toStdString());
        query.exec();
        qInfo() << "Workspace added:" << name << "at" << path;
        return 0; // 成功
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while adding workspace:" << e.what();
        return -2; // SQLiteエラーの場合のエラーコード
    }
}

QStringList SqLiteBase::getRecentFiles(int limit) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot retrieve recent files.";
        return {}; // パスが空の場合は空のリストを返す
    }

    QStringList recent_files;
    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(
            db, "SELECT path FROM recent_files ORDER BY last_opened_at DESC LIMIT ?");
        query.bind(1, limit);

        while (query.executeStep()) {
            recent_files.append(QString::fromStdString(query.getColumn(0).getText()));
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while retrieving recent files:" << e.what();
    }

    return recent_files;
}

QStringList SqLiteBase::getWorkspaces() const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot retrieve workspaces.";
        return {}; // パスが空の場合は空のリストを返す
    }

    QStringList workspaces;
    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT name FROM workspaces ORDER BY created_at DESC");

        while (query.executeStep()) {
            workspaces.append(QString::fromStdString(query.getColumn(0).getText()));
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while retrieving workspaces:" << e.what();
    }

    return workspaces;
}

int SqLiteBase::deleteWorkspace(const QString& name) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot delete workspace.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "DELETE FROM workspaces WHERE name = ?");
        query.bind(1, name.toStdString());
        query.exec();
        qInfo() << "Workspace deleted:" << name;
        return 0; // 成功
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while deleting workspace:" << e.what();
        return -2; // SQLiteエラーの場合のエラーコード
    }
}

QStringList SqLiteBase::getWorkspaceWithName(const QString& name) const {
    QString db_path = getMainDatabasePath();
    if (db_path.isEmpty()) {
        qWarning() << "Database path is empty. Cannot retrieve workspace.";
        return {}; // パスが空の場合は空のリストを返す
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(db_path.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT path FROM workspaces WHERE name = ?");
        query.bind(1, name.toStdString());

        if (query.executeStep()) {
            QStringList workspace;
            workspace.append(name);
            workspace.append(QString::fromStdString(query.getColumn(0).getText()));
            return workspace;
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while retrieving workspace:" << e.what();
    }

    return {}; // ワークスペースが見つからない場合は空のリストを返す
}