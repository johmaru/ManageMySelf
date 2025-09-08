#include "Migration.h"
#include <SQLiteCpp/Database.h>

int getUserVersion(SQLite::Database &db) {
    SQLite::Statement query(db, "PRAGMA user_version");
    if (query.executeStep()) {
        return query.getColumn(0).getInt();
    }
    return 0;
}

void setUserVersion(SQLite::Database &db, int version) {
    db.exec(("PRAGMA user_version = " + std::to_string(version)).c_str());
}

void runMigrations(const std::string& dbPath, const std::vector<Migration>& migrations) {
    SQLite::Database db(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    db.setBusyTimeout(3000);
    db.exec("PRAGMA journal_mode = WAL");
    db.exec("PRAGMA foreign_keys = ON");

    int currentVersion = getUserVersion(db);
    for (const auto& m : migrations) {
        if (m.version <= currentVersion) continue;

        db.exec("BEGIN IMMEDIATE");
        try {
            m.up(db);
            setUserVersion(db, m.version);
            db.exec("COMMIT");
        } catch (const SQLite::Exception &e) {
            db.exec("ROLLBACK");
            throw; // Rethrow the exception after rollback
        }
    }
}