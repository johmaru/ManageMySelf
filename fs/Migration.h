#pragma once
#include <SQLiteCpp/Database.h>
#include <functional>
#include <vector>
#include <QString>

struct Migration {
    int version;
    const char* name;
    std::function<void(SQLite::Database&)> up;
};

int getUserVersion(SQLite::Database& db);
void setUserVersion(SQLite::Database& db, int version);
void runMigrations(const std::string& dbPath, const std::vector<Migration>& migrations);