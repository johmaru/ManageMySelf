#include "UserSql.h"
#include "Migration.h"
#include "AppMigrations.h"
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <SQLiteCpp/Database.h>
#include <qdir.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <qjsonobject.h>

int UserSql::createUserDatabase(const QString &path) const {
    if (path.isEmpty()) {
        qWarning() << "Workspace path has not exists";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    QDir dir(path);
    const QString dbFilePath = dir.filePath("user.db");

    try {
        runMigrations(dbFilePath.toStdString(), makeMigrations());

    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating user database:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }

    dir.mkdir("diaries");
    qInfo() << "Diaries directory created at:" << dir.filePath("diaries");

    return static_cast<int>(UserSql::UserSqlError::NoError);
}

int UserSql::createDiary(int year, int month, int day, const QString &title, const QString &path) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    const QString date = QString("%1-%2-%3")
                           .arg(year)
                           .arg(month, 2, 10, QChar('0'))
                           .arg(day, 2, 10, QChar('0'));

    auto sanitizeFileName = [](QString in) -> QString {
        in.replace(QRegularExpression("[\\\\/:*?\"<>|]"), "_");
        in = in.trimmed();
        while (!in.isEmpty() && (in.endsWith('.') || in.endsWith(' '))) in.chop(1);
        static const QSet<QString> reserved = {
            "CON","PRN","AUX","NUL","COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
            "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
        };
        if (reserved.contains(in.toUpper())) in += "_";
        if (in.isEmpty()) in = "untitled";
        return in;
    };
    const QString safeTitle = sanitizeFileName(title);

    try {

        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        db.setBusyTimeout(3000);

        // entry_date の有無を検出
        bool hasEntryDate = false;
        {
            SQLite::Statement info(db, "PRAGMA table_info(diary)");
            while (info.executeStep()) {
                const std::string colName = info.getColumn(1).getText();
                if (colName == std::string("entry_date")) { hasEntryDate = true; break; }
            }
        }

        if (hasEntryDate) {
            SQLite::Statement check_query(db, "SELECT COUNT(*) FROM diary WHERE title = ? AND entry_date = ?");
            check_query.bind(1, safeTitle.toStdString());
            check_query.bind(2, date.toStdString());
            if (check_query.executeStep() && check_query.getColumn(0).getInt() > 0) {
                qWarning() << "Diary already exists for" << date << "title=" << safeTitle;
                return static_cast<int>(UserSql::UserSqlError::DiaryAlreadyExists);
            }
        } else {
            SQLite::Statement check_query(db, "SELECT COUNT(*) FROM diary WHERE title = ?");
            check_query.bind(1, safeTitle.toStdString());
            if (check_query.executeStep() && check_query.getColumn(0).getInt() > 0) {
                qWarning() << "Diary already exists (legacy schema, title unique). title=" << safeTitle;
                return static_cast<int>(UserSql::UserSqlError::DiaryAlreadyExists);
            }
        }

        if (path.isEmpty()) {
            qWarning() << "Workspace path is empty.";
            return static_cast<int>(UserSqlError::PathNotSet);
        }
        const QString dateDirPath = QDir(path).filePath(QString("diaries/%1").arg(date));
        if (!QDir().mkpath(dateDirPath)) {
            qWarning() << "Failed to create directory:" << dateDirPath;
            return static_cast<int>(UserSqlError::FileError);
        }
        const QString diaryFilePath = QDir(dateDirPath).filePath(safeTitle + ".md");

        if (QFileInfo::exists(diaryFilePath)) {
            qWarning() << "Diary file already exists:" << diaryFilePath;
            return static_cast<int>(UserSqlError::DiaryAlreadyExists);
        }

        QFile diaryFile(diaryFilePath);
        if (!diaryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to create diary file:" << diaryFilePath;
            return static_cast<int>(UserSqlError::FileError);
        }
        diaryFile.close();
        qInfo() << "Diary file created at:" << diaryFilePath;

        if (hasEntryDate) {
            SQLite::Statement query(db,
                "INSERT INTO diary (title, content_path, entry_date, created_at) VALUES (?, ?, ?, ?)");
            query.bind(1, safeTitle.toStdString());
            query.bind(2, diaryFilePath.toStdString());
            query.bind(3, date.toStdString());
            query.bind(4, date.toStdString());
            query.exec();
        } else {
            // 旧スキーマ用（entry_date なし）
            SQLite::Statement query(db,
                "INSERT INTO diary (title, content_path, created_at) VALUES (?, ?, ?)");
            query.bind(1, safeTitle.toStdString());
            query.bind(2, diaryFilePath.toStdString());
            query.bind(3, date.toStdString());
            query.exec();
        }

        qInfo() << "Diary entry created:" << safeTitle;
        return static_cast<int>(UserSql::UserSqlError::NoError);
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating diary entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }
}

