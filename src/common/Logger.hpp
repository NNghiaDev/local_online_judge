#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <iostream>
#include <functional>

namespace loj {

enum class LogLevel {
    INFO,
    SUCCESS,
    WARNING,
    ERR
};

struct LogEntry {
    LogLevel level;
    std::string timestamp;
    std::string message;
};

class Logger {
public:
    static Logger& instance();

    void log(LogLevel level, const std::string& message);
    void info(const std::string& message);
    void success(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    std::vector<LogEntry> getLogs();
    void clear();

    void setListener(std::function<void(const LogEntry&)> listener);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    std::string getCurrentTime();

    std::mutex m_mutex;
    std::vector<LogEntry> m_entries;
    std::function<void(const LogEntry&)> m_listener;
};

} // namespace loj
