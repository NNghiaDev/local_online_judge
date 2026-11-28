#include "SubmissionDAO.hpp"
#include "DatabaseConnection.hpp"
#include "common/Logger.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

namespace loj {

SubmissionDAO::SubmissionDAO() {
    SessionInfo initSession;
    initSession.id = 1;
    initSession.sessionName = "Phiên #1";
    initSession.isCompleted = false;
    m_sessions.push_back(initSession);
    m_currentSessionId = 1;
    m_currentSessionName = "Phiên #1";

    loadCurrentSessionState();
}

SubmissionDAO& SubmissionDAO::instance() {
    static SubmissionDAO s_instance;
    return s_instance;
}

std::string SubmissionDAO::generateSessionDefaultName(int id) {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &now_c);
#else
    localtime_r(&now_c, &tm);
#endif
    std::ostringstream ss;
    ss << "Phiên #" << id << " (" << std::put_time(&tm, "%d/%m %H:%M") << ")";
    return ss.str();
}

int SubmissionDAO::getCurrentSessionId() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSessionId;
}

std::string SubmissionDAO::getCurrentSessionName() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSessionName;
}

void SubmissionDAO::saveCurrentSessionState() {
    try {
        std::ofstream f("active_session.cfg");
        if (f.is_open()) {
            f << m_currentSessionId << "\n";
            f << m_currentSessionName << "\n";
            f.close();
        }
    } catch (...) {}

    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        db.execute("UPDATE grading_sessions SET is_active = 0;");
        db.execute("UPDATE grading_sessions SET is_active = 1 WHERE id = " + std::to_string(m_currentSessionId) + ";");
    }
}

void SubmissionDAO::loadCurrentSessionState() {
    int savedId = 0;
    try {
        std::ifstream f("active_session.cfg");
        if (f.is_open()) {
            f >> savedId;
            f.close();
        }
    } catch (...) {}

    if (savedId > 0) {
        for (const auto& s : m_sessions) {
            if (s.id == savedId) {
                m_currentSessionId = savedId;
                m_currentSessionName = s.sessionName;
                return;
            }
        }
    }
}

int SubmissionDAO::startNewSession(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Hoàn thành phiên cũ
    if (!m_sessions.empty()) {
        m_sessions.back().isCompleted = true;
    }

    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        db.execute("UPDATE grading_sessions SET is_completed = 1 WHERE id = " + std::to_string(m_currentSessionId) + ";");
    }

    int newId = 1;
    for (const auto& s : m_sessions) {
        if (s.id >= newId) newId = s.id + 1;
    }
    std::string sName = name.empty() ? generateSessionDefaultName(newId) : name;

    SessionInfo newSession;
    newSession.id = newId;
    newSession.sessionName = sName;
    newSession.isCompleted = false;
    m_sessions.push_back(newSession);

    m_currentSessionId = newId;
    m_currentSessionName = sName;

    if (db.isConnected()) {
        std::string sql = "INSERT INTO grading_sessions (id, session_name, is_completed, is_active) VALUES (" +
                          std::to_string(newId) + ", '" + sName + "', 0, 1);";
        db.execute(sql);
    }

    saveCurrentSessionState();
    Logger::instance().success("Đã khởi tạo phiên chấm mới: " + sName);
    return newId;
}

bool SubmissionDAO::setCurrentSession(int sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& s : m_sessions) {
        if (s.id == sessionId) {
            m_currentSessionId = s.id;
            m_currentSessionName = s.sessionName;
            saveCurrentSessionState();
            Logger::instance().info("Đã chọn phiên chấm hiện tại: " + s.sessionName);
            return true;
        }
    }
    return false;
}

