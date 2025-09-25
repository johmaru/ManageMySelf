#pragma once
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
#include <cstddef>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qjsondocument.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qtranslator.h>
#include <QQmlEngine>
#include <array>
#include <QMetaType>
#include <QVariant>
#include <QJsonValue>
#include <QJsonObject>

class GlobalSettings final : public QObject, public JsonSettingsBase {
    Q_OBJECT

    Q_PROPERTY(int windowWidth READ getWindowWidth NOTIFY windowSizeChanged)
    Q_PROPERTY(int windowHeight READ getWindowHeight NOTIFY windowSizeChanged)
    Q_PROPERTY(QString theme READ getTheme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString activityBarTheme READ getActivityBarTheme WRITE setActivityBarTheme NOTIFY activityBarThemeChanged)

public slots:
    Q_INVOKABLE int createWorkspaceFromQml(const QString &userName, const QString &name, const QString &path);
    Q_INVOKABLE QStringList getWorkspaceWithName(const QString &name) const;
    Q_INVOKABLE int createDiary(int year, int month, int day, const QString &title, const QString &path) const;
    Q_INVOKABLE int createStatus(const QString &path) const;
    Q_INVOKABLE int editStatus(int year, int month, int day, const QString &jsonString, const QString &path) const;
    Q_INVOKABLE QString getMonthUserDiarySqlData(int year, int month, const QString &path) const;
    Q_INVOKABLE QString getMonthUserStatusData(int year, int month, const QString &path) const;
    Q_INVOKABLE QString loadMarkdownFile(const QString &filePath) const;
    Q_INVOKABLE int writeMarkdownFile(const QString &filePath, const QString &content) const;
    Q_INVOKABLE QString search(const QString& query, const QString& scope, bool casseSensitive, bool useRegex, const QString& path) const;
    Q_INVOKABLE int openGraphWindow(const QString& workspacePath, int scope, int filter, const QString& toStr, const QString& fromStr);
	Q_INVOKABLE [[nodiscard]]QVariant getGraphData(const QString& workspacePath, int scope, int filter, const QString& toStr, const QString& fromStr) const;

    Q_INVOKABLE QStringList getSettings(const QString &path) const;

public:
    explicit GlobalSettings(QObject *parent = nullptr)
        : QObject(parent),
          m_language("en"),
          m_windowSize(800, 600),
          m_theme("light"),
          m_ab_theme("default") {}

    [[nodiscard]] int getWindowWidth() const { return m_windowSize.width(); }
    [[nodiscard]] int getWindowHeight() const { return m_windowSize.height(); }
    [[nodiscard]] QString getTheme() const { return m_theme; }
    [[nodiscard]] QString getLanguage() const { return m_language; }
    [[nodiscard]] QString getActivityBarTheme() const { return m_ab_theme; }

    enum class WorkspaceResult {
        Success = 0,
        FailedToCreateDirectory = -1,
        AlreadyExists = -2,
        ErrorCheckingExistence = -3,
        WorkspaceNameCannotBeEmpty = -4,
        InsufficientArguments = -5,
        WorkspaceDoesNotExist = -6,
        WorkSpaceNotFound = -7,
    };

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

            if (!m_loading) {
                QString filePath = this->getFilePath();
                QFile saveFile(filePath);
                if (!saveFile.open(QIODevice::WriteOnly)) {
                    qWarning() << "Couldn't open settings file for writing:" << filePath;
                    return;
                }
                saveFile.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                saveFile.close();
            }

            emit themeChanged();
        }
    }

    Q_INVOKABLE void setLanguage(const QString &newLanguage)
    {
        if (m_language != newLanguage)
        {
            m_language = newLanguage;

            loadTranslation(m_language);

            if (!m_loading) {
                QString filePath = this->getFilePath();
                QFile saveFile(filePath);
                if (saveFile.open(QIODevice::WriteOnly)) {
                    saveFile.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                    saveFile.close();
                } else {
                    qWarning() << "Couldn't open settings file for writing:" << filePath;
                }
            }

            emit languageChanged();
        }
    }

    int createWorkspace(const QStringList &items) const;

    Q_INVOKABLE void setActivityBarTheme(const QString &newTheme)
    {
        if (m_ab_theme != newTheme)
        {
            m_ab_theme = newTheme;

            if (!m_loading) {
                QString filePath = this->getFilePath();
                QFile saveFile(filePath);
                if (!saveFile.open(QIODevice::WriteOnly)) {
                    qWarning() << "Couldn't open settings file for writing:" << filePath;
                    return;
                }
                saveFile.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                saveFile.close();
            }

            emit activityBarThemeChanged();
        }
    }

signals:
    void windowSizeChanged();
    void themeChanged();
    void languageChanged();
    void workspaceCreated(const QString &userName, const QString &name, const QString &path, int result);
    void activityBarThemeChanged();

public:
    inline static const QString SETTINGS_DIR_NAME = "ManageMySelf";

