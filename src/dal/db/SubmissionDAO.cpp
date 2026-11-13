#include "SubmissionDAO.hpp"
#include "DatabaseConnection.hpp"
#include "common/Logger.hpp"
#include <sstream>
#include <iomanip>

namespace loj {

SubmissionDAO& SubmissionDAO::instance() {
    static SubmissionDAO s_instance;
    return s_instance;
}

bool SubmissionDAO::saveSubmission(SubmissionSummary& submission) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (submission.id <= 0) {
        submission.id = m_nextId++;
    }

    // Save to local cache
    m_localSubmissions.push_back(submission);

    // If MySQL connection is active, persist to database
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
        sqlSub << "INSERT INTO submissions (id, contestant_id, problem_id, source_file, verdict, score, total_time_ms, max_memory_kb, grading_mode) "
               << "VALUES (" << submission.id << ", "
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

    Logger::instance().info("Saved submission #" + std::to_string(submission.id) +
                            " [" + submission.studentId + " - " + submission.problemCode + "]: " +
                            verdictToString(submission.overallVerdict) + " (" +
                            std::to_string(static_cast<int>(submission.totalScore)) + " pts)");
    return true;
}

bool SubmissionDAO::loadFromDatabase() {
    auto& db = DatabaseConnection::instance();
    if (!db.isConnected()) return false;

    std::string sql = 
        "SELECT s.id, s.source_file, s.verdict, s.score, s.total_time_ms, s.max_memory_kb, "
        "s.grading_mode, s.submitted_at, c.student_id, c.student_name, p.id, p.problem_code "
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
        } catch (...) {
            continue;
        }

        m_localSubmissions.push_back(sub);
        if (sub.id >= m_nextId) {
            m_nextId = sub.id + 1;
        }
    }

    Logger::instance().success("Đã đồng bộ " + std::to_string(m_localSubmissions.size()) +
                               " bài nộp từ CSDL MySQL / TiDB Cloud vào bảng xếp hạng!");
    return true;
}

std::vector<SubmissionSummary> SubmissionDAO::getAllSubmissions() {
    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        loadFromDatabase();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_localSubmissions;
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

bool SubmissionDAO::deleteContestant(const std::string& studentId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Remove from local memory cache
    auto it = std::remove_if(m_localSubmissions.begin(), m_localSubmissions.end(),
                             [&studentId](const SubmissionSummary& s) {
                                 return s.studentId == studentId;
                             });
    bool removedAny = (it != m_localSubmissions.end());
    m_localSubmissions.erase(it, m_localSubmissions.end());

    // 2. Remove from MySQL / TiDB Cloud
    auto& db = DatabaseConnection::instance();
    if (db.isConnected()) {
        std::string sqlDelSub = "DELETE FROM submissions WHERE contestant_id = "
                                "(SELECT id FROM contestants WHERE student_id = '" + studentId + "');";
        db.execute(sqlDelSub);

        std::string sqlDelContestant = "DELETE FROM contestants WHERE student_id = '" + studentId + "';";
        db.execute(sqlDelContestant);
    }

    Logger::instance().info("Đã xóa toàn bộ dữ liệu của thí sinh [" + studentId + "] khỏi hệ thống và bảng xếp hạng.");
    return removedAny;
}

void SubmissionDAO::clearAll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_localSubmissions.clear();
    m_nextId = 1;
}

} // namespace loj
