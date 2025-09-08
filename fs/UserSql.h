//
// Created By Johma 2025/08/05.
//

#ifndef MANAGEMYSELF_USERSQL_H
#define MANAGEMYSELF_USERSQL_H

#include "fs/SqLiteBase.h"
#include <qdebug.h>
#include <qdir.h>
#include <qobject.h>
class UserSql : public SqLiteBase {
    public:

        explicit UserSql(const QString &pathToUserDb) : m_pathToUserDb(pathToUserDb) {}

        int createUserDatabase(const QString &path) const;

        static QString getUserDatabasePath(const QString &path) {
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

        void setPathToUserDb(const QString &path) {
            m_pathToUserDb = path;
        }

        bool isTodayDiaryExists(const QString &date) const;

        bool isDayDiaryExists(int year, int month, int day) const;

        bool isTodayStatusExists(const QString &date) const;

        bool isEqualTodayAndCreateAtForDiary(const QString &date) const;

        bool isEqualTodayAndCreateAtForStatus(const QString &date) const;

        int createDiary(int year, int month, int day, const QString &title, const QString &path) const;

        int updateDiary(const QString &title, const QString &newTitle, const QString &newContentPath) const;

        int createStatus() const;

        int editStatus(int year, int month, int day, const QJsonObject &jsonObject) const;

        QString getDiariesByDate(const QString &date) const;

        QString getDiariesByMonthJson(int year, int month) const;

        QString getStatusByMonthJson(int year, int month) const;

private:
      QString m_pathToUserDb;
};

#endif // MANAGEMYSELF_USERSQL_H