bool SubmissionDAO::deleteSession(int sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        db.execute("DELETE st FROM submission_testcases st "
                   "JOIN submissions s ON st.submission_id = s.id "
                   "WHERE s.session_id = " + std::to_string(sessionId) + ";");
        db.execute("DELETE FROM submissions WHERE session_id = " + std::to_string(sessionId) + ";");
        db.execute("DELETE FROM grading_sessions WHERE id = " + std::to_string(sessionId) + ";");
    }

    // Xóa trong bộ nhớ cache submissions
    m_localSubmissions.erase(
        std::remove_if(m_localSubmissions.begin(), m_localSubmissions.end(),
                       [sessionId](const SubmissionSummary& s) { return s.sessionId == sessionId; }),
        m_localSubmissions.end()
    );

    // Xóa phiên trong danh sách m_sessions
    m_sessions.erase(
        std::remove_if(m_sessions.begin(), m_sessions.end(),
                       [sessionId](const SessionInfo& s) { return s.id == sessionId; }),
        m_sessions.end()
    );

    // Nếu vừa xóa đúng phiên hiện tại
    if (m_currentSessionId == sessionId) {
        if (!m_sessions.empty()) {
            m_currentSessionId = m_sessions.back().id;
            m_currentSessionName = m_sessions.back().sessionName;
        } else {
            SessionInfo fresh;
            fresh.id = 1;
            fresh.sessionName = "Phiên #1";
            fresh.isCompleted = false;
            m_sessions.push_back(fresh);
            m_currentSessionId = 1;
            m_currentSessionName = "Phiên #1";
            if (db.isConnected()) {
                db.execute("INSERT INTO grading_sessions (id, session_name, is_completed, is_active) VALUES (1, 'Phiên #1', 0, 1);");
            }
        }
    }

    saveCurrentSessionState();
    Logger::instance().success("Đã xóa hoàn toàn phiên #" + std::to_string(sessionId));
    return true;
}

void SubmissionDAO::completeCurrentSession() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_sessions.empty()) {
        m_sessions.back().isCompleted = true;
    }
    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        db.execute("UPDATE grading_sessions SET is_completed = 1 WHERE id = " + std::to_string(m_currentSessionId) + ";");
    }
    Logger::instance().info("Đã hoàn thành phiên chấm hiện tại: " + m_currentSessionName);
}

std::vector<SessionInfo> SubmissionDAO::getAllSessions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions;
}

bool SubmissionDAO::saveSubmission(SubmissionSummary& submission) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (submission.id <= 0) {
        submission.id = m_nextId++;
    }

    submission.sessionId = m_currentSessionId;
    submission.sessionName = m_currentSessionName;

    // Lưu vào bộ nhớ cache
    m_localSubmissions.push_back(submission);

    // Nếu CSDL MySQL/TiDB đang kết nối, ghi trực tiếp
    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        std::ostringstream sqlContestant;
        sqlContestant << "INSERT INTO contestants (student_id, student_name) VALUES ('"
                      << submission.studentId << "', '" << submission.studentName << "') "
                      << "ON DUPLICATE KEY UPDATE student_name='" << submission.studentName << "';";
        db.execute(sqlContestant.str());

        std::ostringstream sqlProblem;
        sqlProblem << "INSERT INTO problems (problem_code, problem_name, grading_mode) VALUES ('"
                   << submission.problemCode << "', '" << submission.problemCode << "', '"
                   << gradingModeToString(submission.gradingMode) << "') "
                   << "ON DUPLICATE KEY UPDATE grading_mode='" << gradingModeToString(submission.gradingMode) << "';";
        db.execute(sqlProblem.str());

        std::ostringstream sqlSub;
        sqlSub << "INSERT INTO submissions (id, session_id, contestant_id, problem_id, source_file, verdict, score, total_time_ms, max_memory_kb, grading_mode) "
               << "VALUES (" << submission.id << ", "
               << submission.sessionId << ", "
               << "(SELECT id FROM contestants WHERE student_id='" << submission.studentId << "'), "
               << "(SELECT id FROM problems WHERE problem_code='" << submission.problemCode << "'), "
               << "'" << submission.sourceFilePath << "', '"
               << verdictToString(submission.overallVerdict) << "', "
               << submission.totalScore << ", "
               << submission.totalTimeMs << ", "
               << submission.maxMemoryKb << ", '"
               << gradingModeToString(submission.gradingMode) << "');";
        db.execute(sqlSub.str());

        for (const auto& tc : submission.testcaseResults) {
            std::ostringstream sqlTc;
            sqlTc << "INSERT INTO submission_testcases (submission_id, testcase_index, verdict, execution_time_ms, peak_memory_kb, score) "
                  << "VALUES (" << submission.id << ", "
                  << tc.testcaseIndex << ", '"
                  << verdictToString(tc.verdict) << "', "
                  << tc.executionTimeMs << ", "
                  << tc.peakMemoryKb << ", "
                  << tc.score << ");";
            db.execute(sqlTc.str());
        }
    }

    Logger::instance().info("Đã lưu bài nộp #" + std::to_string(submission.id) +
                            " [" + submission.studentId + " - " + submission.problemCode + "]: " +
                            verdictToString(submission.overallVerdict) + " (" +
                            std::to_string(static_cast<int>(submission.totalScore)) + " pts) - " + m_currentSessionName);
    return true;
}

