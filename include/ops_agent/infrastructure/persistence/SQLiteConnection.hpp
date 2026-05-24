#pragma once

#include <mutex>
#include <string>

#if defined(OPS_AGENT_HAS_SQLITE)
#include <sqlite3.h>
#endif

namespace ops_agent::infrastructure::persistence {

class SQLiteConnection {
public:
    explicit SQLiteConnection(std::string path);
    ~SQLiteConnection();

    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    void initialize();

private:
    std::string path_;
#if defined(OPS_AGENT_HAS_SQLITE)
    sqlite3* db_{nullptr};
#endif
    std::mutex mutex_;
};

} // namespace ops_agent::infrastructure::persistence
