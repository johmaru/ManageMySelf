#pragma once
//
// Created by Johma on 25/07/22.
//

#include <cstdint>
#ifndef MANAGEMYSELF_GLOBAL_SETTINGS_H
#define MANAGEMYSELF_GLOBAL_SETTINGS_H

#include "JsonSettingsBase.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaType>
#include <QObject>
#include <QQmlEngine>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <array>
#include <qcontainerfwd.h>
#include <qcoreapplication.h>
#include <qdebug.h>
#include <qdir.h>
#include <qjsondocument.h>
#include <qqmlengine.h>
#include <qtmetamacros.h>
#include <qtranslator.h>

class GlobalSettings final : public QObject, public JsonSettingsBase {
    Q_OBJECT

    Q_PROPERTY(int windowWidth READ getWindowWidth WRITE setWindowWidth NOTIFY windowSizeChanged)
    Q_PROPERTY(int windowHeight READ getWindowHeight WRITE setWindowHeight NOTIFY windowSizeChanged)
    Q_PROPERTY(QString theme READ getTheme WRITE setTheme NOTIFY themeChanged)
    Q_PROPERTY(QString language READ getLanguage WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString activityBarTheme READ getActivityBarTheme WRITE setActivityBarTheme NOTIFY
                   activityBarThemeChanged)
    Q_PROPERTY(bool isAnyItemCreatedAfterOpening READ getIsAnyItemCreatedAfterOpening WRITE
                   setIsAnyItemCreatedAfterOpening NOTIFY isAnyItemCreatedAfterOpeningChanged)

  public slots:
    Q_INVOKABLE int createWorkspaceFromQml(const QString& userName, const QString& name,
                                           const QString& path);
    Q_INVOKABLE [[nodiscard]] QStringList getWorkspaceWithName(const QString& name) const;
    Q_INVOKABLE [[nodiscard]] QUrl createDiary(int year, int month, int day, const QString& title,
                                               const QString& path) const;
    Q_INVOKABLE [[nodiscard]] int createStatus(const QString& path) const;
    Q_INVOKABLE [[nodiscard]] int editStatus(int year, int month, int day,
                                             const QString& jsonString, const QString& path) const;
    Q_INVOKABLE [[nodiscard]] QString getMonthUserDiarySqlData(int year, int month,
                                                               const QString& path) const;
    Q_INVOKABLE [[nodiscard]] QString getMonthUserStatusData(int year, int month,
                                                             const QString& path) const;
    Q_INVOKABLE [[nodiscard]] QString getSelectedUserStatusDataJson(int year, int month, int day,
                                                                    const QString& path) const;
    Q_INVOKABLE [[nodiscard]] QString loadMarkdownFile(const QString& filePath) const;
    Q_INVOKABLE [[nodiscard]] int writeMarkdownFile(const QString& filePath,
                                                    const QString& content) const;
    Q_INVOKABLE [[nodiscard]] QString search(const QString& query, const QString& scope,
                                             bool caseSensitive, bool useRegex,
                                             const QString& path) const;
    Q_INVOKABLE int openGraphWindow(const QString& workspacePath, int scope, int filter,
                                    const QString& toStr, const QString& fromStr);
    [[nodiscard]] Q_INVOKABLE QVariant getGraphData(const QString& workspacePath, int scope,
                                                    int filter, const QString& toStr,
                                                    const QString& fromStr) const;

    Q_INVOKABLE [[nodiscard]] QStringList getSettings(const QString& path) const;

    // NOLINTNEXTLINE(readability-redundant-access-specifiers)
  public:
    explicit GlobalSettings(QObject* parent = nullptr)
        : QObject(parent), m_language("en"), m_windowSize(800, 600), m_theme("light"),
          m_ab_theme("default") {}

    [[nodiscard]] int getWindowWidth() const {
        return m_windowSize.width();
    }
    [[nodiscard]] int getWindowHeight() const {
        return m_windowSize.height();
    }
    [[nodiscard]] QString getTheme() const {
        return m_theme;
    }
    [[nodiscard]] QString getLanguage() const {
        return m_language;
    }
    [[nodiscard]] QString getActivityBarTheme() const {
        return m_ab_theme;
    }
    [[nodiscard]] bool getIsAnyItemCreatedAfterOpening() const {
        return m_isAnyItemCreatedAfterOpening;
    }

