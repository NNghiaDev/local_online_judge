#include "DatabaseConnection.hpp"
#include "SubmissionDAO.hpp"
#include "common/Logger.hpp"
#include <iostream>

#if defined(HAS_MYSQL)
#if __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#elif __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#else
#include <mysql.h>
#endif
#endif

namespace loj {

DatabaseConnection::DatabaseConnection() = default;

DatabaseConnection::~DatabaseConnection() {
    disconnect();
}

DatabaseConnection& DatabaseConnection::instance() {
    static DatabaseConnection s_instance;
    return s_instance;
}

bool DatabaseConnection::connect(const DbConfig& config) {
    m_config = config;
    m_isConnecting = true;

#if defined(HAS_MYSQL)
    // Close old handle if any
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_nativeHandle) {
            mysql_close(static_cast<MYSQL*>(m_nativeHandle));
            m_nativeHandle = nullptr;
        }
        m_connected = false;
    }

    MYSQL* mysql = mysql_init(nullptr);
    if (!mysql) {
        Logger::instance().error("Khởi tạo cấu trúc MySQL thất bại");
        m_isConnecting = false;
        return false;
    }

    // Ép TLSv1.2 cho cloud database
    const char* tlsVer = "TLSv1.2";
    mysql_options(mysql, MYSQL_OPT_TLS_VERSION, tlsVer);

    if (config.port != 3306 || config.host.find("cloud") != std::string::npos || config.host.find("tidb") != std::string::npos) {
        my_bool enforce = 1;
        mysql_options(mysql, MYSQL_OPT_SSL_ENFORCE, &enforce);
    }

    // Thiết lập timeout kết nối 5 giây tránh đứng quá lâu nếu sai IP
    unsigned int timeoutSec = 5;
    mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeoutSec);

    Logger::instance().info("Đang kết nối CSDL: " + config.host + ":" + std::to_string(config.port) + "...");

    // Gọi connect NGOÀI mutex để KHÔNG BAO GIỜ làm đơ GUI thread!
    if (!mysql_real_connect(mysql, config.host.c_str(), config.user.c_str(),
                            config.password.c_str(), config.database.c_str(),
                            config.port, nullptr, 0)) {
        std::string err = mysql_error(mysql);
        Logger::instance().error("Kết nối CSDL thất bại: " + err);
        mysql_close(mysql);
        m_isConnecting = false;
        m_connected = false;
        return false;
    }

    // Kết nối thành công, gán handle và chuyển trạng thái
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_nativeHandle = mysql;
        m_connected = true;
    }
    m_isConnecting = false;

    std::string sVer = mysql_get_server_info(mysql);
    Logger::instance().success("Kết nối thành công CSDL [" + config.database + "]! Server: " + sVer);

    // Tự động khởi tạo bảng
    execute("CREATE TABLE IF NOT EXISTS grading_sessions ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "session_name VARCHAR(128) NOT NULL,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
            "is_completed TINYINT DEFAULT 0);");

    execute("CREATE TABLE IF NOT EXISTS contestants ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "student_id VARCHAR(64) NOT NULL UNIQUE,"
            "student_name VARCHAR(128) NOT NULL,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");

    execute("CREATE TABLE IF NOT EXISTS problems ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "problem_code VARCHAR(64) NOT NULL UNIQUE,"
            "problem_name VARCHAR(128) NOT NULL,"
            "time_limit_ms INT NOT NULL DEFAULT 1000,"
            "memory_limit_mb INT NOT NULL DEFAULT 256,"
            "grading_mode ENUM('ICPC', 'IOI', 'EQUAL') NOT NULL DEFAULT 'EQUAL',"
            "total_score DOUBLE NOT NULL DEFAULT 100.0,"
            "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");

    execute("CREATE TABLE IF NOT EXISTS submissions ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "session_id INT NOT NULL DEFAULT 1,"
            "contestant_id INT NOT NULL,"
            "problem_id INT NOT NULL,"
            "source_file VARCHAR(255) NOT NULL,"
            "verdict ENUM('AC', 'WA', 'TLE', 'MLE', 'RTE', 'CE', 'PENDING') NOT NULL DEFAULT 'PENDING',"
            "score DOUBLE NOT NULL DEFAULT 0.0,"
            "total_time_ms INT NOT NULL DEFAULT 0,"
            "max_memory_kb INT NOT NULL DEFAULT 0,"
            "compiler_output TEXT,"
            "grading_mode ENUM('ICPC', 'IOI', 'EQUAL') NOT NULL,"
            "submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");

    // Đảm bảo cột session_id tồn tại nếu bảng đã tạo từ phiên bản trước
    auto cols = queryRows("SHOW COLUMNS FROM submissions LIKE 'session_id'");
    if (cols.empty()) {
        execute("ALTER TABLE submissions ADD COLUMN session_id INT NOT NULL DEFAULT 1;");
    }

    execute("CREATE TABLE IF NOT EXISTS submission_testcases ("
            "id INT AUTO_INCREMENT PRIMARY KEY,"
            "submission_id INT NOT NULL,"
            "testcase_index INT NOT NULL,"
            "verdict ENUM('AC', 'WA', 'TLE', 'MLE', 'RTE', 'CE') NOT NULL,"
            "execution_time_ms INT NOT NULL DEFAULT 0,"
            "peak_memory_kb INT NOT NULL DEFAULT 0,"
            "score DOUBLE NOT NULL DEFAULT 0.0,"
            "error_message TEXT);");

    // Tự động tải lại toàn bộ lịch sử điểm số và bài nộp từ CSDL về Bảng xếp hạng
    SubmissionDAO::instance().loadFromDatabase();

    return true;
