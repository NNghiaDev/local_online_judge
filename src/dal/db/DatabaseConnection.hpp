#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <atomic>

namespace loj {

struct DbConfig {
    std::string host = "127.0.0.1";
    int port = 3306;
    std::string user = "root";
    std::string password = "";
    std::string database = "local_online_judge";
};

class DatabaseConnection {
public:
    static DatabaseConnection& instance();

    bool connect(const DbConfig& config);
    void disconnect();
    
    // Lock-free check so GUI render loop NEVER blocks
    bool isConnected() const { return m_connected.load(); }
    bool isConnecting() const { return m_isConnecting.load(); }

    const DbConfig& getConfig() const { return m_config; }

    bool execute(const std::string& sqlQuery);

    using Row = std::vector<std::string>;
    using ResultSet = std::vector<Row>;
    ResultSet queryRows(const std::string& sqlQuery);

private:
    DatabaseConnection();
    ~DatabaseConnection();

    DbConfig m_config;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_isConnecting{false};
    mutable std::mutex m_mutex;
    void* m_nativeHandle = nullptr;
};

} // namespace loj