    enum class WorkspaceResult : std::int8_t {
        SUCCESS = 0,
        FAILED_TO_CREATE_DIRECTORY = -1,
        ALREADY_EXISTS = -2,
        ERROR_CHECKING_EXISTENCE = -3,
        WORKSPACE_NAME_CANNOT_BE_EMPTY = -4,
        INSUFFICIENT_ARGUMENTS = -5,
        WORKSPACE_DOES_NOT_EXIST = -6,
        WORK_SPACE_NOT_FOUND = -7,
    };

    void initialize(QQmlEngine* engine) {
        m_engine = engine;
        loadTranslation(m_language);
    }

    Q_INVOKABLE [[nodiscard]] QStringList getWorkspaces() const;
    Q_INVOKABLE [[nodiscard]] int deleteWorkspace(const QString& name) const;
    Q_INVOKABLE [[nodiscard]] int openWorkspace(const QString& path) const;

    Q_INVOKABLE void setTheme(const QString& newTheme) {
        if (m_theme != newTheme) {
            m_theme = newTheme;

            if (newTheme == "dark") {
                qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Dark");
            } else {
                qputenv("QT_QUICK_CONTROLS_MATERIAL_THEME", "Light");
            }

            if (m_engine != nullptr) {
                m_engine->clearComponentCache();
            }

            if (!m_loading) {
                QString file_path = this->getFilePath();
                QFile save_file(file_path);
                if (!save_file.open(QIODevice::WriteOnly)) {
                    qWarning() << "Couldn't open settings file for writing:" << file_path;
                    return;
                }
                save_file.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                save_file.close();
            }

            emit themeChanged();
        }
    }

    Q_INVOKABLE void setLanguage(const QString& newLanguage) {
        if (m_language != newLanguage) {
            m_language = newLanguage;

            loadTranslation(m_language);

            if (!m_loading) {
                QString file_path = this->getFilePath();
                QFile save_file(file_path);
                if (save_file.open(QIODevice::WriteOnly)) {
                    save_file.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                    save_file.close();
                } else {
                    qWarning() << "Couldn't open settings file for writing:" << file_path;
                }
            }

            emit languageChanged();
        }
    }

    int createWorkspace(const QStringList& items) const;

    Q_INVOKABLE void setActivityBarTheme(const QString& newTheme) {
        if (m_ab_theme != newTheme) {
            m_ab_theme = newTheme;

            if (!m_loading) {
                QString file_path = this->getFilePath();
                QFile save_file(file_path);
                if (!save_file.open(QIODevice::WriteOnly)) {
                    qWarning() << "Couldn't open settings file for writing:" << file_path;
                    return;
                }
                save_file.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                save_file.close();
            }

            emit activityBarThemeChanged();
        }
    }

    Q_INVOKABLE void setWindowWidth(const int WIDTH) {
        if (m_windowSize.width() == WIDTH) {
            return;
        }
        m_windowSize.setWidth(WIDTH);
        if (!m_loading) {
            const QString FILE_PATH = this->getFilePath();
            QFile save_file(FILE_PATH);
            if (!save_file.open(QIODevice::WriteOnly)) {
                qWarning() << "Couldn't open settings file for writing:" << FILE_PATH;
            } else {
                save_file.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                save_file.close();
            }
        }
        emit windowSizeChanged();
    }

    Q_INVOKABLE void setWindowHeight(const int HEIGHT) {
        if (m_windowSize.height() == HEIGHT) {
            return;
        }
        m_windowSize.setHeight(HEIGHT);
        if (!m_loading) {
            const QString FILE_PATH = this->getFilePath();
            QFile save_file(FILE_PATH);
            if (!save_file.open(QIODevice::WriteOnly)) {
                qWarning() << "Couldn't open settings file for writing:" << FILE_PATH;
            } else {
                save_file.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                save_file.close();
            }
        }
        emit windowSizeChanged();
    }

