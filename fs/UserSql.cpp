#include "UserSql.h"

#include "AppMigrations.h"
#include "Migration.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <SQLiteCpp/Database.h>
#include <algorithm>
#include <functional>
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <qdir.h>
#include <qjsonobject.h>
#include <qurl.h>

using std::greater;

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
int UserSql::createUserDatabase(const QString& path) const {
    if (path.isEmpty()) {
        qWarning() << "Workspace path has not exists";
        return static_cast<int>(UserSql::UserSqlError::PATH_NOT_SET);
    }

    QDir dir(path);
    const QString DB_FILE_PATH = dir.filePath("user.db");

    try {
        runMigrations(DB_FILE_PATH.toStdString(), makeMigrations());

    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while creating user database:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLITE_ERROR);
    }

    dir.mkdir("diaries");
    qInfo() << "Diaries directory created at:" << dir.filePath("diaries");

    return static_cast<int>(UserSql::UserSqlError::NO_ERROR);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
QUrl UserSql::createDiary(int year, int month, int day, const QString& title,
                          const QString& path) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return {};
    }

    const QString DATE =
        QString("%1-%2-%3").arg(year).arg(month, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));

    const QString SAFE_TITLE = sanitizeFileName(title);

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        db.setBusyTimeout(3000);

        // entry_date の有無を検出
        bool has_entry_date = false;
        {
            SQLite::Statement info(db, "PRAGMA table_info(diary)");
            while (info.executeStep()) {
                const std::string COL_NAME = info.getColumn(1).getText();
                if (COL_NAME == std::string("entry_date")) {
                    has_entry_date = true;
                    break;
                }
            }
        }

        if (has_entry_date) {
            SQLite::Statement check_query(
                db, "SELECT COUNT(*) FROM diary WHERE title = ? AND entry_date = ?");
            check_query.bind(1, SAFE_TITLE.toStdString());
            check_query.bind(2, DATE.toStdString());
            if (check_query.executeStep() && check_query.getColumn(0).getInt() > 0) {
                qWarning() << "Diary already exists for" << DATE << "title=" << SAFE_TITLE;
                return {};
            }
        } else {
            SQLite::Statement check_query(db, "SELECT COUNT(*) FROM diary WHERE title = ?");
            check_query.bind(1, SAFE_TITLE.toStdString());
            if (check_query.executeStep() && check_query.getColumn(0).getInt() > 0) {
                qWarning() << "Diary already exists (legacy schema, title unique). title="
                           << SAFE_TITLE;
                return {};
            }
        }

        if (path.isEmpty()) {
            qWarning() << "Workspace path is empty.";
            return {};
        }
        const QString DATE_DIR_PATH = QDir(path).filePath(QString("diaries/%1").arg(DATE));
        if (!QDir().mkpath(DATE_DIR_PATH)) {
            qWarning() << "Failed to create directory:" << DATE_DIR_PATH;
            return {};
        }
        const QString DIARY_FILE_PATH = QDir(DATE_DIR_PATH).filePath(SAFE_TITLE + ".md");

        if (QFileInfo::exists(DIARY_FILE_PATH)) {
            qWarning() << "Diary file already exists:" << DIARY_FILE_PATH;
            return {};
        }

        QFile diary_file(DIARY_FILE_PATH);
        if (!diary_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to create diary file:" << DIARY_FILE_PATH;
            return {};
        }
        diary_file.close();
        qInfo() << "Diary file created at:" << DIARY_FILE_PATH;

        if (has_entry_date) {
            SQLite::Statement query(db, "INSERT INTO diary (title, content_path, entry_date, "
                                        "created_at) VALUES (?, ?, ?, ?)");
            query.bind(1, SAFE_TITLE.toStdString());
            query.bind(2, DIARY_FILE_PATH.toStdString());
            query.bind(3, DATE.toStdString());
            query.bind(4, DATE.toStdString());
            query.exec();
        } else {
            // 旧スキーマ用（entry_date なし）
            SQLite::Statement query(
                db, "INSERT INTO diary (title, content_path, created_at) VALUES (?, ?, ?)");
            query.bind(1, SAFE_TITLE.toStdString());
            query.bind(2, DIARY_FILE_PATH.toStdString());
            query.bind(3, DATE.toStdString());
            query.exec();
        }

        qInfo() << "Diary entry created:" << SAFE_TITLE;
        return {QUrl::fromLocalFile(DIARY_FILE_PATH)};
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while creating diary entry:" << e.what();
        return {};
    }
}
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
int UserSql::updateDiary(const QString& title, const QString& newTitle,
                         const QString& newContentPath) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PATH_NOT_SET);
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);

        SQLite::Statement select_query(db, "SELECT content_path FROM diary WHERE title = ?");
        select_query.bind(1, title.toStdString());

        QString old_content_path;
        if (select_query.executeStep()) {
            old_content_path = QString::fromStdString(select_query.getColumn(0).getText());
        } else {
            qWarning() << "Diary entry not found with title:" << title;
            return static_cast<int>(UserSql::UserSqlError::DIARY_ALREADY_EXISTS);
        }

        QDir target_dir = QFileInfo(old_content_path).dir();
        const QString NEW_PATH = QDir::cleanPath(target_dir.filePath(newTitle + ".md"));

        if (old_content_path != NEW_PATH) {
            if (QFileInfo::exists(NEW_PATH)) {
                qWarning() << "Target diary file already exists:" << NEW_PATH;
                return static_cast<int>(UserSql::UserSqlError::DIARY_FILE_RENAME_FAILED);
            }
            if (!QFile::rename(old_content_path, NEW_PATH)) {
                qWarning() << "Failed to rename diary file from" << old_content_path << "to"
                           << NEW_PATH;
                return static_cast<int>(UserSql::UserSqlError::DIARY_FILE_RENAME_FAILED);
            }
        }

        SQLite::Statement update_query(db, "UPDATE diary SET title = ?, content_path = ?, "
                                           "last_modified = CURRENT_TIMESTAMP WHERE title = ?");
        update_query.bind(1, newTitle.toStdString());
        update_query.bind(2, NEW_PATH.toStdString());
        update_query.bind(3, title.toStdString());
        update_query.exec();

        qInfo() << "Diary entry updated:" << newTitle;
        return static_cast<int>(UserSql::UserSqlError::NO_ERROR);
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while updating diary entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLITE_ERROR);
    }
}

