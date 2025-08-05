//
// Created by Johma on 25/07/22.
//

#include "../fs/global_settings.h"
#include "SqLiteBase.h"
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

int GlobalSettings::createWorkspaceFromQml(const QString &name, const QString &path) {
    if (name.isEmpty()) {
        qWarning() << "Workspace name cannot be empty";
        return -1;
    }
    
    if (path.isEmpty()) {
        qWarning() << "Workspace path cannot be empty";
        return -1;
    }
    
    QStringList items;
    items << name << path;
    
    int result = createWorkspace(items);
    
    emit workspaceCreated(name, path, result);
    
    return result;
}

int GlobalSettings::createWorkspace(const QStringList &items) const {
    if (items.size() < 2) {
        qWarning() << "Insufficient arguments for workspace creation. Expected: name, path";
        return -1;
    }
    
    const QString workspaceName = items.at(0);
    const QString workspacePath = items.at(1);
    
    qInfo() << "Creating workspace:" << workspaceName << "at" << workspacePath;
    

    QString fullPath = QDir(workspacePath).filePath(workspaceName);

    if (!QDir().mkpath(fullPath)) {
        qWarning() << "Failed to create workspace directory:" << fullPath;
        return -1;
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