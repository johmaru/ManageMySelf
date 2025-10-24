//
// Created By Johma 2025/08/01.
//

#ifndef MANAGEMYSELF_SQLITEBASE_H
#define MANAGEMYSELF_SQLITEBASE_H

#include <QString>
#include <cstdint>

class SqLiteBase {
  public:
    SqLiteBase() = default;
    SqLiteBase(const SqLiteBase&) = default;
    SqLiteBase(SqLiteBase&&) noexcept = default;
    SqLiteBase& operator=(const SqLiteBase&) = default;
    SqLiteBase& operator=(SqLiteBase&&) noexcept = default;
    virtual ~SqLiteBase() = default;

    enum class UserSqlError : std::int8_t {
        NO_ERROR = 0,
        PATH_NOT_SET = -1,
        DIARY_ALREADY_EXISTS = -3,
        PATH_DOES_NOT_EXIST = -4,
        DIRECTORY_CHANGE_FAILED = -5,
        DIARY_FILE_CREATION_FAILED = -6,
        NO_STATUS_ENTRY_FOUND = -7,
        DIARY_FILE_RENAME_FAILED = -8,
        FILE_ERROR = -9,
        SQLITE_ERROR = -2
    };

    // Method to get the file path of the SQLite database
    [[nodiscard]] QString getMainDatabasePath() const;

    // Method to check if the main database not exists then create it
    [[nodiscard]] int checkMainDatabaseAndCreate() const;

    // Method to get recent files from the database
    [[nodiscard]] int addRecentFile(const QString& filePath) const;

    // Method to retrieve a list of recent files from the database
    [[nodiscard]] QStringList getRecentFiles(int limit = 10) const;

    [[nodiscard]] int existCheckWorkspaceAtName(const QString& name) const;

    [[nodiscard]] int existCheckWorkspaceAtPath(const QString& path) const;

    [[nodiscard]] QStringList getWorkspaces() const;

    // Method to get a workspace by its name
    // QStringlist in 0 index is the name, 1 index is the path
    [[nodiscard]] QStringList getWorkspaceWithName(const QString& name) const;

    [[nodiscard]] int addWorkspace(const QString& name, const QString& path) const;

    [[nodiscard]] int deleteWorkspace(const QString& name) const;
};

#endif // MANAGEMYSELF_SQLITEBASE_H