bool SubmissionDAO::loadFromDatabase() {
    auto& db = DatabaseConnection::instance();
    if (!db.isConnected()) return false;

    // 1. Tải danh sách phiên chấm (Sessions)
    auto sessionRows = db.queryRows("SELECT id, session_name, created_at, is_completed FROM grading_sessions ORDER BY id ASC;");
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!sessionRows.empty()) {
            m_sessions.clear();
            for (const auto& sr : sessionRows) {
                if (sr.size() < 4) continue;
                SessionInfo s;
                s.id = std::stoi(sr[0]);
                s.sessionName = sr[1];
                s.createdAt = sr[2];
                s.isCompleted = (sr[3] == "1");
                m_sessions.push_back(s);
            }
            // Khôi phục phiên làm việc đang chọn từ CSDL hoặc file cấu hình
            auto activeRows = db.queryRows("SELECT id, session_name FROM grading_sessions WHERE is_active = 1 LIMIT 1;");
            if (!activeRows.empty() && activeRows[0].size() >= 2) {
                m_currentSessionId = std::stoi(activeRows[0][0]);
                m_currentSessionName = activeRows[0][1];
            } else {
                loadCurrentSessionState();
            }
        }
    }

    // 2. Tải danh sách các bài nộp (Submissions)
    std::string sql = 
        "SELECT s.id, s.source_file, s.verdict, s.score, s.total_time_ms, s.max_memory_kb, "
        "s.grading_mode, s.submitted_at, c.student_id, c.student_name, p.id, p.problem_code, s.session_id "
        "FROM submissions s "
        "LEFT JOIN contestants c ON s.contestant_id = c.id "
        "LEFT JOIN problems p ON s.problem_id = p.id "
        "ORDER BY s.id ASC;";

    auto rows = db.queryRows(sql);
    if (rows.empty()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_localSubmissions.clear();

    for (const auto& r : rows) {
        if (r.size() < 12) continue;
        SubmissionSummary sub;
        try {
            sub.id = std::stoi(r[0]);
            sub.sourceFilePath = r[1];
            sub.overallVerdict = stringToVerdict(r[2]);
            sub.totalScore = r[3].empty() ? 0.0 : std::stod(r[3]);
            sub.totalTimeMs = r[4].empty() ? 0 : std::stoul(r[4]);
            sub.maxMemoryKb = r[5].empty() ? 0 : std::stoul(r[5]);
            sub.gradingMode = stringToGradingMode(r[6]);
            sub.submittedAt = r[7];
            sub.studentId = r[8];
            sub.studentName = r[9];
            sub.problemId = r[10].empty() ? 0 : std::stoi(r[10]);
            sub.problemCode = r[11];
            sub.sessionId = (r.size() >= 13 && !r[12].empty()) ? std::stoi(r[12]) : 1;
            
            // Tìm tên phiên tương ứng
            for (const auto& s : m_sessions) {
                if (s.id == sub.sessionId) {
                    sub.sessionName = s.sessionName;
                    break;
                }
            }
            if (sub.sessionName.empty()) {
                sub.sessionName = "Phiên #" + std::to_string(sub.sessionId);
            }
        } catch (...) {
            continue;
        }

        // Tải chi tiết từng testcase
        std::string sqlTc = "SELECT testcase_index, verdict, execution_time_ms, peak_memory_kb, score, error_message "
                            "FROM submission_testcases WHERE submission_id = " + std::to_string(sub.id) +
                            " ORDER BY testcase_index ASC;";
        auto tcRows = db.queryRows(sqlTc);
        for (const auto& tcr : tcRows) {
            if (tcr.size() < 6) continue;
            TestcaseResult tr;
            tr.testcaseIndex = std::stoi(tcr[0]);
            tr.verdict = stringToVerdict(tcr[1]);
            tr.executionTimeMs = tcr[2].empty() ? 0 : std::stoul(tcr[2]);
            tr.peakMemoryKb = tcr[3].empty() ? 0 : std::stoul(tcr[3]);
            tr.score = tcr[4].empty() ? 0.0 : std::stod(tcr[4]);
            tr.errorMessage = tcr[5];
            sub.testcaseResults.push_back(tr);
        }

        m_localSubmissions.push_back(sub);
        if (sub.id >= m_nextId) {
            m_nextId = sub.id + 1;
        }
    }

    Logger::instance().success("Đã đồng bộ " + std::to_string(m_localSubmissions.size()) +
                               " bài nộp từ CSDL MySQL / TiDB Cloud.");
    return true;
}