int UserSql::createStatus() const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PATH_NOT_SET);
    }

    if (isTodayStatusExists(QDate::currentDate().toString("yyyy-MM-dd"))) {
        qWarning() << "Status entry found for today.";
        return static_cast<int>(UserSql::UserSqlError::NO_STATUS_ENTRY_FOUND);
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO user_status (mood, free_mood_text, sleep_time, "
                                    "wake_up_time, temperature) VALUES (?, ?, ?, ?, ?)");
        query.bind(1, 2);
        query.bind(2, QString().toStdString());
        query.bind(3, 0.0);
        query.bind(4, 0.0);
        query.bind(5, 0.0);
        query.exec();
        qInfo() << "Status entry created.";
        return static_cast<int>(UserSql::UserSqlError::NO_ERROR);
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while creating status entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLITE_ERROR);
    }
}

int UserSql::editStatus(int year, int month, int day, const QJsonObject& jsonObject) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PATH_NOT_SET);
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(
            db,
            "UPDATE user_status SET mood = ?, free_mood_text = ?, sleep_time = ?, wake_up_time = "
            "?, temperature = ?, last_modified = CURRENT_TIMESTAMP WHERE date(created_at) = ?");
        query.bind(1, jsonObject["mood"].toInt());
        query.bind(2, jsonObject["free_mood_text"].toString().toStdString());
        query.bind(3, jsonObject["sleep_time"].toDouble());
        query.bind(4, jsonObject["wake_up_time"].toDouble());
        query.bind(5, jsonObject["temperature"].toDouble());
        query.bind(6, QDate(year, month, day).toString("yyyy-MM-dd").toStdString());
        query.exec();
        qInfo() << "Status entry updated for date:"
                << QDate(year, month, day).toString("yyyy-MM-dd");
        return static_cast<int>(UserSql::UserSqlError::NO_ERROR);
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while updating status entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLITE_ERROR);
    }
}

