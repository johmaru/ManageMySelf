//
// Created By Johma 2025/08/01.
//

#ifndef MANAGEMYSELF_SQLITEBASE_H
#define MANAGEMYSELF_SQLITEBASE_H

#include <QString>

class SqLiteBase {
public:
    virtual ~SqLiteBase() = default;

    enum class UserSqlError {
        NoError = 0,
        PathNotSet = -1,
        DiaryAlreadyExists = -3,
        PathDoesNotExist = -4,
        DirectoryChangeFailed = -5,
        DiaryFileCreationFailed = -6,
        NoStatusEntryFound = -7,
        DiaryFileRenameFailed = -8,
        SQLiteError = -2
    };


    // Method to get the file path of the SQLite database
    [[nodiscard]] QString getMainDatabasePath() const;

    // Method to check if the main database not exists then create it
    int checkMainDatabaseAndCreate() const;

    // Method to get recent files from the database
    int addRecentFile(const QString &filePath) const;

    // Method to retrieve a list of recent files from the database
    QStringList getRecentFiles(int limit = 10) const;

    int ExistCheckWorkspaceAtName(const QString &name) const;

    int ExistCheckWorkspaceAtPath(const QString &path) const;

    QStringList getWorkspaces() const;

    // Method to get a workspace by its name
    // QStringlist in 0 index is the name, 1 index is the path
    QStringList getWorkspaceWithName(const QString &name) const;

    int addWorkspace(const QString &name, const QString &path) const;

    int deleteWorkspace(const QString &name) const;
};

#endif // MANAGEMYSELF_SQLITEBASE_H