//
// Created by Johma on 25/07/22.
//

#include "../fs/JsonSettingsBase.h"
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

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

bool JsonSettingsBase::loadFromFile(const QString &filePath) {
    if (!checkFile(filePath)) {
        return false;
    }
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open load file" << filePath;
        return false;
    }

    const QByteArray saveData = loadFile.readAll();
    const QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    loadFile.close();

    if (loadDoc.isNull()) {
        qWarning() << "Couldn't parse load file" << filePath;
        return false;
    }

    this->loadFromJson(loadDoc.object());
    return true;
}

bool JsonSettingsBase::checkFile(const QString &filePath) {
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open load file" << filePath;
        return false;
    }

    const QByteArray saveData = loadFile.readAll();
    const QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    loadFile.close();

    if (loadDoc.isNull()) {
        qWarning() << "Couldn't parse load file" << filePath;
        return false;
    }

    return true;
}