#else
    m_isConnecting = false;
    Logger::instance().warning("Chưa có thư viện MySQL; đang chạy ở chế độ lưu trữ cục bộ tạm thời.");
    m_connected = false;
    return true;
#endif
}

void DatabaseConnection::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
#if defined(HAS_MYSQL)
    if (m_nativeHandle) {
        mysql_close(static_cast<MYSQL*>(m_nativeHandle));
        m_nativeHandle = nullptr;
    }
#endif
    m_connected = false;
    m_isConnecting = false;
}

bool DatabaseConnection::execute(const std::string& sqlQuery) {
    std::lock_guard<std::mutex> lock(m_mutex);
#if defined(HAS_MYSQL)
    if (!m_connected || !m_nativeHandle) {
        return false;
    }
    int res = mysql_query(static_cast<MYSQL*>(m_nativeHandle), sqlQuery.c_str());
    if (res != 0) {
        Logger::instance().error("Lỗi SQL: " + std::string(mysql_error(static_cast<MYSQL*>(m_nativeHandle))));
        return false;
    }
    return true;
#else
    (void)sqlQuery;
    return true;
#endif
}

DatabaseConnection::ResultSet DatabaseConnection::queryRows(const std::string& sqlQuery) {
    std::lock_guard<std::mutex> lock(m_mutex);
    ResultSet results;
#if defined(HAS_MYSQL)
    if (!m_connected || !m_nativeHandle) {
        return results;
    }
    MYSQL* mysql = static_cast<MYSQL*>(m_nativeHandle);
    if (mysql_query(mysql, sqlQuery.c_str()) != 0) {
        Logger::instance().error("Lỗi truy vấn SQL: " + std::string(mysql_error(mysql)));
        return results;
    }
    MYSQL_RES* res = mysql_store_result(mysql);
    if (!res) {
        return results;
    }
    unsigned int numFields = mysql_num_fields(res);
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        Row r;
        r.reserve(numFields);
        for (unsigned int i = 0; i < numFields; ++i) {
            r.push_back(row[i] ? row[i] : "");
        }
        results.push_back(std::move(r));
    }
    mysql_free_result(res);
#else
    (void)sqlQuery;
#endif
    return results;
}

} // namespace loj
