#include "UserSql.h"
#include <qdebug.h>
#include <SQLiteCpp/Database.h>
#include <qdir.h>


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
                "content TEXT, "
                "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)");

        qInfo() << "Diary table ensured in the user database.";
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating user database:" << e.what();
        return -2; // エラーコードを返す
    }

    return 0; // 成功コードを返す
}

int UserSql::createDiary(const QString &title, const QString &content) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return -1; // パスが空の場合のエラーコード
    }

    if (getDiariesByDate(QDate::currentDate().toString("yyyy-MM-dd")).contains(QPair<QString, QString>(title, content))) {
        qWarning() << "Diary entry with title" << title << "already exists for today.";
        return -3; // 今日の日記が既に存在する場合のエラーコード
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "INSERT INTO diary (title, content) VALUES (?, ?)");
        query.bind(1, title.toStdString());
        query.bind(2, content.toStdString());
        query.exec();
        qInfo() << "Diary entry created:" << title;
        return 0; // 成功
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while creating diary entry:" << e.what();
        return -2; // エラーコード
    }
}

int UserSql::updateDiary(const QString &title, const QString &content) const {
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return -1; // パスが空の場合のエラーコード
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READWRITE);
        SQLite::Statement query(db, "UPDATE diary SET content = ? WHERE title = ?");
        query.bind(1, content.toStdString());
        query.bind(2, title.toStdString());
        query.exec();
        qInfo() << "Diary entry updated:" << title;
        return 0; // 成功
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while updating diary entry:" << e.what();
        return -2; // エラーコード
    }
}

QList<QPair<QString, QString>> UserSql::getDiariesByDate(const QString &date) const {
    QList<QPair<QString, QString>> diaries;
    if (m_pathToUserDb.isEmpty()) {
        qWarning() << "User database path is not set.";
        return diaries;
    }

    try {
        SQLite::Database db(m_pathToUserDb.toStdString(), SQLite::OPEN_READONLY);
        SQLite::Statement query(db, "SELECT title, content FROM diary WHERE date(created_at) = ?");
        query.bind(1, date.toStdString());

        while (query.executeStep()) {
            diaries.append({
                QString::fromStdString(query.getColumn(0).getString()),
                QString::fromStdString(query.getColumn(1).getString())
            });
        }
    } catch (const SQLite::Exception &e) {
        qWarning() << "SQLite error while getting diaries by date:" << e.what();
    }
    return diaries;
}