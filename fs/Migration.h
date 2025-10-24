#pragma once
#include <QString>
#include <SQLiteCpp/Database.h>
#include <functional>
#include <vector>


struct Migration {
    int version;
    const char* name;
    std::function<void(SQLite::Database&)> up;
};

// NOLINTNEXTLINE(readability-identifier-length)
int getUserVersion(SQLite::Database& db);
// NOLINTNEXTLINE(readability-identifier-length)
void setUserVersion(SQLite::Database& db, int version);
void runMigrations(const std::string& dbPath, const std::vector<Migration>& migrations);