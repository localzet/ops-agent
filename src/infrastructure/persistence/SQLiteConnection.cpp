#include "ops_agent/infrastructure/persistence/SQLiteConnection.hpp"

#include <stdexcept>
#include <utility>

namespace ops_agent::infrastructure::persistence {

SQLiteConnection::SQLiteConnection(std::string path)
    : path_(std::move(path))
{
}

SQLiteConnection::~SQLiteConnection()
{
#if defined(OPS_AGENT_HAS_SQLITE)
    if (db_ != nullptr) {
        sqlite3_close(db_);
    }
#endif
}

void SQLiteConnection::initialize()
{
    std::lock_guard<std::mutex> lock{mutex_};
#if !defined(OPS_AGENT_HAS_SQLITE)
    (void)path_;
    return;
#else
    if (db_ != nullptr) {
        return;
    }

    if (sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        const std::string error = db_ != nullptr ? sqlite3_errmsg(db_) : "unknown sqlite error";
        throw std::runtime_error("failed to open sqlite database: " + error);
    }

    char* error_message = nullptr;
    const char* sql = "CREATE TABLE IF NOT EXISTS schema_migrations ("
                      "version INTEGER PRIMARY KEY,"
                      "applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
                      ");";
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &error_message) != SQLITE_OK) {
        std::string error = error_message != nullptr ? error_message : "unknown sqlite error";
        sqlite3_free(error_message);
        throw std::runtime_error("failed to initialize sqlite database: " + error);
    }
#endif
}

} // namespace ops_agent::infrastructure::persistence
