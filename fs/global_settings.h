//
// Created by Johma on 25/07/22.
//

#ifndef MANAGEMYSELF_GLOBAL_SETTINGS_H
#define MANAGEMYSELF_GLOBAL_SETTINGS_H

#include <QFile>

#include "JsonSettingsBase.h"
#include <QString>
#include <QSize>
#include <QObject>

class GlobalSettings final :public QObject,  public JsonSettingsBase {
    Q_OBJECT

    Q_PROPERTY(int windowWidth READ getWindowWidth NOTIFY windowSizeChanged)
    Q_PROPERTY(int windowHeight READ getWindowHeight NOTIFY windowSizeChanged)
    Q_PROPERTY(QString theme READ getTheme WRITE setTheme NOTIFY themeChanged)

public:
    explicit GlobalSettings(QObject *parent = nullptr) : QObject(parent), m_language("en"), m_windowSize(800, 600), m_theme("light") {}

    [[nodiscard]] int getWindowWidth() const {return m_windowSize.width();}
    [[nodiscard]] int getWindowHeight() const {return m_windowSize.height();}
    [[nodiscard]] QString getTheme() const {return m_theme;}
    [[nodiscard]] QString getLanguage() const {return m_language;}

    void setTheme(const QString &newTheme)
    {
        if (m_theme != newTheme)
        {
            m_theme = newTheme;
            emit themeChanged();
        }
    }

signals:
    void windowSizeChanged();
    void themeChanged();

public:

    inline static const QString SETTINGS_DIR_NAME = "ManageMySelf";

    [[nodiscard]] QJsonObject toJson() const override {
        QJsonObject json;
        json["language"] = m_language;
        QJsonObject sizeObject;
        sizeObject["width"] = m_windowSize.width();
        sizeObject["height"] = m_windowSize.height();
        json["windowSize"] = sizeObject;
        json["theme"] = m_theme;
        return json;
    }

    // This Function has side effects
    void loadFromJson(const QJsonObject &json) override {
        QJsonObject mutableJson = json;
        bool wasModified = false;

        if (!mutableJson.contains("language") || !mutableJson["language"].isString()) {
            mutableJson["language"] = "en";
            wasModified = true;
        }

        if (!mutableJson.contains("windowSize") || !mutableJson["windowSize"].isObject()) {
            QJsonObject sizeObject;
            sizeObject["width"] = 800;
            sizeObject["height"] = 600;
            mutableJson["windowSize"] = sizeObject;
            wasModified = true;
        }

        if (!mutableJson.contains("theme") || !mutableJson["theme"].isString()) {
            mutableJson["theme"] = "light";
            wasModified = true;
        }

        if (wasModified) {
            qInfo() << "Settings file was outdated or incomplete. Updating it now...";
            QFile saveFile(this->getFilePath());
            if (saveFile.open(QIODevice::WriteOnly)) {
                saveFile.write(QJsonDocument(mutableJson).toJson(QJsonDocument::Indented));
                saveFile.close();
            } else {
                qWarning() << "Could not update settings file:" << this->getFilePath();
            }
        }

        m_language = mutableJson["language"].toString();

        QJsonObject sizeObject = mutableJson["windowSize"].toObject();

        int width = sizeObject.value("width").toInt(800);
        int height = sizeObject.value("height").toInt(600);
        QSize newSize(width, height);
        if (newSize != m_windowSize) {
            m_windowSize = newSize;
            emit windowSizeChanged();
        }

        setTheme(mutableJson["theme"].toString());
    }

   [[nodiscard]] QString getFilePath() const override;

private:
    QString m_language;
    QSize m_windowSize;
    QString m_theme;
};


#endif //MANAGEMYSELF_GLOBAL_SETTINGS_H
