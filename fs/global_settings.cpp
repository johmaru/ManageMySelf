//
// Created by Johma on 25/07/22.
//

#include "../fs/global_settings.h"
#include "SqLiteBase.h"
#include "UserSql.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QWidget>
#include <QJsonDocument>
#include <qcontainerfwd.h>



const QString SETTINGS_FILE_NAME = "settings.json";

// 呼び出すと現在定義されている 'GlobalSettings'にDocumentのディレクトリパスがセットされる。
// IO副作用として、'SETTINGS_DIR_NAME'のディレクトリが特殊パスDocumentに存在しない場合に、ディレクトリが作成される。
QString GlobalSettings::getFilePath() const {
    const QString documentsPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    if (documentsPath.isEmpty()) {
        qWarning() << "Cant get the path for document folder";
        return "";
    }

    QDir settingsDir(documentsPath);
    if (!settingsDir.exists(SETTINGS_DIR_NAME)) {
        qInfo() << "Currently creating directory..." << settingsDir.filePath(SETTINGS_DIR_NAME);
        if (!settingsDir.mkdir(SETTINGS_DIR_NAME)) {
            qWarning() << "Cant create directory" << settingsDir.filePath(SETTINGS_DIR_NAME);
            return "";
        }
    }

    settingsDir.cd(SETTINGS_DIR_NAME);

    QString filePath = settingsDir.filePath(SETTINGS_FILE_NAME);
    qInfo() << "Setting Path : " << filePath;

    return filePath;
}

QStringList GlobalSettings::getWorkspaces() const {
    SqLiteBase sqlite;
    return sqlite.getWorkspaces();
}

QStringList GlobalSettings::getWorkspaceWithName(const QString &name) const {
    SqLiteBase sqlite;
    return sqlite.getWorkspaceWithName(name);
}

int GlobalSettings::createWorkspaceFromQml(const QString &userName, const QString &name, const QString &path) {
    if (name.isEmpty()) {
        qWarning() << "Workspace name cannot be empty";
        return -1;
    }
    
    if (path.isEmpty()) {
        qWarning() << "Workspace path cannot be empty";
        return -1;
    }
    
    QStringList items;
    items << userName << name << path;

    int result = createWorkspace(items);

    emit workspaceCreated(userName, name, path, result);

    return result;
}

int GlobalSettings::createWorkspace(const QStringList &items) const {
    if (items.size() < 3) {
        qWarning() << "Insufficient arguments for workspace creation. Expected: userName, name, path";
        return -1;
    }

    const QString userName = items.at(0);
    const QString workspaceName = items.at(1);
    const QString workspacePath = items.at(2);
    
    qInfo() << "Creating workspace:" << workspaceName << "at" << workspacePath;
    

    QString fullPath = QDir(workspacePath).filePath(workspaceName);

    if (!QDir().mkpath(fullPath)) {
        qWarning() << "Failed to create workspace directory:" << fullPath;
        return -1;
    }

    QString dbPath = QDir(fullPath).filePath("user.db");

    UserSql userSql(dbPath);
    int resultCreateUserDb = userSql.createUserDatabase(fullPath);
    if (resultCreateUserDb != 0) {
        qWarning() << "Failed to create user database at:" << fullPath << "Error code:" << resultCreateUserDb;
        return resultCreateUserDb; // エラーコードを返す
    } else {
        qInfo() << "User database created successfully at:" << fullPath;
    }

    // ここで設定を作成 & 保存

    QJsonObject settingsJson;
    settingsJson["userName"] = userName;
    
    GlobalSettings settings;
    if (!settings.saveToFileAny(QDir(fullPath).filePath("settings.json"), settingsJson)) {
        qWarning() << "Failed to create settings file at:" << QDir(fullPath).filePath("settings.json");
    } else {
        qInfo() << "Settings file created at:" << QDir(fullPath).filePath("settings.json");
    }

    qInfo() << "Workspace created successfully at:" << fullPath;

    SqLiteBase db;

    int exists = db.ExistCheckWorkspaceAtName(workspaceName);
    if (exists > 0) {
        qWarning() << "Workspace with name" << workspaceName << "already exists.";
        return -2; // Workspace already exists
    } else if (exists < 0) {
        qWarning() << "Error checking workspace existence:" << exists;
        return -3; // Error checking existence
    }

    int result = db.addWorkspace(workspaceName, fullPath);
    if (result == 0) {
        qInfo() << "Workspace added to database successfully.";
    } else {
        qWarning() << "Failed to add workspace to database. Error code:" << result;
    }

    return 0;
}

int GlobalSettings::deleteWorkspace(const QString &name) const {
    SqLiteBase db;
    int exists = db.ExistCheckWorkspaceAtName(name);
    if (exists <= 0) {
        qWarning() << "Workspace with name" << name << "does not exist or an error occurred.";
        return -1; // ワークスペースがない場合
    }

    QStringList workspaceInfo = db.getWorkspaceWithName(name);
    if (workspaceInfo.isEmpty()) {
        qWarning() << "No workspace found with name:" << name;
        return -2; // ワークスペースが見つからない場合
    }

    QString path = workspaceInfo.at(1);
    QDir dir(path);
    if (!dir.removeRecursively()) {
        qWarning() << "Failed to delete workspace directory at:" << path;
        return -3; // ディレクトリの削除に失敗した場合
    }

    int result = db.deleteWorkspace(name);
    if (result != 0) {
        qWarning() << "Failed to remove workspace from database. Error code:" << result;
        return -4; // データベースから削除に失敗した場合
    }

    qInfo() << "Workspace deleted successfully:" << name;
    return 0; // 成功
}

int GlobalSettings::openWorkspace(const QString &path) const {
    if (path.isEmpty()) {
        qWarning() << "Workspace path cannot be empty";
        return -1; // パスが空の場合のエラーコード
    }

    SqLiteBase db;
    int exists = db.ExistCheckWorkspaceAtPath(path);
    if (exists > 0) {
        qInfo() << "Workspace exists at path:" << path;

        QString user_db_path = UserSql::getUserDatabasePath(path);
        if (user_db_path.isEmpty()) {
            qWarning() << "Failed to get user database path for workspace at:" << path;
            return -4; // ユーザーデータベースパスが取得できない場合のエラーコード
        } 
        return 0; // 成功
    } else if (exists < 0) {
        qWarning() << "Error checking workspace existence:" << exists;
        return -2; // 存在チェックのエラーコード
    }

    qWarning() << "No workspace found at path:" << path;
    return -3; // ワークスペースが存在しない場合のエラーコード
}

QStringList GlobalSettings::getSettings(const QString &path) const {
    if (path.isEmpty()) {
        qWarning() << "Path is empty. Cannot get settings.";
        return QStringList(); // パスが空の場合は空のリストを返す
    }

    QDir dir(path);
    QString settingsFilePath = dir.filePath("settings.json");

    if (!QFile::exists(settingsFilePath)) {
        qWarning() << "Settings file does not exist at path:" << settingsFilePath;
        return QStringList(); // 設定ファイルが存在しない場合は空のリストを返す
    }

    QJsonObject json = JsonSettingsBase::loadFromFileAny(settingsFilePath);
    if (json.isEmpty()) {
        qWarning() << "Failed to load settings from file:" << settingsFilePath;
        return QStringList(); // 設定の読み込みに失敗した場合は空のリストを返す
    }

    QStringList settings;
    settings << json["userName"].toString();

    return settings;
}