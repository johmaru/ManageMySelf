//
// Created by Johma on 25/07/22.
//

#include "../fs/JsonSettingsBase.h"

#include <QDebug>
#include <QFile>
#include <QJsonDocument>

// IO副作用として、指定のパスにファイルを新規作成、または、編集する
bool JsonSettingsBase::saveToFile(const QString& filePath) const {
    QFile save_file(filePath);
    if (!save_file.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open save file" << filePath;
        return false;
    }

    const QJsonObject SETTINGS_OBJECT = this->toJson();

    const QJsonDocument SAVE_DOC(SETTINGS_OBJECT);
    save_file.write(SAVE_DOC.toJson(QJsonDocument::Indented));
    save_file.close();

    return true;
}

// IO副作用として、指定のパスにファイルを読み込み、設定を更新する
int JsonSettingsBase::saveToFileAny(const QString& filePath, const QJsonObject& json) {
    QFile save_file(filePath);
    if (!save_file.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open save file" << filePath;
        return -1; // エラーコード
    }

    const QJsonDocument SAVE_DOC(json);
    save_file.write(SAVE_DOC.toJson(QJsonDocument::Indented));
    save_file.close();

    return 0; // 成功コード
}

QJsonObject JsonSettingsBase::loadFromFileAny(const QString& filePath) {
    QFile load_file(filePath);
    if (!load_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open settings file for reading:" << filePath;
        return {}; // 空のオブジェクトを返す
    }

    const QByteArray SAVE_DATA = load_file.readAll();
    load_file.close();

    QJsonParseError parse_error;
    const QJsonDocument LOAD_DOC = QJsonDocument::fromJson(SAVE_DATA, &parse_error);

    if (LOAD_DOC.isNull()) {
        qWarning() << "Couldn't parse settings file:" << filePath;
        qWarning() << "Parse error:" << parse_error.errorString();
        return {}; // 空のオブジェクトを返す
    }

    if (!LOAD_DOC.isObject()) {
        qWarning() << "Settings file does not contain a JSON object:" << filePath;
        return {}; // 空のオブジェクトを返す
    }

    return LOAD_DOC.object();
}

bool JsonSettingsBase::loadFromFile(const QString& filePath) {
    QFile load_file(filePath);
    if (!load_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Couldn't open settings file for reading:" << filePath;
        return false;
    }

    const QByteArray SAVE_DATA = load_file.readAll();
    load_file.close();

    QJsonParseError parse_error;
    const QJsonDocument LOAD_DOC = QJsonDocument::fromJson(SAVE_DATA, &parse_error);

    if (LOAD_DOC.isNull()) {
        qWarning() << "Couldn't parse settings file:" << filePath;
        qWarning() << "Parse error:" << parse_error.errorString();
        return false;
    }

    if (!LOAD_DOC.isObject()) {
        qWarning() << "Settings file does not contain a JSON object:" << filePath;
        return false;
    }

    this->loadFromJson(LOAD_DOC.object());
    return true;
}