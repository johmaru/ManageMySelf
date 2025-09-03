#include "UserSql.h"
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

namespace {

void migrateDiarySchemaIfNeeded(const QString &dbPath) {
    try {
        SQLite::Database db(dbPath.toStdString(), SQLite::OPEN_READWRITE);
        db.setBusyTimeout(3000);

        SQLite::Statement stmt(db, "SELECT sql FROM sqlite_master WHERE type='table' AND name='diary'");
        QString tableSql;
        if (stmt.executeStep() && !stmt.getColumn(0).isNull()) {
            tableSql = QString::fromStdString(stmt.getColumn(0).getText());
        }

        if (tableSql.contains("title TEXT NOT NULL UNIQUE")) {
            db.exec("BEGIN");
            db.exec(
                "CREATE TABLE diary_new ("
                "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "  title TEXT NOT NULL,"
                "  content_path TEXT NOT NULL,"
                "  entry_date TEXT NOT NULL,"
                "  last_modified DATETIME DEFAULT CURRENT_TIMESTAMP,"
                "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
                ")"
            );
            db.exec(
                "INSERT INTO diary_new (id, title, content_path, entry_date, last_modified, created_at) "
                "SELECT id, title, content_path, date(created_at), last_modified, created_at FROM diary"
            );
            db.exec("DROP TABLE diary");
            db.exec("ALTER TABLE diary_new RENAME TO diary");
            db.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_diary_title_entry_date ON diary(title, entry_date)");
            db.exec("COMMIT");
            qInfo() << "Migrated diary table schema to (title, entry_date) unique.";
            return;
        }

        bool hasEntryDate = false;
        {
            SQLite::Statement info(db, "PRAGMA table_info(diary)");
            while (info.executeStep()) {
                const QString colName = QString::fromStdString(info.getColumn(1).getText());
                if (colName == "entry_date") {
                    hasEntryDate = true;
                    break;
                }
            }
        }
        if (!hasEntryDate) {
            db.exec("ALTER TABLE diary ADD COLUMN entry_date TEXT");
            db.exec("UPDATE diary SET entry_date = date(created_at) WHERE entry_date IS NULL");
        }
        db.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_diary_title_entry_date ON diary(title, entry_date)");
    } catch (const SQLite::Exception &e) {
        qWarning() << "Schema migration check failed:" << e.what();
    }
}
} // namespace

int UserSql::createUserDatabase(const QString &path) const {
    if (path.isEmpty()) {
        qWarning() << "Workspace path has not exists";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    QDir dir(path);
    const QString dbFilePath = dir.filePath("user.db");

    try {
        SQLite::Database db(dbFilePath.toStdString(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        qInfo() << "User database created successfully at:" << dbFilePath;

        db.exec(
            "CREATE TABLE IF NOT EXISTS diary ("
            "  id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "  title TEXT NOT NULL, "
            "  content_path TEXT NOT NULL, "
            "  entry_date TEXT NOT NULL, "
            "  last_modified DATETIME DEFAULT CURRENT_TIMESTAMP, "
            "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
        );
        // 複合ユニーク
        db.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_diary_title_entry_date ON diary(title, entry_date)");

        qInfo() << "Diary table ensured in the user database.";

        db.exec("CREATE TABLE IF NOT EXISTS user_status ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "mood INTEGER NOT NULL, "
                "free_mood_text TEXT NOT NULL, "
                "sleep_time REAL NOT NULL, "
                "wake_up_time REAL NOT NULL, "
                "temperature REAL, "
                "last_modified DATETIME DEFAULT CURRENT_TIMESTAMP, "
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating user database:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }

    dir.mkdir("diaries");
    qInfo() << "Diaries directory created at:" << dir.filePath("diaries");
    migrateDiarySchemaIfNeeded(dbFilePath);

    return static_cast<int>(UserSql::UserSqlError::NoError);
}

int UserSql::createDiary(int year, int month, int day, const QString &title, const QString &path) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    migrateDiarySchemaIfNeeded(m_pathToUserDb);

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