#include "UserSql.h"
#include <qdebug.h>
#include <SQLiteCpp/Database.h>
#include <qdir.h>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>


int UserSql::createUserDatabase(const QString &path) const {
    if (path.isEmpty()) {
        qWarning() << "Workspace path has not exists";
        return -1; // Workspace pathが空の場合のエラーコード
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
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

        qInfo() << "Diary table ensured in the user database.";
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating user database:" << e.what();
        return -2; // エラーコードを返す
    }

    dir.mkdir("diaries"); // 日記用のディレクトリを作成
    qInfo() << "Diaries directory created at:" << dir.filePath("diaries");

    return 0; // 成功コードを返す
}

int UserSql::createDiary(const QString &title,const QString &path) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return -1; // パスが空の場合のエラーコード
    }

    if (isTodayDiaryExists(QDate::currentDate().toString("yyyy-MM-dd"))) {
        qWarning() << "Diary entry with title" << title << "already exists for today.";
        return -3; // 今日の日記が既に存在する場合のエラーコード
    }

    try {

        QDir dir(path);
        if (!dir.exists()) {
            return -4; // パスが存在しない場合のエラーコード
        }
        if (!dir.cd("diaries")) {
            qWarning() << "Failed to enter diaries directory at:" << dir.filePath("diaries");
            return -5; // ディレクトリcd失敗のエラーコード
        }

        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO diary (title, content_path) VALUES (?, ?)");
        query.bind(1, title.toStdString());
        query.bind(2, dir.filePath(title + ".md").toStdString());
        query.exec();
        qInfo() << "Diary entry created:" << title;

        QString diaryFilePath = dir.filePath(title + ".md");
        QFile diaryFile(diaryFilePath);
        if (!diaryFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "Failed to create diary file at:" << diaryFilePath;
            return -6; // 日記ファイル作成失敗のエラーコード
        }
        diaryFile.close();
        qInfo() << "Diary file created at:" << diaryFilePath;

        return 0; // 成功
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating diary entry:" << e.what();
        return -2; // エラーコード
    }
}

int UserSql::updateDiary(const QString &title, const QString &newTitle, const QString &newContentPath) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return -1; // パスが空の場合のエラーコード
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
            return -3; // 日記エントリが見つからない場合のエラーコード
        }
        
        QFileInfo fileInfo(oldContentPath);
        QString newPath = fileInfo.dir().filePath(newTitle + ".md");
        
        SQLite::Statement updateQuery(db, "UPDATE diary SET title = ?, content_path = ? WHERE title = ?");
        updateQuery.bind(1, newTitle.toStdString());
        updateQuery.bind(2, newPath.toStdString());
        updateQuery.bind(3, title.toStdString());
        updateQuery.exec();
        
        qInfo() << "Diary entry updated:" << newTitle;
        return 0; // 成功
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while updating diary entry:" << e.what();
        return -2; // エラーコード
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
        SQLite::Statement query(db, "SELECT title, content_path, created_at FROM diary WHERE strftime('%Y', created_at) = ? AND strftime('%m', created_at) = ?");
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