#include "Logger.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace loj {

Logger& Logger::instance() {
    static Logger s_instance;
    return s_instance;
}

std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &now_c);
#else
    localtime_r(&now_c, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%H:%M:%S");
    return ss.str();
}

void Logger::log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    LogEntry entry{level, getCurrentTime(), message};
    m_entries.push_back(entry);

    // Standard console mirror
    std::cout << "[" << entry.timestamp << "] " << message << "\n";

    if (m_listener) {
        m_listener(entry);
    }
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, "[INFO] " + message);
}

void Logger::success(const std::string& message) {
    log(LogLevel::SUCCESS, "[OK] " + message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, "[WARN] " + message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERR, "[ERROR] " + message);
}

std::vector<LogEntry> Logger::getLogs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_entries;
}

void Logger::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_entries.clear();
}

void Logger::setListener(std::function<void(const LogEntry&)> listener) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listener = std::move(listener);
}

} // namespace loj
