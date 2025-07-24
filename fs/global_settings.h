//
// Created by Johma on 25/07/22.
//

#ifndef MANAGEMYSELF_GLOBAL_SETTINGS_H
#define MANAGEMYSELF_GLOBAL_SETTINGS_H

#include "JsonSettingsBase.h"
#include <QString>
#include <QSize>
#include <QObject>

class GlobalSettings final :public QObject,  public JsonSettingsBase {
    Q_OBJECT

    Q_PROPERTY(int windowWidth READ getWindowWidth NOTIFY windowSizeChanged)
    Q_PROPERTY(int windowHeight READ getWindowHeight NOTIFY windowSizeChanged)

public:
    explicit GlobalSettings(QObject *parent = nullptr) : QObject(parent), language("en"), windowSize(800, 600) {}

    [[nodiscard]] int getWindowWidth() const {return windowSize.width();}
    [[nodiscard]] int getWindowHeight() const {return windowSize.height();}

signals:
    void windowSizeChanged();

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
            int width = windowSize.width();
            int height = windowSize.height();
            if (sizeObject.contains("width") && sizeObject["width"].isDouble())
            {
                width = sizeObject["width"].toInt(width);
            }
            if (sizeObject.contains("height") && sizeObject["height"].isDouble())
            {
                height = sizeObject["height"].toInt(height);
            }
            QSize newSize(width, height);
            if (newSize != windowSize)
            {
                windowSize = newSize;
                emit windowSizeChanged();
            }
        }
    }

   [[nodiscard]] QString getFilePath() const override;
};


#endif //MANAGEMYSELF_GLOBAL_SETTINGS_H