int UserSql::updateDiary(const QString &title, const QString &newTitle, const QString &newContentPath) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        
        SQLite::Statement selectQuery(db, "SELECT content_path FROM diary WHERE title = ?");
        selectQuery.bind(1, title.toStdString());
        
        QString oldContentPath;
        if (selectQuery.executeStep()) {
            oldContentPath = QString::fromStdString(selectQuery.getColumn(0).getText());
        } else {
            qWarning() << "Diary entry not found with title:" << title;
            return static_cast<int>(UserSql::UserSqlError::DiaryAlreadyExists);
        }
        
        QDir targetDir = QFileInfo(oldContentPath).dir();
        const QString newPath = QDir::cleanPath(targetDir.filePath(newTitle + ".md"));

            if (oldContentPath != newPath) {
            if (QFileInfo::exists(newPath)) {
                qWarning() << "Target diary file already exists:" << newPath;
                return static_cast<int>(UserSql::UserSqlError::DiaryFileRenameFailed);
            }
            if (!QFile::rename(oldContentPath, newPath)) {
                qWarning() << "Failed to rename diary file from" << oldContentPath << "to" << newPath;
                return static_cast<int>(UserSql::UserSqlError::DiaryFileRenameFailed);
            }
        }

        SQLite::Statement updateQuery(
            db,
            "UPDATE diary SET title = ?, content_path = ?, last_modified = CURRENT_TIMESTAMP WHERE title = ?"
        );
        updateQuery.bind(1, newTitle.toStdString());
        updateQuery.bind(2, newPath.toStdString());
        updateQuery.bind(3, title.toStdString());
        updateQuery.exec();
        
        qInfo() << "Diary entry updated:" << newTitle;
        return static_cast<int>(UserSql::UserSqlError::NoError);
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while updating diary entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }
}

int UserSql::createStatus() const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    if (isTodayStatusExists(QDate::currentDate().toString("yyyy-MM-dd"))) {
        qWarning() << "Status entry found for today.";
        return static_cast<int>(UserSql::UserSqlError::NoStatusEntryFound);
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO user_status (mood, free_mood_text, sleep_time, wake_up_time, temperature) VALUES (?, ?, ?, ?, ?)");
        query.bind(1, 2);
        query.bind(2, QString().toStdString());
        query.bind(3, 0.0);
        query.bind(4, 0.0);
        query.bind(5, 0.0);
        query.exec();
        qInfo() << "Status entry created.";
        return static_cast<int>(UserSql::UserSqlError::NoError);
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating status entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }
}

int UserSql::editStatus(int year, int month, int day, const QJsonObject &jsonObject) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "UPDATE user_status SET mood = ?, free_mood_text = ?, sleep_time = ?, wake_up_time = ?, temperature = ?, last_modified = CURRENT_TIMESTAMP WHERE date(created_at) = ?");
        query.bind(1, jsonObject["mood"].toInt());
        query.bind(2, jsonObject["free_mood_text"].toString().toStdString());
        query.bind(3, jsonObject["sleep_time"].toDouble());
        query.bind(4, jsonObject["wake_up_time"].toDouble());
        query.bind(5, jsonObject["temperature"].toDouble());
        query.bind(6, QDate(year, month, day).toString("yyyy-MM-dd").toStdString());
        query.exec();
        qInfo() << "Status entry updated for date:" << QDate(year, month, day).toString("yyyy-MM-dd");
        return static_cast<int>(UserSql::UserSqlError::NoError);
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while updating status entry:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }
}