bool UserSql::isTodayDiaryExists(const QString& date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 今日の日記が存在するかどうかを返す
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking today's diary existence:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isDayDiaryExists(int year, int month, int day) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    QString date =
        QString("%1-%2-%3").arg(year).arg(month, 2, 10, QChar('0')).arg(day, 2, 10, QChar('0'));

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 指定された日の日記が存在するかどうかを返す
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking diary existence for date" << date << ":"
                   << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isTodayStatusExists(const QString& date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM user_status WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 今日のステータスが存在するかどうかを返す
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking today's status existence:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isEqualTodayAndCreateAtForDiary(const QString& date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() >
                   0; // 今日の日付と一致する日記が存在するかどうかを返す
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking today's diary creation date:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isEqualTodayAndCreateAtForStatus(const QString& date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM user_status WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() >
                   0; // 今日の日付と一致するステータスが存在するかどうかを返す
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while checking today's status creation date:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

QString UserSql::getDiariesByDate(const QString& date) const {
    QString diaries;
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return {};
    }

    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db,
                                "SELECT title, content_path FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        while (query.executeStep()) {
            QString title = QString::fromStdString(query.getColumn(0).getText());
            QString content_path = QString::fromStdString(query.getColumn(1).getText());
            diaries.append(title + ": " + content_path + "\n");
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while getting diaries by date:" << e.what();
    }
    return diaries;
}

QString UserSql::getDiariesByMonthJson(int year, int month) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return {};
    }

    QJsonArray diaries_array;
    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(
            db, "SELECT title, content_path, datetime(created_at, 'localtime') AS created_at_local "
                "FROM diary "
                "WHERE strftime('%Y', datetime(created_at, 'localtime')) = ? "
                "AND strftime('%m', datetime(created_at, 'localtime')) = ?");
        query.bind(1, QString::number(year).toStdString());
        query.bind(2, QString::number(month).rightJustified(2, '0').toStdString());

        while (query.executeStep()) {
            QJsonObject diary_object;
            diary_object["title"] = QString::fromStdString(query.getColumn(0).getText());
            diary_object["contentPath"] = QString::fromStdString(query.getColumn(1).getText());
            diary_object["createdAt"] = QString::fromStdString(query.getColumn(2).getText());
            diaries_array.append(diary_object);
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while getting diaries by month:" << e.what();
    }

    QJsonObject result;
    result["diaries"] = diaries_array;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}

QString UserSql::getStatusByMonthJson(int year, int month) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return {};
    }

    QJsonArray status_array;
    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(
            db, "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, "
                "datetime(created_at, 'localtime') AS created_at_local "
                "FROM user_status "
                "WHERE strftime('%Y', datetime(created_at, 'localtime')) = ? "
                "AND strftime('%m', datetime(created_at, 'localtime')) = ?");
        query.bind(1, QString::number(year).toStdString());
        query.bind(2, QString::number(month).rightJustified(2, '0').toStdString());

        while (query.executeStep()) {
            QJsonObject status_object;
            status_object["mood"] = query.getColumn(0).getInt();
            status_object["freeMoodText"] = QString::fromStdString(query.getColumn(1).getText());
            status_object["sleepTime"] = query.getColumn(2).getDouble();
            status_object["wakeUpTime"] = query.getColumn(3).getDouble();
            status_object["temperature"] = query.getColumn(4).getDouble();
            status_object["createdAt"] = QString::fromStdString(query.getColumn(5).getText());
            status_array.append(status_object);
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while getting user status by month:" << e.what();
    }

    QJsonObject result;
    result["status"] = status_array;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}