    Q_INVOKABLE void setIsAnyItemCreatedAfterOpening(const bool VALUE) {
        if (!m_loading) {
            m_isAnyItemCreatedAfterOpening = VALUE;
            QString file_path = this->getFilePath();
            QFile save_file(file_path);
            if (save_file.open(QIODevice::WriteOnly)) {
                save_file.write(QJsonDocument(this->toJson()).toJson(QJsonDocument::Indented));
                save_file.close();
            } else {
                qWarning() << "Couldn't open settings file for writing:" << file_path;
            }
        }

        emit isAnyItemCreatedAfterOpeningChanged();
    }

  signals:
    void windowSizeChanged();
    void isAnyItemCreatedAfterOpeningChanged();
    void themeChanged();
    void languageChanged();
    void workspaceCreated(const QString& userName, const QString& name, const QString& path,
                          int result);
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

    static QVariant getLanguageVar(const GlobalSettings& settings) {
        return settings.m_language;
    }
    static void setLanguageVar(GlobalSettings& settings, const QVariant& variant) {
        settings.setLanguage(variant.toString());
    }
    static QVariant defLanguage() {
        return QVariant{QStringLiteral("en")};
    }

    static QVariant getThemeVar(const GlobalSettings& settings) {
        return settings.m_theme;
    }
    static void setThemeVar(GlobalSettings& settings, const QVariant& variant) {
        settings.setTheme(variant.toString());
    }
    static QVariant defTheme() {
        return QVariant{QStringLiteral("light")};
    }

    static QVariant getActivityBarThemeVar(const GlobalSettings& settings) {
        return settings.m_ab_theme;
    }
    static void setActivityBarThemeVar(GlobalSettings& settings, const QVariant& variant) {
        settings.m_ab_theme = variant.toString();
    }
    static QVariant defActivityBarTheme() {
        return QVariant{QStringLiteral("default")};
    }

    static QVariant getWindowSizeVar(const GlobalSettings& settings) {
        return QVariant::fromValue(settings.m_windowSize);
    }
    static void setWindowSizeVar(GlobalSettings& settings, const QVariant& variant) {
        const QSize SIZE = variant.canConvert<QSize>() ? variant.toSize() : QSize(800, 600);
        if (SIZE != settings.m_windowSize) {
            settings.m_windowSize = SIZE;
            emit settings.windowSizeChanged();
        }
    }
    static QVariant defWindowSize() {
        return QVariant::fromValue(QSize(800, 600));
    }

    static QVariant getIsAnyItemCreatedAfterOpeningVar(const GlobalSettings& settings) {
        return {settings.m_isAnyItemCreatedAfterOpening};
    }

    static void setIsAnyItemCreatedAfterOpeningVar(GlobalSettings& settings,
                                                   const QVariant& variant) {
        settings.m_isAnyItemCreatedAfterOpening = variant.toBool();
    }

    static QVariant defIsAnyItemCreatedAfterOpening() {
        return {true};
    }

    static const std::array<FieldDesc, 5>& fields() {
        static const std::array<FieldDesc, 5> KEYS = {{
            {.key = "language",
             .typeId = QMetaType::QString,
             .get = getLanguageVar,
             .set = setLanguageVar,
             .def = defLanguage},
            {.key = "windowSize",
             .typeId = QMetaType::QSize,
             .get = getWindowSizeVar,
             .set = setWindowSizeVar,
             .def = defWindowSize},
            {.key = "theme",
             .typeId = QMetaType::QString,
             .get = getThemeVar,
             .set = setThemeVar,
             .def = defTheme},
            {.key = "activityBarTheme",
             .typeId = QMetaType::QString,
             .get = getActivityBarThemeVar,
             .set = setActivityBarThemeVar,
             .def = defActivityBarTheme},
            {.key = "isAnyItemCreatedAfterOpening",
             .typeId = QMetaType::Bool,
             .get = getIsAnyItemCreatedAfterOpeningVar,
             .set = setIsAnyItemCreatedAfterOpeningVar,
             .def = defIsAnyItemCreatedAfterOpening},
        }};
        return KEYS;
    }