bool UserSql::isTodayDiaryExists(const QString &date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 今日の日記が存在するかどうかを返す
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking today's diary existence:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isDayDiaryExists(int year, int month, int day) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    QString date = QString("%1-%2-%3")
                       .arg(year)
                       .arg(month, 2, 10, QChar('0'))
                       .arg(day, 2, 10, QChar('0'));

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 指定された日の日記が存在するかどうかを返す
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking diary existence for date" << date << ":" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isTodayStatusExists(const QString &date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM user_status WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 今日のステータスが存在するかどうかを返す
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking today's status existence:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isEqualTodayAndCreateAtForDiary(const QString &date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 今日の日付と一致する日記が存在するかどうかを返す
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking today's diary creation date:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

bool UserSql::isEqualTodayAndCreateAtForStatus(const QString &date) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return false; // パスが空の場合はfalseを返す
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT COUNT(*) FROM user_status WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        if (query.executeStep()) {
            return query.getColumn(0).getInt() > 0; // 今日の日付と一致するステータスが存在するかどうかを返す
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while checking today's status creation date:" << e.what();
    }
    return false; // エラーが発生した場合はfalseを返す
}

QString UserSql::getDiariesByDate(const QString &date) const {
    QString diaries;
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return QString();
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT title, content_path FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        while (query.executeStep()) {
            QString title = QString::fromStdString(query.getColumn(0).getText());
            QString contentPath = QString::fromStdString(query.getColumn(1).getText());
            diaries.append(title + ": " + contentPath + "\n");
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while getting diaries by date:" << e.what();
    }
    return diaries;
}

QString UserSql::getDiariesByMonthJson(int year, int month) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return QString();
    }

    QJsonArray diariesArray;
    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(
            db,
            "SELECT title, content_path, datetime(created_at, 'localtime') AS created_at_local "
            "FROM diary "
            "WHERE strftime('%Y', datetime(created_at, 'localtime')) = ? "
            "AND strftime('%m', datetime(created_at, 'localtime')) = ?"
        );
        query.bind(1, QString::number(year).toStdString());
        query.bind(2, QString::number(month).rightJustified(2, '0').toStdString());

        while (query.executeStep()) {
            QJsonObject diaryObject;
            diaryObject["title"] = QString::fromStdString(query.getColumn(0).getText());
            diaryObject["contentPath"] = QString::fromStdString(query.getColumn(1).getText());
            diaryObject["createdAt"] = QString::fromStdString(query.getColumn(2).getText());
            diariesArray.append(diaryObject);
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while getting diaries by month:" << e.what();
    }

    QJsonObject result;
    result["diaries"] = diariesArray;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}

QString UserSql::getStatusByMonthJson(int year, int month) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return QString();
    }

    QJsonArray statusArray;
    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(
            db,
            "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, datetime(created_at, 'localtime') AS created_at_local "
            "FROM user_status "
            "WHERE strftime('%Y', datetime(created_at, 'localtime')) = ? "
            "AND strftime('%m', datetime(created_at, 'localtime')) = ?"
        );
        query.bind(1, QString::number(year).toStdString());
        query.bind(2, QString::number(month).rightJustified(2, '0').toStdString());

        while (query.executeStep()) {
            QJsonObject statusObject;
            statusObject["mood"] = query.getColumn(0).getInt();
            statusObject["freeMoodText"] = QString::fromStdString(query.getColumn(1).getText());
            statusObject["sleepTime"] = query.getColumn(2).getDouble();
            statusObject["wakeUpTime"] = query.getColumn(3).getDouble();
            statusObject["temperature"] = query.getColumn(4).getDouble();
            statusObject["createdAt"] = QString::fromStdString(query.getColumn(5).getText());
            statusArray.append(statusObject);
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while getting user status by month:" << e.what();
    }

    QJsonObject result;
    result["status"] = statusArray;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}