// とりあえずreadability-function-cognitive-complexityは無視する。実際に見にくく感じたら関数を分割する。
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters, readability-function-cognitive-complexity)
QString UserSql::search(const QString& query, const QString& scope, bool caseSensitive,
                        bool useRegex) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return {};
    }

    const QString STR = scope.trimmed().toLower();

    // Optional: detect a simple date condition like "date>=YYYY-MM-DD"
    // Supported ops: =, ==, <>, !=, <, <=, >, >=
    QRegularExpression date_rx(
        R"(^\s*date\s*(<=|>=|=|==|<>|!=|<|>)\s*([0-9]{4}-[0-9]{2}-[0-9]{2})\s*$)",
        QRegularExpression::CaseInsensitiveOption);
    const auto MATCH = date_rx.match(query);
    const bool HAS_DATE_COND = MATCH.hasMatch();
    // NOLINTNEXTLINE(readability-identifier-length)
    QString op;
    QString date_str;
    if (HAS_DATE_COND) {
        op = MATCH.captured(1);
        date_str = MATCH.captured(2);
        if (op == "==") {
            op = "=";
        }
        if (op == "<>") {
            op = "!=";
        }
        if (op == "!=") {
            op = "!=";
        }
    }

    QJsonArray results_array;
    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        if (STR != "diary" && STR != "status" && STR != "all") {
            qWarning() << "Invalid search scope:" << scope;
            return {};
        }

        if (STR == "diary") {
            if (HAS_DATE_COND) {
                const std::string SQL =
                    std::string("SELECT title, content_path, datetime(created_at, 'localtime') AS "
                                "created_at_local "
                                "FROM diary WHERE date(datetime(created_at, 'localtime')) ") +
                    op.toStdString() + " ?";
                SQLite::Statement stmt(db, SQL);
                stmt.bind(1, date_str.toStdString());
                while (stmt.executeStep()) {
                    QString title = QString::fromStdString(stmt.getColumn(0).getText());
                    QString content_path = QString::fromStdString(stmt.getColumn(1).getText());
                    QString created_at = QString::fromStdString(stmt.getColumn(2).getText());
                    QJsonObject result;
                    result["type"] = "diary";
                    result["title"] = title;
                    result["contentPath"] = content_path;
                    result["date"] = created_at;
                    results_array.append(result);
                }
            } else {
                SQLite::Statement stmt(db, "SELECT title, content_path, datetime(created_at, "
                                           "'localtime') AS created_at_local FROM diary");
                while (stmt.executeStep()) {
                    QString title = QString::fromStdString(stmt.getColumn(0).getText());
                    QString content_path = QString::fromStdString(stmt.getColumn(1).getText());
                    QString created_at = QString::fromStdString(stmt.getColumn(2).getText());

                    bool match = false;
                    if (useRegex) {
                        QRegularExpression regex(
                            query, caseSensitive ? QRegularExpression::NoPatternOption
                                                 : QRegularExpression::CaseInsensitiveOption);
                        match =
                            regex.match(title).hasMatch() || regex.match(content_path).hasMatch();
                    } else {
                        Qt::CaseSensitivity csens =
                            caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                        match = title.contains(query, csens) || content_path.contains(query, csens);
                    }

                    if (match) {
                        QJsonObject result;
                        result["type"] = "diary";
                        result["title"] = title;
                        result["contentPath"] = content_path;
                        result["date"] = created_at;
                        results_array.append(result);
                    }
                }
            }
        }

        if (STR == "status") {
            if (HAS_DATE_COND) {
                const std::string SQL =
                    std::string(
                        "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, "
                        "datetime(created_at, 'localtime') AS created_at_local "
                        "FROM user_status WHERE date(datetime(created_at, 'localtime')) ") +
                    op.toStdString() + " ?";
                SQLite::Statement stmt(db, SQL);
                stmt.bind(1, date_str.toStdString());
                while (stmt.executeStep()) {
                    QJsonObject result;
                    result["type"] = "status";
                    result["mood"] = stmt.getColumn(0).getInt();
                    result["freeMoodText"] = QString::fromStdString(stmt.getColumn(1).getText());
                    result["sleepTime"] = stmt.getColumn(2).getDouble();
                    result["wakeUpTime"] = stmt.getColumn(3).getDouble();
                    result["temperature"] = stmt.getColumn(4).getDouble();
                    result["date"] = QString::fromStdString(stmt.getColumn(5).getText());
                    results_array.append(result);
                }
            } else {
                SQLite::Statement stmt(
                    db, "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, "
                        "datetime(created_at, 'localtime') AS created_at_local FROM user_status");
                while (stmt.executeStep()) {
                    QString mood = QString::number(stmt.getColumn(0).getInt());
                    QString free_mood_text = QString::fromStdString(stmt.getColumn(1).getText());
                    QString sleep_time = QString::number(stmt.getColumn(2).getDouble());
                    QString wake_up_time = QString::number(stmt.getColumn(3).getDouble());
                    QString temperature = QString::number(stmt.getColumn(4).getDouble());
                    QString created_at = QString::fromStdString(stmt.getColumn(5).getText());

                    bool match = false;
                    if (useRegex) {
                        QRegularExpression regex(
                            query, caseSensitive ? QRegularExpression::NoPatternOption
                                                 : QRegularExpression::CaseInsensitiveOption);
                        match = regex.match(mood).hasMatch() ||
                                regex.match(free_mood_text).hasMatch() ||
                                regex.match(sleep_time).hasMatch() ||
                                regex.match(wake_up_time).hasMatch() ||
                                regex.match(temperature).hasMatch();
                    } else {
                        Qt::CaseSensitivity csens =
                            caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                        match = mood.contains(query, csens) ||
                                free_mood_text.contains(query, csens) ||
                                sleep_time.contains(query, csens) ||
                                wake_up_time.contains(query, csens) ||
                                temperature.contains(query, csens);
                    }

                    if (match) {
                        QJsonObject result;
                        result["type"] = "status";
                        result["mood"] = mood;
                        result["freeMoodText"] = free_mood_text;
                        result["sleepTime"] = sleep_time;
                        result["wakeUpTime"] = wake_up_time;
                        result["temperature"] = temperature;
                        result["date"] = created_at;
                        results_array.append(result);
                    }
                }
            }
        }

        if (STR == "all") {
            if (HAS_DATE_COND) {
                const QString QSQL =
                    QString(
                        "SELECT type, created_at_local, id, title, content_path, "
                        "       mood, free_mood_text, sleep_time, wake_up_time, temperature "
                        "FROM ( "
                        "  SELECT 'diary' AS type, datetime(created_at, 'localtime') AS "
                        "created_at_local, "
                        "         id, title, content_path, NULL AS mood, NULL AS free_mood_text, "
                        "         NULL AS sleep_time, NULL AS wake_up_time, NULL AS temperature "
                        "  FROM diary WHERE date(datetime(created_at, 'localtime')) %1 ? "
                        "  UNION ALL "
                        "  SELECT 'status', datetime(created_at, 'localtime'), "
                        "         id, NULL, NULL, mood, free_mood_text, sleep_time, wake_up_time, "
                        "temperature "
                        "  FROM user_status WHERE date(datetime(created_at, 'localtime')) %1 ? "
                        ") u "
                        "ORDER BY created_at_local DESC")
                        .arg(op);
                SQLite::Statement stmt(db, QSQL.toStdString());
                stmt.bind(1, date_str.toStdString());
                stmt.bind(2, date_str.toStdString());
                while (stmt.executeStep()) {
                    const QString TYPE = QString::fromStdString(stmt.getColumn(0).getText());
                    const QString CREATED_AT = QString::fromStdString(stmt.getColumn(1).getText());
                    QJsonObject result;
                    result["type"] = TYPE;
                    result["date"] = CREATED_AT;
                    if (TYPE == "diary") {
                        result["title"] = QString::fromStdString(stmt.getColumn(3).getText());
                        result["contentPath"] = QString::fromStdString(stmt.getColumn(4).getText());
                    } else {
                        result["mood"] =
                            stmt.getColumn(5).isNull() ? 0 : stmt.getColumn(5).getInt();
                        result["freeMoodText"] =
                            QString::fromStdString(stmt.getColumn(6).getText());
                        result["sleepTime"] =
                            stmt.getColumn(7).isNull() ? 0.0 : stmt.getColumn(7).getDouble();
                        result["wakeUpTime"] =
                            stmt.getColumn(8).isNull() ? 0.0 : stmt.getColumn(8).getDouble();
                        result["temperature"] =
                            stmt.getColumn(9).isNull() ? 0.0 : stmt.getColumn(9).getDouble();
                    }
                    results_array.append(result);
                }
            } else {
                // Fallback: return flat list (you can swap to grouped if desired)
                SQLite::Statement stmt(
                    db, "SELECT type, created_at_local, id, title, content_path, "
                        "       mood, free_mood_text, sleep_time, wake_up_time, temperature "
                        "FROM ( "
                        "  SELECT 'diary' AS type, datetime(created_at, 'localtime') AS "
                        "created_at_local, "
                        "         id, title, content_path, NULL AS mood, NULL AS free_mood_text, "
                        "         NULL AS sleep_time, NULL AS wake_up_time, NULL AS temperature "
                        "  FROM diary "
                        "  UNION ALL "
                        "  SELECT 'status', datetime(created_at, 'localtime'), "
                        "         id, NULL, NULL, mood, free_mood_text, sleep_time, wake_up_time, "
                        "temperature "
                        "  FROM user_status "
                        ") u "
                        "ORDER BY created_at_local DESC");
                while (stmt.executeStep()) {
                    const QString TYPE = QString::fromStdString(stmt.getColumn(0).getText());
                    const QString CREATED_AT = QString::fromStdString(stmt.getColumn(1).getText());
                    QJsonObject result;
                    result["type"] = TYPE;
                    result["date"] = CREATED_AT;
                    if (TYPE == "diary") {
                        result["title"] = QString::fromStdString(stmt.getColumn(3).getText());
                        result["contentPath"] = QString::fromStdString(stmt.getColumn(4).getText());
                    } else {
                        result["mood"] =
                            stmt.getColumn(5).isNull() ? 0 : stmt.getColumn(5).getInt();
                        result["freeMoodText"] =
                            QString::fromStdString(stmt.getColumn(6).getText());
                        result["sleepTime"] =
                            stmt.getColumn(7).isNull() ? 0.0 : stmt.getColumn(7).getDouble();
                        result["wakeUpTime"] =
                            stmt.getColumn(8).isNull() ? 0.0 : stmt.getColumn(8).getDouble();
                        result["temperature"] =
                            stmt.getColumn(9).isNull() ? 0.0 : stmt.getColumn(9).getDouble();
                    }
                    results_array.append(result);
                }
            }
        }

    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while performing search:" << e.what();
    }

    QJsonObject result;
    result["results"] = results_array;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}
