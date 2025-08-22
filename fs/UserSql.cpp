#include "UserSql.h"
#include <qcontainerfwd.h>
#include <qdebug.h>
#include <SQLiteCpp/Database.h>
#include <qdir.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>


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

        // 日記のテーブルを作成
        db.exec("CREATE TABLE IF NOT EXISTS diary ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "title TEXT NOT NULL UNIQUE, "
                "content_path TEXT NOT NULL, "
                "last_modified DATETIME DEFAULT CURRENT_TIMESTAMP, "
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

        qInfo() << "Diary table ensured in the user database.";

        db.exec("CREATE TABLE IF NOT EXISTS user_status ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                "mood INTEGER NOT NULL, "
                "free_mood_text TEXT NOT NULL, "
                "sleep_time INTEGER NOT NULL, "
                "wake_up_time INTEGER NOT NULL, "
                "temperature INTEGER, "
                "last_modified DATETIME DEFAULT CURRENT_TIMESTAMP, "
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating user database:" << e.what();
        return static_cast<int>(UserSql::UserSqlError::SQLiteError);
    }

    dir.mkdir("diaries"); // 日記用のディレクトリを作成
    qInfo() << "Diaries directory created at:" << dir.filePath("diaries");

    return static_cast<int>(UserSql::UserSqlError::NoError);
}

int UserSql::createDiary(const QString &title,const QString &path) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return static_cast<int>(UserSql::UserSqlError::PathNotSet);
    }

    if (isTodayDiaryExists(QDate::currentDate().toString("yyyy-MM-dd"))) {
        qWarning() << "Diary entry with title" << title << "already exists for today.";
        return static_cast<int>(UserSql::UserSqlError::DiaryAlreadyExists);
    }

    try {

        QDir dir(path);
        if (!dir.exists()) {
            return static_cast<int>(UserSql::UserSqlError::PathDoesNotExist);
        }
        if (!dir.cd("diaries")) {
            qWarning() << "Failed to enter diaries directory at:" << dir.filePath("diaries");
            return static_cast<int>(UserSql::UserSqlError::DirectoryChangeFailed);
        }

        const QString today = QDate::currentDate().toString("yyyy-MM-dd");

        if (!dir.exists(today)) {
            dir.mkdir(today);
        }

        if (!dir.cd(today)) {
            qWarning() << "Failed to enter today's diary directory at:" << dir.filePath(today);
            return static_cast<int>(UserSql::UserSqlError::DirectoryChangeFailed);
        }

        const QString diaryFilePath = dir.filePath(title + ".md");

        QFile diaryFile(diaryFilePath);
        if (!diaryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to create diary file at:" << diaryFilePath;
            return static_cast<int>(UserSql::UserSqlError::DiaryFileCreationFailed);
        }
        diaryFile.close();
        qInfo() << "Diary file created at:" << diaryFilePath;

        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO diary (title, content_path) VALUES (?, ?)");
        query.bind(1, title.toStdString());
        query.bind(2, diaryFilePath.toStdString());
        query.exec();
        qInfo() << "Diary entry created:" << title;

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
        query.bind(1, 0);
        query.bind(2, QString().toStdString());
        query.bind(3, 0);
        query.bind(4, 0);
        query.bind(5, 0);
        query.exec();
        qInfo() << "Status entry created.";
        return static_cast<int>(UserSql::UserSqlError::NoError);
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating status entry:" << e.what();
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
            "SELECT mood, free_mood_text, datetime(created_at, 'localtime') AS created_at_local "
            "FROM user_status "
            "WHERE strftime('%Y', datetime(created_at, 'localtime')) = ? "
            "AND strftime('%m', datetime(created_at, 'localtime')) = ?"
        );
        query.bind(1, QString::number(year).toStdString());
        query.bind(2, QString::number(month).rightJustified(2, '0').toStdString());

        while (query.executeStep()) {
            QJsonObject statusObject;
            statusObject["mood"] = QString::fromStdString(query.getColumn(0).getText());
            statusObject["freeMoodText"] = QString::fromStdString(query.getColumn(1).getText());
            statusObject["createdAt"] = QString::fromStdString(query.getColumn(2).getText());
            statusArray.append(statusObject);
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while getting user status by month:" << e.what();
    }

    QJsonObject result;
    result["status"] = statusArray;
    return QJsonDocument(result).toJson(QJsonDocument::Indented);
}