QString UserSql::search(const QString& query, const QString& scope, bool caseSensitive, bool useRegex) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return QString();
    }

    const QString s = scope.trimmed().toLower();

    // Optional: detect a simple date condition like "date>=YYYY-MM-DD"
    // Supported ops: =, ==, <>, !=, <, <=, >, >=
    QRegularExpression dateRx(
        "^\\s*date\\s*(<=|>=|=|==|<>|!=|<|>)\\s*([0-9]{4}-[0-9]{2}-[0-9]{2})\\s*$",
        QRegularExpression::CaseInsensitiveOption);
    const auto m = dateRx.match(query);
    const bool hasDateCond = m.hasMatch();
    QString op;
    QString dateStr;
    if (hasDateCond) {
        op = m.captured(1);
        dateStr = m.captured(2);
        if (op == "==") op = "=";
        if (op == "<>") op = "!=";
        if (op == "!=") op = "!=";
    }

    QJsonArray resultsArray;
    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        if (s != "diary" && s != "status" && s != "all") {
            qWarning() << "Invalid search scope:" << scope;
            return QString();
        }

        if (s == "diary") {
            if (hasDateCond) {
                const std::string sql = std::string(
                    "SELECT title, content_path, datetime(created_at, 'localtime') AS created_at_local "
                    "FROM diary WHERE date(datetime(created_at, 'localtime')) ") + op.toStdString() + " ?";
                SQLite::Statement stmt(db, sql);
                stmt.bind(1, dateStr.toStdString());
                while (stmt.executeStep()) {
                    QString title = QString::fromStdString(stmt.getColumn(0).getText());
                    QString contentPath = QString::fromStdString(stmt.getColumn(1).getText());
                    QString createdAt = QString::fromStdString(stmt.getColumn(2).getText());
                    QJsonObject result;
                    result["type"] = "diary";
                    result["title"] = title;
                    result["contentPath"] = contentPath;
                    result["date"] = createdAt;
                    resultsArray.append(result);
                }
            } else {
                SQLite::Statement stmt(db, "SELECT title, content_path, datetime(created_at, 'localtime') AS created_at_local FROM diary");
                while (stmt.executeStep()) {
                    QString title = QString::fromStdString(stmt.getColumn(0).getText());
                    QString contentPath = QString::fromStdString(stmt.getColumn(1).getText());
                    QString createdAt = QString::fromStdString(stmt.getColumn(2).getText());

                    bool match = false;
                    if (useRegex) {
                        QRegularExpression regex(query, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
                        match = regex.match(title).hasMatch() || regex.match(contentPath).hasMatch();
                    } else {
                        Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                        match = title.contains(query, cs) || contentPath.contains(query, cs);
                    }

                    if (match) {
                        QJsonObject result;
                        result["type"] = "diary";
                        result["title"] = title;
                        result["contentPath"] = contentPath;
                        result["date"] = createdAt;
                        resultsArray.append(result);
                    }
                }
            }
        }

        if (s == "status") {
            if (hasDateCond) {
                const std::string sql = std::string(
                    "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, datetime(created_at, 'localtime') AS created_at_local "
                    "FROM user_status WHERE date(datetime(created_at, 'localtime')) ") + op.toStdString() + " ?";
                SQLite::Statement stmt(db, sql);
                stmt.bind(1, dateStr.toStdString());
                while (stmt.executeStep()) {
                    QJsonObject result;
                    result["type"] = "status";
                    result["mood"] = stmt.getColumn(0).getInt();
                    result["freeMoodText"] = QString::fromStdString(stmt.getColumn(1).getText());
                    result["sleepTime"] = stmt.getColumn(2).getDouble();
                    result["wakeUpTime"] = stmt.getColumn(3).getDouble();
                    result["temperature"] = stmt.getColumn(4).getDouble();
                    result["date"] = QString::fromStdString(stmt.getColumn(5).getText());
                    resultsArray.append(result);
                }
            } else {
                SQLite::Statement stmt(db, "SELECT mood, free_mood_text, sleep_time, wake_up_time, temperature, datetime(created_at, 'localtime') AS created_at_local FROM user_status");
                while (stmt.executeStep()) {
                    QString mood = QString::number(stmt.getColumn(0).getInt());
                    QString freeMoodText = QString::fromStdString(stmt.getColumn(1).getText());
                    QString sleepTime = QString::number(stmt.getColumn(2).getDouble());
                    QString wakeUpTime = QString::number(stmt.getColumn(3).getDouble());
                    QString temperature = QString::number(stmt.getColumn(4).getDouble());
                    QString createdAt = QString::fromStdString(stmt.getColumn(5).getText());

                    bool match = false;
                    if (useRegex) {
                        QRegularExpression regex(query, caseSensitive ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption);
                        match = regex.match(mood).hasMatch() || regex.match(freeMoodText).hasMatch() ||
                                regex.match(sleepTime).hasMatch() || regex.match(wakeUpTime).hasMatch() ||
                                regex.match(temperature).hasMatch();
                    } else {
                        Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
                        match = mood.contains(query, cs) || freeMoodText.contains(query, cs) ||
                                sleepTime.contains(query, cs) || wakeUpTime.contains(query, cs) ||
                                temperature.contains(query, cs);
                    }

                    if (match) {
                        QJsonObject result;
                        result["type"] = "status";
                        result["mood"] = mood;
                        result["freeMoodText"] = freeMoodText;
                        result["sleepTime"] = sleepTime;
                        result["wakeUpTime"] = wakeUpTime;
                        result["temperature"] = temperature;
                        result["date"] = createdAt;
                        resultsArray.append(result);
                    }
                }
            }
        }

        if (s == "all"){
            if (hasDateCond) {
                const QString qsql = QString(
                    "SELECT type, created_at_local, id, title, content_path, "
                    "       mood, free_mood_text, sleep_time, wake_up_time, temperature "
                    "FROM ( "
                    "  SELECT 'diary' AS type, datetime(created_at, 'localtime') AS created_at_local, "
                    "         id, title, content_path, NULL AS mood, NULL AS free_mood_text, "
                    "         NULL AS sleep_time, NULL AS wake_up_time, NULL AS temperature "
                    "  FROM diary WHERE date(datetime(created_at, 'localtime')) %1 ? "
                    "  UNION ALL "
                    "  SELECT 'status', datetime(created_at, 'localtime'), "
                    "         id, NULL, NULL, mood, free_mood_text, sleep_time, wake_up_time, temperature "
                    "  FROM user_status WHERE date(datetime(created_at, 'localtime')) %1 ? "
                    ") u "
                    "ORDER BY created_at_local DESC").arg(op);
                SQLite::Statement stmt(db, qsql.toStdString());
                stmt.bind(1, dateStr.toStdString());
                stmt.bind(2, dateStr.toStdString());
                while (stmt.executeStep()) {
                    const QString type = QString::fromStdString(stmt.getColumn(0).getText());
                    const QString createdAt = QString::fromStdString(stmt.getColumn(1).getText());
                    QJsonObject result;
                    result["type"] = type;
                    result["date"] = createdAt;
                    if (type == "diary") {
                        result["title"] = QString::fromStdString(stmt.getColumn(3).getText());
                        result["contentPath"] = QString::fromStdString(stmt.getColumn(4).getText());
                    } else {
                        result["mood"] = stmt.getColumn(5).isNull() ? 0 : stmt.getColumn(5).getInt();
                        result["freeMoodText"] = QString::fromStdString(stmt.getColumn(6).getText());
                        result["sleepTime"] = stmt.getColumn(7).isNull() ? 0.0 : stmt.getColumn(7).getDouble();
                        result["wakeUpTime"] = stmt.getColumn(8).isNull() ? 0.0 : stmt.getColumn(8).getDouble();
                        result["temperature"] = stmt.getColumn(9).isNull() ? 0.0 : stmt.getColumn(9).getDouble();
                    }
                    resultsArray.append(result);
                }
            } else {
                // Fallback: return flat list (you can swap to grouped if desired)
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
                    const QString type = QString::fromStdString(stmt.getColumn(0).getText());
                    const QString createdAt = QString::fromStdString(stmt.getColumn(1).getText());
                    QJsonObject result;
                    result["type"] = type;
                    result["date"] = createdAt;
                    if (type == "diary") {
                        result["title"] = QString::fromStdString(stmt.getColumn(3).getText());
                        result["contentPath"] = QString::fromStdString(stmt.getColumn(4).getText());
                    } else {
                        result["mood"] = stmt.getColumn(5).isNull() ? 0 : stmt.getColumn(5).getInt();
                        result["freeMoodText"] = QString::fromStdString(stmt.getColumn(6).getText());
                        result["sleepTime"] = stmt.getColumn(7).isNull() ? 0.0 : stmt.getColumn(7).getDouble();
                        result["wakeUpTime"] = stmt.getColumn(8).isNull() ? 0.0 : stmt.getColumn(8).getDouble();
                        result["temperature"] = stmt.getColumn(9).isNull() ? 0.0 : stmt.getColumn(9).getDouble();
                    }
                    resultsArray.append(result);
                }
            }
        }

    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while performing search:" << e.what();
    }

    QJsonObject result;
    result["results"] = resultsArray;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}

