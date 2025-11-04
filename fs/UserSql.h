//
// Created By Johma 2025/08/05.
//

#ifndef MANAGEMYSELF_USERSQL_H
#define MANAGEMYSELF_USERSQL_H

#include "fs/SqLiteBase.h"

#include <SQLiteCpp/Database.h>
#include <qdebug.h>
#include <qdir.h>
#include <qobject.h>

class UserSql : public SqLiteBase {
  public:
    explicit UserSql(QString pathToUserDb);

    [[nodiscard]] int createUserDatabase(const QString& path) const;

    static QString getUserDatabasePath(const QString& path) {
        if (path.isEmpty()) {
            qWarning() << "Workspace path is empty. Cannot get user database path.";
            return "";
        }
        QDir dir(path);
        return dir.filePath("user.db");
    }

    [[nodiscard]] QString getPathToUserDb() const {
        return m_pathToUserDb;
    }

    void setPathToUserDb(const QString& path) {
        m_pathToUserDb = path;
    }

    [[nodiscard]] bool isTodayDiaryExists(const QString& date) const;

    [[nodiscard]] bool isDayDiaryExists(int year, int month, int day) const;

    [[nodiscard]] bool isTodayStatusExists(const QString& date) const;

    [[nodiscard]] bool isEqualTodayAndCreateAtForDiary(const QString& date) const;

    [[nodiscard]] bool isEqualTodayAndCreateAtForStatus(const QString& date) const;

    [[nodiscard]] QUrl createDiary(int year, int month, int day, const QString& title,
                                   const QString& path) const;

    [[nodiscard]] int updateDiary(const QString& title, const QString& newTitle,
                                  const QString& newContentPath) const;

    [[nodiscard]] int createStatus() const;

    [[nodiscard]] int editStatus(int year, int month, int day, const QJsonObject& jsonObject) const;

    [[nodiscard]] QString getDiariesByDate(const QString& date) const;

    [[nodiscard]] QString getDiariesByMonthJson(int year, int month) const;

    [[nodiscard]] QString getStatusByMonthJson(int year, int month) const;

    [[nodiscard]] QString search(const QString& query, const QString& scope, bool caseSensitive,
                                 bool useRegex) const;

    [[nodiscard]] QVariant getGraphData(int scope, int filter, const QString& start_date,
                                        const QString& end_date) const;

    // NOLINTNEXTLINE(readability-identifier-length)
    [[nodiscard]] QString sanitizeFileName(const QString& in) const;

  private:
    QString m_pathToUserDb;

    // NOLINTNEXTLINE(readability-identifier-length)
    QString searchGroupedAll(SQLite::Database& db) const;
};

#endif // MANAGEMYSELF_USERSQL_H