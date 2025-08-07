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
#include <QStringList>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qtranslator.h>
#include <QQmlEngine>

class GlobalSettings final :public QObject,  public JsonSettingsBase {
    Q_OBJECT

    Q_PROPERTY(int windowWidth READ getWindowWidth NOTIFY windowSizeChanged)
    Q_PROPERTY(int windowHeight READ getWindowHeight NOTIFY windowSizeChanged)
    Q_PROPERTY(QString theme READ getTheme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY languageChanged)

public slots:
    Q_INVOKABLE int createWorkspaceFromQml(const QString &userName, const QString &name, const QString &path);
    Q_INVOKABLE QStringList getWorkspaceWithName(const QString &name) const;
    Q_INVOKABLE int createDiary(const QString &title, const QString &path) const;
    Q_INVOKABLE QString getMonthUserDiarySqlData(int year, int month, const QString &path) const;

    // --- ここからユーザーワークスペースの関数 ---
    Q_INVOKABLE QStringList getSettings(const QString &path) const;

public:
    explicit GlobalSettings(QObject *parent = nullptr) : QObject(parent), m_language("en"), m_windowSize(800, 600), m_theme("light") {}

    [[nodiscard]] int getWindowWidth() const {return m_windowSize.width();}
    [[nodiscard]] int getWindowHeight() const {return m_windowSize.height();}
    [[nodiscard]] QString getTheme() const {return m_theme;}
    [[nodiscard]] QString getLanguage() const {return m_language;}

    void initialize(QQmlEngine *engine) {
        m_engine = engine;
        loadTranslation(m_language);
    }

    Q_INVOKABLE QStringList getWorkspaces() const;

    Q_INVOKABLE int deleteWorkspace(const QString &name) const;

    Q_INVOKABLE int openWorkspace(const QString &path) const;


    Q_INVOKABLE void setTheme(const QString &newTheme)
    {
        if (m_theme != newTheme)
        {
            m_theme = newTheme;

            if (newTheme == "dark") {
                qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");
            } else {
                qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Light");
            }

            if (m_engine) {
                m_engine->clearComponentCache();
            }

            QString filePath = this->getFilePath();
            QFile saveFile(filePath);
            if (!saveFile.open(QIODevice::WriteOnly)) {
                qWarning() << "Couldn't open settings file for writing:" << filePath;
                return;
            }
            saveFile.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
            saveFile.close();
            
            emit themeChanged();
        }
    }

    Q_INVOKABLE void setLanguage(const QString &newLanguage)
    {
        if (m_language != newLanguage)
        {
            m_language = newLanguage;

            loadTranslation(m_language);

            QString filePath = this->getFilePath();
            QFile saveFile(filePath);
            if (saveFile.open(QIODevice::WriteOnly)) {
                saveFile.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                saveFile.close();
            } else {
                qWarning() << "Couldn't open settings file for writing:" << filePath;
            }

            emit languageChanged();
        }
    }

    int createWorkspace(const QStringList &items) const;

signals:
    void windowSizeChanged();
    void themeChanged();
    void languageChanged();
    void workspaceCreated(const QString &userName, const QString &name, const QString &path, int result);

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
    QTranslator *m_translator = nullptr;
    QQmlEngine *m_engine = nullptr;

    void loadTranslation(const QString &language) {
        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator);
            delete m_translator;
            m_translator = nullptr;
        }

        m_translator = new QTranslator(this);
        QString translationFile = QString(":/i18n/ManageMySelf_%1.qm").arg(language);

        if (QFile::exists(translationFile)) {
            if (m_translator->load(translationFile)) {
                QCoreApplication::installTranslator(m_translator);
                qDebug() << "Loaded translation file:" << translationFile;

                if (m_engine) {
                    m_engine->retranslate();
                }

            } else {
                qDebug() << "Failed to load translation file:" << translationFile;
            }
        } else {
            qDebug() << "Translation file does not exist:" << translationFile;
        }
    }
};

#endif //MANAGEMYSELF_GLOBAL_SETTINGS_H
