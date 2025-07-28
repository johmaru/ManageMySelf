//
// Created by Johma on 25/07/22.
//

#include "../fs/global_settings.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QApplication>
#include <QWidget>
#include <QJsonDocument>


const QString SETTINGS_DIR_NAME = "ManageMySelf";
const QString SETTINGS_FILE_NAME = "settings.json";

// 呼び出すと現在定義されている 'GlobalSettings'に値がセットされる。
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