QString UserSql::searchGroupedAll(SQLite::Database &db) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return QString();
    }

    QMap<QString, QJsonArray> buckets;
    try{
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
            "ORDER BY created_at_local DESC"
        );

        while (stmt.executeStep()){
            const QString type = QString::fromStdString(stmt.getColumn(0).getText());
            const QString createdAt = QString::fromStdString(stmt.getColumn(1).getText());
            const int id = stmt.getColumn(2).getInt();

            QJsonObject entry;
            entry["type"] = type;
            entry["id"] = id;
            entry["createdAt"] = createdAt;

            if (type == "diary") {
                entry["title"] = QString::fromStdString(stmt.getColumn(3).getText());
                entry["contentPath"] = QString::fromStdString(stmt.getColumn(4).getText());
            } else if (type == "status") {
                QJsonObject st;
                entry["mood"] = stmt.getColumn(5).getInt();
                entry["freeMoodText"] = QString::fromStdString(stmt.getColumn(6).getText());
                entry["sleepTime"] = stmt.getColumn(7).getDouble();
                entry["wakeUpTime"] = stmt.getColumn(8).getDouble();
                entry["temperature"] = stmt.getColumn(9).getDouble();
                entry["status"] = st;
            }

            const QString dateKey = createdAt.left(10);
            buckets[dateKey].append(entry);
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while grouping search results:" << e.what();
    }

    QStringList dates = buckets.keys();
    std::sort(dates.begin(), dates.end(), std::greater<QString>());

    QJsonArray groupedResults;
    for (const QString &date : dates) {
        QJsonObject g;
        g["date"] = date;
        g["entries"] = buckets.value(date);
        groupedResults.append(g);
    }

    QJsonObject out;
    out["groupedResults"] = groupedResults;
    return QJsonDocument(out).toJson(QJsonDocument::Indented);
}