std::vector<SubmissionSummary> SubmissionDAO::getAllSubmissions() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_localSubmissions;
}

std::vector<SubmissionSummary> SubmissionDAO::getSubmissionsBySession(int sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (sessionId <= 0) {
        return m_localSubmissions;
    }
    std::vector<SubmissionSummary> result;
    for (const auto& s : m_localSubmissions) {
        if (s.sessionId == sessionId) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<SubmissionSummary> SubmissionDAO::getSubmissionsByContestant(const std::string& studentId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SubmissionSummary> result;
    for (const auto& s : m_localSubmissions) {
        if (s.studentId == studentId) {
            result.push_back(s);
        }
    }
    return result;
}

std::vector<SubmissionSummary> SubmissionDAO::getSubmissionsByProblem(const std::string& problemCode) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SubmissionSummary> result;
    for (const auto& s : m_localSubmissions) {
        if (s.problemCode == problemCode) {
            result.push_back(s);
        }
    }
    return result;
}

std::optional<SubmissionSummary> SubmissionDAO::getSubmissionById(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& s : m_localSubmissions) {
        if (s.id == id) {
            return s;
        }
    }
    return std::nullopt;
}

bool SubmissionDAO::deleteSubmission(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_localSubmissions.begin(), m_localSubmissions.end(),
                             [id](const SubmissionSummary& s) { return s.id == id; });
    bool found = (it != m_localSubmissions.end());
    m_localSubmissions.erase(it, m_localSubmissions.end());

    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        db.execute("DELETE FROM submission_testcases WHERE submission_id = " + std::to_string(id) + ";");
        db.execute("DELETE FROM submissions WHERE id = " + std::to_string(id) + ";");
    }
    Logger::instance().info("Đã xóa bài nộp #" + std::to_string(id));
    return found;
}

bool SubmissionDAO::deleteContestant(const std::string& studentId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = std::remove_if(m_localSubmissions.begin(), m_localSubmissions.end(),
                             [&studentId](const SubmissionSummary& s) {
                                 return s.studentId == studentId;
                             });
    bool removedAny = (it != m_localSubmissions.end());
    m_localSubmissions.erase(it, m_localSubmissions.end());

    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        db.execute("DELETE st FROM submission_testcases st "
                   "JOIN submissions s ON st.submission_id = s.id "
                   "JOIN contestants c ON s.contestant_id = c.id "
                   "WHERE c.student_id = '" + studentId + "';");

        db.execute("DELETE s FROM submissions s "
                   "JOIN contestants c ON s.contestant_id = c.id "
                   "WHERE c.student_id = '" + studentId + "';");

        db.execute("DELETE FROM contestants WHERE student_id = '" + studentId + "';");
    }

    Logger::instance().info("Đã xóa toàn bộ dữ liệu của thí sinh [" + studentId + "] khỏi hệ thống.");
    return removedAny;
}

void SubmissionDAO::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_localSubmissions.clear();
    m_nextId = 1;
}

} // namespace loj
