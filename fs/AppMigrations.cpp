#include "AppMigrations.h"

std::vector<Migration> makeMigrations() {
    return {
        {
            1, "init_tables",
            [](SQLite::Database& db) {
                db.exec(
                    "CREATE TABLE IF NOT EXISTS diary ("
                    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "  title TEXT NOT NULL,"
                    "  content_path TEXT NOT NULL,"
                    "  entry_date TEXT,"
                    "  last_modified DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
                );
                db.exec(
                    "CREATE TABLE IF NOT EXISTS user_status ("
                    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "  mood INTEGER NOT NULL,"
                    "  free_mood_text TEXT NOT NULL,"
                    "  sleep_time REAL NOT NULL,"
                    "  wake_up_time REAL NOT NULL,"
                    "  temperature REAL,"
                    "  last_modified DATETIME DEFAULT CURRENT_TIMESTAMP,"
                    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
                );
            }
        },
        {
            2, "backfill_entry_date",
            [](SQLite::Database& db) {
                db.exec("UPDATE diary SET entry_date = date(created_at) WHERE entry_date IS NULL");
            }
        },
        {
            3, "unique_index_title_entry_date",
            [](SQLite::Database& db) {
                db.exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_diary_title_entry_date ON diary(title, entry_date)");
            }
        }
    };
}