// NOLINTNEXTLINE(readability-identifier-length)
QString UserSql::searchGroupedAll(SQLite::Database& db) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return {};
    }

    QMap<QString, QJsonArray> buckets;
    try {
        SQLite::Statement stmt(
            db,
            "SELECT type, created_at_local, id, title, content_path, "
            "       mood, free_mood_text, sleep_time, wake_up_time, temperature "
            "FROM ( "
            "  SELECT 'diary' AS type, datetime(created_at, 'localtime') AS created_at_local, "
            "         id, title, content_path, NULL AS mood, NULL AS free_mood_text, "
            "         NULL AS sleep_time, NULL AS wake_up_time, NULL AS temperature "
            "  FROM diary "
            "  UNION ALL "
            "  SELECT 'status', datetime(created_at, 'localtime'), "
            "         id, NULL, NULL, mood, free_mood_text, sleep_time, wake_up_time, temperature "
            "  FROM user_status "
            ") u "
            "ORDER BY created_at_local DESC");

        while (stmt.executeStep()) {
            const QString TYPE = QString::fromStdString(stmt.getColumn(0).getText());
            const QString CREATED_AT = QString::fromStdString(stmt.getColumn(1).getText());
            // NOLINTNEXTLINE(readability-identifier-length)
            const int ID = stmt.getColumn(2).getInt();

            QJsonObject entry;
            entry["type"] = TYPE;
            entry["id"] = ID;
            entry["createdAt"] = CREATED_AT;

            if (TYPE == "diary") {
                entry["title"] = QString::fromStdString(stmt.getColumn(3).getText());
                entry["contentPath"] = QString::fromStdString(stmt.getColumn(4).getText());
            } else if (TYPE == "status") {
                QJsonObject status;
                entry["mood"] = stmt.getColumn(5).getInt();
                entry["freeMoodText"] = QString::fromStdString(stmt.getColumn(6).getText());
                entry["sleepTime"] = stmt.getColumn(7).getDouble();
                entry["wakeUpTime"] = stmt.getColumn(8).getDouble();
                entry["temperature"] = stmt.getColumn(9).getDouble();
                entry["status"] = status;
            }

            const QString DATE_KEY = CREATED_AT.left(10);
            buckets[DATE_KEY].append(entry);
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while grouping search results:" << e.what();
    }

    QStringList dates = buckets.keys();
    std::ranges::sort(dates, std::ranges::greater{});

    QJsonArray grouped_results;
    for (const QString& date : dates) {
        QJsonObject group;
        group["date"] = date;
        group["entries"] = buckets.value(date);
        grouped_results.append(group);
    }

    QJsonObject out;
    out["groupedResults"] = grouped_results;
    return QJsonDocument(out).toJson(QJsonDocument::Indented);
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
QVariant UserSql::getGraphData(int scope, int filter, const QString& start_date,
                               const QString& end_date) const {
    QVariantMap root;

    QString start_out = start_date;
    QString end_out = end_date;

    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        root.insert("start", start_out);
        root.insert("end", end_out);

        return root;
    }

    QDate from = QDate::fromString(start_date, "yyyy-MM-dd");
    // NOLINTNEXTLINE(readability-identifier-length)
    QDate to = QDate::fromString(end_date, "yyyy-MM-dd");
    if (!from.isValid() || !to.isValid()) {
        qWarning() << "Invalid date range:" << start_date << "to" << end_date;
        root.insert("start", start_out);
        root.insert("end", end_out);
        root.insert("days", QVariantList{});

        return root;
    }

    if (from > to) {
        std::swap(from, to);
    }

    start_out = from.toString("yyyy-MM-dd");
    end_out = to.toString("yyyy-MM-dd");

    QVariantList days_list;
    try {
        // NOLINTNEXTLINE(readability-identifier-length)
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        db.setBusyTimeout(3000);

        QSet<QString> diary_dates;
        SQLite::Statement diary_query(
            db,
            "SELECT DISTINCT date(created_at) FROM diary WHERE date(created_at) BETWEEN ? AND ?");
        diary_query.bind(1, from.toString("yyyy-MM-dd").toStdString());
        diary_query.bind(2, to.toString("yyyy-MM-dd").toStdString());
        while (diary_query.executeStep()) {
            diary_dates.insert(QString::fromStdString(diary_query.getColumn(0).getText()));
        }

        QHash<QString, QVariantMap> status_data;

        QString status_sq1;
        if (scope == 0) {
            status_sq1 = "SELECT date(created_at), mood, free_mood_text, sleep_time, wake_up_time, "
                         "temperature FROM user_status WHERE date(created_at) BETWEEN ? AND ?";
        } else if (scope == 1) {
            status_sq1 = "SELECT date(created_at), sleep_time, wake_up_time FROM user_status WHERE "
                         "date(created_at) BETWEEN ? AND ?";
        }

        SQLite::Statement status_query(db, status_sq1.toStdString());
        status_query.bind(1, from.toString("yyyy-MM-dd").toStdString());
        status_query.bind(2, to.toString("yyyy-MM-dd").toStdString());
        while (status_query.executeStep()) {
            QString date = QString::fromStdString(status_query.getColumn(0).getText());
            QVariantMap data;

            if (scope == 1) {
                data.insert("sleep_time", status_query.getColumn(1).getDouble());
                data.insert("wake_up_time", status_query.getColumn(2).getDouble());
            } else {
                data.insert("mood", status_query.getColumn(1).getInt());
                data.insert("free_mood_text",
                            QString::fromStdString(status_query.getColumn(2).getText()));
                data.insert("sleep_time", status_query.getColumn(3).getDouble());
                data.insert("wake_up_time", status_query.getColumn(4).getDouble());
                data.insert("temperature", status_query.getColumn(5).getDouble());
            }
            status_data.insert(date, data);
        }

        for (QDate cursor = from; cursor <= to; cursor = cursor.addDays(1)) {
            QString data_str = cursor.toString("yyyy-MM-dd");
            QVariantMap day_data;
            day_data.insert("date", data_str);
            day_data.insert("diary_exists", diary_dates.contains(data_str));

            if (status_data.contains(data_str)) {
                day_data.insert("status", status_data.value(data_str));
            } else {
                day_data.insert("status", QVariant());
            }
            days_list.append(day_data);
        }
    } catch (const SQLite::Exception& e) {
        qWarning() << "SQLite error while getting graph data:" << e.what();
        root.insert("start", start_out);
        root.insert("end", end_out);
        root.insert("days", QVariantList{});

        return root;
    }

    root.insert("start", start_out);
    root.insert("end", end_out);
    root.insert("days", days_list);
    root.insert("scope", scope);

    return root;
}
// NOLINTNEXTLINE(readability-identifier-length, readability-convert-member-functions-to-static)
QString UserSql::sanitizeFileName(const QString& in) const {
    QString sanitized = in;
    sanitized.replace(QRegularExpression(R"([\\/:*?"<>|])"), "_");
    sanitized = sanitized.trimmed();
    while (!sanitized.isEmpty() && (sanitized.endsWith('.') || sanitized.endsWith(' '))) {
        sanitized.chop(1);
    }
    static const QSet<QString> RESERVED = {
        "CON",  "PRN",  "AUX",  "NUL",  "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7",
        "COM8", "COM9", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
    if (RESERVED.contains(sanitized.toUpper())) {
        sanitized += "_";
    }
    if (sanitized.isEmpty()) {
        sanitized = "untitled";
    }
    return sanitized;
}
UserSql::UserSql(QString pathToUserDb) : m_pathToUserDb(std::move(pathToUserDb)) {}
