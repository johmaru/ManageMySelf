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

class GlobalSettings final : public JsonSettingsBase {
public:
    GlobalSettings() : language("en"), windowSize(800, 600) {}

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
        if (json.contains("language") && json["language"].isString())
        {
            language = json["language"].toString();
        } else
        {
            language = "en";
        }

        if (json.contains("windowSize") && json["windowSize"].isObject())
        {
            QJsonObject sizeObject = json["windowSize"].toObject();
            int width = 800;
            int height = 600;
            if (sizeObject.contains("width") && sizeObject["width"].isDouble())
            {
                width = sizeObject["width"].toInt(width);
            }
            if (sizeObject.contains("height") && sizeObject["height"].isDouble())
            {
                height = sizeObject["height"].toInt(height);
            } else
            {
                windowSize = QSize(width, height);
            }
        }
    }

   [[nodiscard]] QString getFilePath() const override;
};


#endif //MANAGEMYSELF_GLOBAL_SETTINGS_H