    static QJsonValue variantToJson(const QVariant& variant) {
        if (variant.metaType().id() == QMetaType::QSize) {
            const QSize SIZE = variant.toSize();
            QJsonObject obj;
            obj["width"] = SIZE.width();
            obj["height"] = SIZE.height();
            return obj;
        }
        return QJsonValue::fromVariant(variant);
    }

    static QVariant jsonToVariant(const QJsonValue& jvalue, int typeId) {
        if (typeId == QMetaType::QSize) {
            const auto OBJ = jvalue.toObject();
            return QVariant::fromValue(
                QSize(OBJ.value("width").toInt(800), OBJ.value("height").toInt(600)));
        }
        return jvalue.toVariant();
    }

  public:
    [[nodiscard]] QJsonObject toJson() const override {
        QJsonObject json;
        for (const auto& fidesc : fields()) {
            json.insert(QString::fromUtf8(fidesc.key), variantToJson(fidesc.get(*this)));
        }
        return json;
    }

    int migrationJson(const QString FILEPATH) override {
        QFile file(FILEPATH);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qWarning() << "Couldn't open settings file for migration:" << FILEPATH;
            return -1;
        }

        const QByteArray SAVE_DATA = file.readAll();
        file.close();
        QJsonDocument load_doc = QJsonDocument::fromJson(SAVE_DATA);
        if (load_doc.isNull() || !load_doc.isObject()) {
            qWarning() << "Couldn't parse settings file for migration:" << FILEPATH;
            return -2;
        }

        QJsonObject json = load_doc.object();
        bool modified = false;

        for (const auto& fidesc : fields()) {
            if (!json.contains(fidesc.key)) {
                json.insert(QString::fromUtf8(fidesc.key), variantToJson(fidesc.def()));
                modified = true;
            }
        }

        if (modified) {
            QFile save_file(FILEPATH);
            if (!save_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                qWarning() << "Couldn't open settings file to write migration:" << FILEPATH;
                return -3;
            }
            save_file.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
            save_file.close();
        }

        return 0;
    }

    // This Function has side effects
    void loadFromJson(const QJsonObject& json) override {
        QJsonObject mutable_json = json;
        bool was_modified = false;

        for (const auto& fidesc : fields()) {
            if (!mutable_json.contains(fidesc.key)) {
                mutable_json.insert(QString::fromUtf8(fidesc.key), variantToJson(fidesc.def()));
                was_modified = true;
            }
        }

        m_loading = true;
        for (const auto& fidesc : fields()) {
            const QJsonValue JVALUE = mutable_json.value(fidesc.key);
            const QVariant VARIANT = jsonToVariant(JVALUE, fidesc.typeId);
            fidesc.set(*this, VARIANT);
        }
        m_loading = false;

        if (was_modified) {
            qInfo() << "Settings file was outdated or incomplete. Updating it now...";
            QFile save_file(this->getFilePath());
            if (save_file.open(QIODevice::WriteOnly)) {
                save_file.write(QJsonDocument(mutable_json).toJson(QJsonDocument::Indented));
                save_file.close();
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
    std::unique_ptr<QTranslator> m_translator;
    QQmlEngine* m_engine = nullptr;
    QString m_ab_theme;
    bool m_loading = false;
    bool m_isAnyItemCreatedAfterOpening = false;

    void loadTranslation(const QString& language) {
        if (m_translator) {
            QCoreApplication::removeTranslator(m_translator.get());
            m_translator.reset();
        }

        m_translator = std::make_unique<QTranslator>(this);
        QString translation_file = QString(":/i18n/ManageMySelf_%1.qm").arg(language);

        if (QFile::exists(translation_file)) {
            if (m_translator->load(translation_file)) {
                QCoreApplication::installTranslator(m_translator.get());
                qDebug() << "Loaded translation file:" << translation_file;

                if (m_engine != nullptr) {
                    m_engine->retranslate();
                }

            } else {
                qDebug() << "Failed to load translation file:" << translation_file;
            }
        } else {
            qDebug() << "Translation file does not exist:" << translation_file;
        }
    }
};

#endif // MANAGEMYSELF_GLOBAL_SETTINGS_H
