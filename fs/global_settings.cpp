//
// Created by Johma on 25/07/22.
//

#include "../fs/global_settings.h"

#include "SqLiteBase.h"
#include "UserSql.h"
#include "fs/AppMigrations.h"
#include "fs/Migration.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QStandardPaths>
#include <QWidget>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qobject.h>
#include <qwindow.h>

const QString SETTINGS_FILE_NAME = "settings.json";

// 呼び出すと現在定義されている 'GlobalSettings'にDocumentのディレクトリパスがセットされる。
// IO副作用として、'SETTINGS_DIR_NAME'のディレクトリが特殊パスDocumentに存在しない場合に、ディレクトリが作成される。
QString GlobalSettings::getFilePath() const {
    const QString DOCUMENTS_PATH =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

    if (DOCUMENTS_PATH.isEmpty()) {
        qWarning() << "Cant get the path for document folder";
        return "";
    }

    QDir settings_dir(DOCUMENTS_PATH);
    if (!settings_dir.exists(SETTINGS_DIR_NAME)) {
        qInfo() << "Currently creating directory..." << settings_dir.filePath(SETTINGS_DIR_NAME);
        if (!settings_dir.mkdir(SETTINGS_DIR_NAME)) {
            qWarning() << "Cant create directory" << settings_dir.filePath(SETTINGS_DIR_NAME);
            return "";
        }
    }

    settings_dir.cd(SETTINGS_DIR_NAME);

    QString file_path = settings_dir.filePath(SETTINGS_FILE_NAME);
    qInfo() << "Setting Path : " << file_path;

    return file_path;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QStringList GlobalSettings::getWorkspaces() const {
    SqLiteBase sqlite;
    return sqlite.getWorkspaces();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QStringList GlobalSettings::getWorkspaceWithName(const QString& name) const {
    SqLiteBase sqlite;
    return sqlite.getWorkspaceWithName(name);
}

int GlobalSettings::createWorkspaceFromQml(const QString& userName, const QString& name,
                                           const QString& path) {
    if (name.isEmpty()) {
        qWarning() << "Workspace name cannot be empty";
        return -1;
    }

    if (path.isEmpty()) {
        qWarning() << "Workspace path cannot be empty";
        return -1;
    }

    QStringList items;
    items << userName << name << path;

    int result = createWorkspace(items);

    emit workspaceCreated(userName, name, path, result);

    return result;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int GlobalSettings::createWorkspace(const QStringList& items) const {
    if (items.size() < 3) {
        qWarning()
            << "Insufficient arguments for workspace creation. Expected: userName, name, path";
        return -1;
    }

    const QString& user_name = items.at(0);
    const QString& workspace_name = items.at(1);
    const QString& workspace_path = items.at(2);

    qInfo() << "Creating workspace:" << workspace_name << "at" << workspace_path;

    QString full_path = QDir(workspace_path).filePath(workspace_name);

    if (!QDir().mkpath(full_path)) {
        qWarning() << "Failed to create workspace directory:" << full_path;
        return -1;
    }

    QString db_path = QDir(full_path).filePath("user.db");

    UserSql user_sql(db_path);
    int result_create_user_db = user_sql.createUserDatabase(full_path);
    if (result_create_user_db != 0) {
        qWarning() << "Failed to create user database at:" << full_path
                   << "Error code:" << result_create_user_db;
        return result_create_user_db; // エラーコードを返す
    }
    qInfo() << "User database created successfully at:" << full_path;

    // ここで設定を作成 & 保存

    QJsonObject settings_json;
    settings_json["userName"] = user_name;

    GlobalSettings settings;
    // NOLINTNEXTLINE(readability-identifier-length)
    int rc =
        GlobalSettings::saveToFileAny(QDir(full_path).filePath("settings.json"), settings_json);
    if (rc != 0) {
        qWarning() << "Failed to create settings file at:"
                   << QDir(full_path).filePath("settings.json");
    } else {
        qInfo() << "Settings file created at:" << QDir(full_path).filePath("settings.json");
    }

    qInfo() << "Workspace created successfully at:" << full_path;

    // NOLINTNEXTLINE(readability-identifier-length)
    SqLiteBase db;

    int exists = db.existCheckWorkspaceAtName(workspace_name);
    if (exists > 0) {
        qWarning() << "Workspace with name" << workspace_name << "already exists.";
        return -2; // Workspace already exists
    }
    if (exists < 0) {
        qWarning() << "Error checking workspace existence:" << exists;
        return -3; // Error checking existence
    }

    int result = db.addWorkspace(workspace_name, full_path);
    if (result == 0) {
        qInfo() << "Workspace added to database successfully.";
    } else {
        qWarning() << "Failed to add workspace to database. Error code:" << result;
    }

    return 0;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int GlobalSettings::deleteWorkspace(const QString& name) const {
    // NOLINTNEXTLINE(readability-identifier-length)
    SqLiteBase db;
    int exists = db.existCheckWorkspaceAtName(name);
    if (exists <= 0) {
        qWarning() << "Workspace with name" << name << "does not exist or an error occurred.";
        return -1; // ワークスペースがない場合
    }

    QStringList workspace_info = db.getWorkspaceWithName(name);
    if (workspace_info.isEmpty()) {
        qWarning() << "No workspace found with name:" << name;
        return -2; // ワークスペースが見つからない場合
    }

    const QString& path = workspace_info.at(1);
    QDir dir(path);
    if (!dir.removeRecursively()) {
        qWarning() << "Failed to delete workspace directory at:" << path;
        return -3; // ディレクトリの削除に失敗した場合
    }

    int result = db.deleteWorkspace(name);
    if (result != 0) {
        qWarning() << "Failed to remove workspace from database. Error code:" << result;
        return -4; // データベースから削除に失敗した場合
    }

    qInfo() << "Workspace deleted successfully:" << name;
    return 0; // 成功
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int GlobalSettings::openWorkspace(const QString& path) const {
    if (path.isEmpty()) {
        qWarning() << "Workspace path cannot be empty";
        return -1; // パスが空の場合のエラーコード
    }

    // NOLINTNEXTLINE(readability-identifier-length)
    SqLiteBase db;
    int exists = db.existCheckWorkspaceAtPath(path);
    if (exists > 0) {
        qInfo() << "Workspace exists at path:" << path;

        QString user_db_path = UserSql::getUserDatabasePath(path);
        if (user_db_path.isEmpty()) {
            qWarning() << "Failed to get user database path for workspace at:" << path;
            return -4; // ユーザーデータベースパスが取得できない場合のエラーコード
        }

        runMigrations(user_db_path.toStdString(), makeMigrations());

        return 0; // 成功
    }
    if (exists < 0) {
        qWarning() << "Error checking workspace existence:" << exists;
        return -2; // 存在チェックのエラーコード
    }

    qWarning() << "No workspace found at path:" << path;
    return -3; // ワークスペースが存在しない場合のエラーコード
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QStringList GlobalSettings::getSettings(const QString& path) const {
    if (path.isEmpty()) {
        qWarning() << "Path is empty. Cannot get settings.";
        return {}; // パスが空の場合は空のリストを返す
    }

    QDir dir(path);
    QString settings_file_path = dir.filePath("settings.json");

    if (!QFile::exists(settings_file_path)) {
        qWarning() << "Settings file does not exist at path:" << settings_file_path;
        return {}; // 設定ファイルが存在しない場合は空のリストを返す
    }

    QJsonObject json = JsonSettingsBase::loadFromFileAny(settings_file_path);
    if (json.isEmpty()) {
        qWarning() << "Failed to load settings from file:" << settings_file_path;
        return {}; // 設定の読み込みに失敗した場合は空のリストを返す
    }

    QStringList settings;
    settings << json["userName"].toString();

    return settings;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QUrl GlobalSettings::createDiary(int year, int month, int day, const QString& title,
                                 const QString& path) const {
    if (title.isEmpty() || path.isEmpty()) {
        qWarning() << "Title and path cannot be empty";
        return {}; // タイトルまたはパスが空
    }

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path for:" << path;
        return {}; // ユーザーデータベースパスの取得に失敗
    }

    UserSql user_sql(user_db_path);
    return user_sql.createDiary(year, month, day, title, path);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int GlobalSettings::createStatus(const QString& path) const {
    if (path.isEmpty()) {
        qWarning() << "Path cannot be empty";
        return -1; // パスが空の場合
    }

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path for:" << path;
        return -2; // ユーザーデータベースパスの取得に失敗
    }

    UserSql user_sql(user_db_path);
    return user_sql.createStatus();
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static,bugprone-easily-swappable-parameters)
int GlobalSettings::editStatus(int year, int month, int day, const QString& jsonString,
                               const QString& path) const {
    if (jsonString.isEmpty()) {
        qWarning() << "JSON string cannot be empty";
        return -1;
    }

    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON format";
        return -2;
    }

    QJsonObject status_obj = doc.object();

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path for:" << path;
        return -5;
    }

    UserSql user_sql(user_db_path);
    return user_sql.editStatus(year, month, day, status_obj);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QString GlobalSettings::getMonthUserDiarySqlData(int year, int month, const QString& path) const {
    if (year < 1 || month < 1 || month > 12) {
        qWarning() << "Invalid year or month for diary data retrieval.";
        return {}; // 無効な年または月の場合は空の文字列を返す
    }

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path.";
        return {}; // ユーザーデータベースパスの取得に失敗
    }

    UserSql user_sql(user_db_path);
    return user_sql.getDiariesByMonthJson(year, month);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QString GlobalSettings::getMonthUserStatusData(int year, int month, const QString& path) const {
    if (year < 1 || month < 1 || month > 12) {
        qWarning() << "Invalid year or month for status data retrieval.";
        return {}; // 無効な年または月の場合は空の文字列を返す
    }

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path.";
        return {}; // ユーザーデータベースパスの取得に失敗
    }

    UserSql user_sql(user_db_path);
    return user_sql.getStatusByMonthJson(year, month);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QString GlobalSettings::getSelectedUserStatusDataJson(int year, int month, int day,
                                                      const QString& path) const {
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31) {
        qWarning() << "Invalid date for status data retrieval.";
        return {}; // 無効な日付の場合は空の文字列を返す
    }

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path.";
        return {}; // ユーザーデータベースパスの取得に失敗
    }

    UserSql user_sql(user_db_path);
    QString date =
        QString("%1-%2-%3").arg(year).arg(month, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(user_db_path.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(
            db, "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, "
                "datetime(created_at, 'localtime') AS created_at_local "
                "FROM user_status "
                "WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            QJsonObject status_object;
            status_object["mood"] = query.getColumn(0).getInt();
            status_object["free_mood_text"] = QString::fromStdString(query.getColumn(1).getText());
            status_object["sleep_time"] = query.getColumn(2).getDouble();
            status_object["wake_up_time"] = query.getColumn(3).getDouble();
            status_object["temperature"] = query.getColumn(4).getDouble();
            status_object["createdAt"] = QString::fromStdString(query.getColumn(5).getText());
            return QJsonDocument(status_object).toJson(QJsonDocument::Indented);
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while getting selected status data:" << e.what();
    }

    return {}; // データが見つからない場合は空の文字列を返す
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QString GlobalSettings::loadMarkdownFile(const QString& filePath) const {

    QFileInfo file_info(filePath);
    if (!file_info.exists() || !file_info.isFile()) {
        qWarning() << "Invalid markdown file path:" << filePath;
        return {}; // 無効なファイルパスの場合は空の文字列を返す
    }

    if (file_info.size() == 0) {
        qInfo() << "[Markdown] File exists but empty (0 bytes):" << filePath;
        return QStringLiteral("# (Empty Diary)\n\n");
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not open markdown file:" << filePath;
        return {}; // ファイルを開けない場合は空の文字列を返す
    }

    // NOLINTNEXTLINE(readability-identifier-length)
    QTextStream in(&file);
    QString content = in.readAll();
    qInfo() << "[Markdown] Loaded:" << content;
    return content;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static,bugprone-easily-swappable-parameters)
int GlobalSettings::writeMarkdownFile(const QString& filePath, const QString& content) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open markdown file for writing:" << filePath;
        return -1; // ファイルを開けない場合のエラーコード
    }

    QTextStream out(&file);
    out << content;
    file.close();

    qInfo() << "Markdown file written successfully:" << filePath;
    return 0; // 成功
}
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
QString GlobalSettings::search(const QString& query, const QString& scope, bool caseSensitive,
                               bool useRegex, const QString& path) const {
    if (query.isEmpty() || path.isEmpty()) {
        qWarning() << "Query and path cannot be empty";
        return {}; // クエリまたはパスが空の場合は空の文字列を返す
    }

    QString user_db_path = UserSql::getUserDatabasePath(path);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path for:" << path;
        return {}; // ユーザーデータベースパスの取得に失敗
    }

    UserSql user_sql(user_db_path);
    return user_sql.search(query, scope, caseSensitive, useRegex);
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static,bugprone-easily-swappable-parameters)
int GlobalSettings::openGraphWindow(const QString& workspacePath, int scope, int filter,
                                    const QString& toStr, const QString& fromStr) {

    QWindow* parent_win = QGuiApplication::focusWindow();
    if ((parent_win == nullptr) && !QGuiApplication::topLevelWindows().isEmpty()) {
        parent_win = QGuiApplication::topLevelWindows().first();
    }

    const quint64 PARENT_ID = (parent_win != nullptr) ? parent_win->winId() : 0;

    QStringList args;
    if (PARENT_ID != 0U) {
        args << QStringLiteral("--parent-winid") << QString::number(PARENT_ID);
    }
    if (!workspacePath.isEmpty()) {

        args << QStringLiteral("--workspace") << workspacePath;
    }
    args << QStringLiteral("--scope") << QString::number(scope);
    args << QStringLiteral("--filter") << QString::number(filter);
    args << QStringLiteral("--to") << toStr;
    args << QStringLiteral("--from") << fromStr;
    args << QStringLiteral("--ident") << QStringLiteral("graph");

    const QString EXE = QCoreApplication::applicationFilePath();
    return static_cast<int>(QProcess::startDetached(EXE, args));
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
[[nodiscard]] QVariant GlobalSettings::getGraphData(const QString& workspacePath, int scope,
                                                    int filter, const QString& toStr,
                                                    const QString& fromStr) const {
    if (workspacePath.isEmpty()) {
        qWarning() << "Workspace path cannot be empty";
        return {};
    }
    QString user_db_path = UserSql::getUserDatabasePath(workspacePath);
    if (user_db_path.isEmpty()) {
        qWarning() << "Failed to get user database path for:" << workspacePath;
        return {};
    }
    UserSql user_sql(user_db_path);
    return user_sql.getGraphData(scope, filter, toStr, fromStr);
}
