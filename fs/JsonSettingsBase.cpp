//
// Created by Johma on 25/07/22.
//

#include "../fs/JsonSettingsBase.h"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

// IO副作用として、指定のパスにファイルを新規作成、または、編集する
bool JsonSettingsBase::saveToFile(const QString &filePath) const {
    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open save file" << filePath;
        return false;
    }

    const QJsonObject settingsObject = this->toJson();

    const QJsonDocument saveDoc(settingsObject);
    saveFile.write(saveDoc.toJson(QJsonDocument::Indented));
    saveFile.close();

    return true;
}

// IO副作用として、指定のパスにファイルを読み込み、設定を更新する
int JsonSettingsBase::saveToFileAny(const QString &filePath, const QJsonObject &json) {
    QFile saveFile(filePath);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open save file" << filePath;
        return -1; // エラーコード
    }

    const QJsonDocument saveDoc(json);
    saveFile.write(saveDoc.toJson(QJsonDocument::Indented));
    saveFile.close();

    return 0; // 成功コード
}

QJsonObject JsonSettingsBase::loadFromFileAny(const QString &filePath) {
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open settings file for reading:" << filePath;
        return QJsonObject(); // 空のオブジェクトを返す
    }

    const QByteArray saveData = loadFile.readAll();
    loadFile.close();

    QJsonParseError parseError;
    const QJsonDocument loadDoc = QJsonDocument::fromJson(saveData, &parseError);

    if (loadDoc.isNull()) {
        qWarning() << "Couldn't parse settings file:" << filePath;
        qWarning() << "Parse error:" << parseError.errorString();
        return QJsonObject(); // 空のオブジェクトを返す
    }

    if (!loadDoc.isObject()) {
        qWarning() << "Settings file does not contain a JSON object:" << filePath;
        return QJsonObject(); // 空のオブジェクトを返す
    }

    return loadDoc.object();
}

bool JsonSettingsBase::loadFromFile(const QString &filePath) {
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open settings file for reading:" << filePath;
        return false;
    }

    const QByteArray saveData = loadFile.readAll();
    loadFile.close();

    QJsonParseError parseError;
    const QJsonDocument loadDoc = QJsonDocument::fromJson(saveData, &parseError);

    if (loadDoc.isNull()) {
        qWarning() << "Couldn't parse settings file:" << filePath;
        qWarning() << "Parse error:" << parseError.errorString();
        return false;
    }

    if (!loadDoc.isObject()) {
        qWarning() << "Settings file does not contain a JSON object:" << filePath;
        return false;
    }

    this->loadFromJson(loadDoc.object());
    return true;
}