QVariant UserSql::getGraphData(int scope, int filter, const QString& start_date, const QString& end_date) const {
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
	QDate to = QDate::fromString(end_date, "yyyy-MM-dd");
    if (!from.isValid() || !to.isValid()) {
        qWarning() << "Invalid date range:" << start_date << "to" << end_date;
        root.insert("start", start_out);
        root.insert("end", end_out);
        root.insert("days", QVariantList{});

		return root;
	}

    if (from > to) std::swap(from, to);

	start_out = from.toString("yyyy-MM-dd");
	end_out = to.toString("yyyy-MM-dd");

    QVariantList days_list;
    try {
		SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        db.setBusyTimeout(3000);

        QSet<QString> diary_dates;
		SQLite::Statement diary_query(db, "SELECT DISTINCT date(created_at) FROM diary WHERE date(created_at) BETWEEN ? AND ?");
        diary_query.bind(1, from.toString("yyyy-MM-dd").toStdString());
        diary_query.bind(2, to.toString("yyyy-MM-dd").toStdString());
        while (diary_query.executeStep()) {
            diary_dates.insert(QString::fromStdString(diary_query.getColumn(0).getText()));
		}

        QHash<QString, QVariantMap> status_data;

        QString status_sq1;
        if (scope == 0) {
			status_sq1 = "SELECT date(created_at), mood, free_mood_text, sleep_time, wake_up_time, temperature FROM user_status WHERE date(created_at) BETWEEN ? AND ?";
		}
        else if (scope == 1) {
			status_sq1 = "SELECT date(created_at), sleep_time, wake_up_time FROM user_status WHERE date(created_at) BETWEEN ? AND ?";
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
            }
            else {
                data.insert("mood", status_query.getColumn(1).getInt());
                data.insert("free_mood_text", QString::fromStdString(status_query.getColumn(2).getText()));
                data.insert("sleep_time", status_query.getColumn(3).getDouble());
                data.insert("wake_up_time", status_query.getColumn(4).getDouble());
                data.insert("temperature", status_query.getColumn(5).getDouble());
            }
			status_data.insert(date, data);
        }

        for (QDate cursor = from; cursor <= to; cursor = cursor.addDays(1)) {
            QString dataStr = cursor.toString("yyyy-MM-dd");
            QVariantMap day_data;
			day_data.insert("date", dataStr);
            day_data.insert("diary_exists", diary_dates.contains(dataStr));

            if (status_data.contains(dataStr)) {
                day_data.insert("status", status_data.value(dataStr));
            }
            else {
                day_data.insert("status", QVariant());
            }
			days_list.append(day_data);
        }
    } catch (const SQLite::Exception &e) {
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
