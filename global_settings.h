//
// Created by Johma on 25/07/22.
//

#ifndef MANAGEMYSELF_GLOBAL_SETTINGS_H
#define MANAGEMYSELF_GLOBAL_SETTINGS_H

#include "JsonSettingsBase.h"
#include <Qstring>
#include <QSize>
#include <QStandardPaths>
#include <QDir>

class GlobalSettings : public JsonSettingsBase {
public:
    QString language;
    QSize windowSize;

    [[nodiscard]] QJsonObject toJson() const override {
        QJsonObject json;
        json["language"] = language;
        QJsonObject sizeObject;
        sizeObject["width"] = windowSize.width();
        sizeObject["height"] = windowSize.height();
        json["windowSize"] = sizeObject;
        return json;
    }

    void loadFromJson(const QJsonObject &json) override {
        language = json["language"].toString();
        QJsonObject sizeObject = json["windowSize"].toObject();
        windowSize.setWidth(sizeObject["width"].toInt());
        windowSize.setHeight(sizeObject["height"].toInt());
    }

   [[nodiscard]] QString getFilePath() const override;
};


#endif //MANAGEMYSELF_GLOBAL_SETTINGS_H
