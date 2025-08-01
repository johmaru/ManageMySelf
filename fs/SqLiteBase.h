//
// Created By Johma 2025/08/01.
//

#ifndef MANAGEMYSELF_SQLITEBASE_H
#define MANAGEMYSELF_SQLITEBASE_H

#include <QString>

class SqLiteBase {
public:
    virtual ~SqLiteBase() = default;


    // Method to get the file path of the SQLite database
    [[nodiscard]] QString getMainDatabasePath() const;

    // Method to check if the main database not exists then create it
    int checkMainDatabaseAndCreate() const;

    // Method to get recent files from the database
    int addRecentFile(const QString &filePath) const;

    // Method to retrieve a list of recent files from the database
    QStringList getRecentFiles(int limit = 10) const;
};

#endif // MANAGEMYSELF_SQLITEBASE_H