private:
    struct FieldDesc {
        const char* key;
        int typeId;
        QVariant (*get)(const GlobalSettings&);
        void (*set)(GlobalSettings&, const QVariant&);
        QVariant (*def)();
    };

    static QVariant getLanguageVar(const GlobalSettings& s) { return s.m_language; }
    static void setLanguageVar(GlobalSettings& s, const QVariant& v) { s.setLanguage(v.toString()); }
    static QVariant defLanguage() { return QVariant(QStringLiteral("en")); }

    static QVariant getThemeVar(const GlobalSettings& s) { return s.m_theme; }
    static void setThemeVar(GlobalSettings& s, const QVariant& v) { s.setTheme(v.toString()); }
    static QVariant defTheme() { return QVariant(QStringLiteral("light")); }

    static QVariant getActivityBarThemeVar(const GlobalSettings& s) { return s.m_ab_theme; }
    static void setActivityBarThemeVar(GlobalSettings& s, const QVariant& v) { s.m_ab_theme = v.toString(); }
    static QVariant defActivityBarTheme() { return QVariant(QStringLiteral("default")); }

    static QVariant getWindowSizeVar(const GlobalSettings& s) { return QVariant::fromValue(s.m_windowSize); }
    static void setWindowSizeVar(GlobalSettings& s, const QVariant& v) {
        const QSize size = v.canConvert<QSize>() ? v.toSize() : QSize(800, 600);
        if (size != s.m_windowSize) {
            s.m_windowSize = size;
            emit s.windowSizeChanged();
        }
    }
    static QVariant defWindowSize() { return QVariant::fromValue(QSize(800, 600)); }

    static const std::array<FieldDesc, 4>& fields() {
        static const std::array<FieldDesc, 4> k = {{
            { "language",          QMetaType::QString, getLanguageVar,        setLanguageVar,        defLanguage },
            { "windowSize",        QMetaType::QSize,   getWindowSizeVar,      setWindowSizeVar,      defWindowSize },
            { "theme",             QMetaType::QString, getThemeVar,           setThemeVar,           defTheme },
            { "activityBarTheme",  QMetaType::QString, getActivityBarThemeVar,setActivityBarThemeVar,defActivityBarTheme },
        }};
        return k;
    }

    static QJsonValue variantToJson(const QVariant& v) {
        if (v.metaType().id() == QMetaType::QSize) {
            const QSize sz = v.toSize();
            QJsonObject o;
            o["width"] = sz.width();
            o["height"] = sz.height();
            return o;
        }
        return QJsonValue::fromVariant(v);
    }

    static QVariant jsonToVariant(const QJsonValue& jv, int typeId) {
        if (typeId == QMetaType::QSize) {
            const auto o = jv.toObject();
            return QVariant::fromValue(QSize(o.value("width").toInt(800),
                                             o.value("height").toInt(600)));
        }
        return jv.toVariant();
    }

public:
    [[nodiscard]] QJsonObject toJson() const override {
        QJsonObject json;
        for (const auto& f : fields()) {
            json.insert(QString::fromUtf8(f.key), variantToJson(f.get(*this)));
        }
        return json;
    }

    int migrationJson(const QString filepath) override {
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Couldn't open settings file for migration:" << filepath;
            return -1;
        }

        const QByteArray saveData = file.readAll();
        file.close();
        QJsonDocument loadDoc = QJsonDocument::fromJson(saveData);
        if (loadDoc.isNull() || !loadDoc.isObject()) {
            qWarning() << "Couldn't parse settings file for migration:" << filepath;
            return -2;
        }

        QJsonObject json = loadDoc.object();
        bool modified = false;

        for (const auto& f : fields()) {
            if (!json.contains(f.key)) {
                json.insert(QString::fromUtf8(f.key), variantToJson(f.def()));
                modified = true;
            }
        }

        if (modified) {
            QFile saveFile(filepath);
            if (!saveFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qWarning() << "Couldn't open settings file to write migration:" << filepath;
                return -3;
            }
            saveFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
            saveFile.close();
        }

        return 0;
    }

    // This Function has side effects
    void loadFromJson(const QJsonObject &json) override {
        QJsonObject mutableJson = json;
        bool wasModified = false;

        for (const auto& f : fields()) {
            if (!mutableJson.contains(f.key)) {
                mutableJson.insert(QString::fromUtf8(f.key), variantToJson(f.def()));
                wasModified = true;
            }
        }

        m_loading = true;
        for (const auto& f : fields()) {
            const QJsonValue jv = mutableJson.value(f.key);
            const QVariant v = jsonToVariant(jv, f.typeId);
            f.set(*this, v);
        }
        m_loading = false;

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
    }

    [[nodiscard]] QString getFilePath() const override;

private:
    QString m_language;
    QSize m_windowSize;
    QString m_theme;
    QTranslator *m_translator = nullptr;
    QQmlEngine *m_engine = nullptr;
    QString m_ab_theme;
    bool m_loading = false;

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

#endif // MANAGEMYSELF_GLOBAL_SETTINGS_H
