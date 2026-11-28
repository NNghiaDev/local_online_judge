#pragma once

#include "common/Types.hpp"
#include <vector>
#include <mutex>
#include <optional>
#include <string>

namespace loj {

class SubmissionDAO {
public:
    static SubmissionDAO& instance();

    // Session Management
    int getCurrentSessionId() const;
    std::string getCurrentSessionName() const;
    int startNewSession(const std::string& name = "");
    void completeCurrentSession();
    bool setCurrentSession(int sessionId);
    bool deleteSession(int sessionId);
    std::vector<SessionInfo> getAllSessions();

    // Submissions CRUD
    bool saveSubmission(SubmissionSummary& submission);
    bool loadFromDatabase();
    std::vector<SubmissionSummary> getAllSubmissions();
    std::vector<SubmissionSummary> getSubmissionsBySession(int sessionId);
    std::vector<SubmissionSummary> getSubmissionsByContestant(const std::string& studentId);
    std::vector<SubmissionSummary> getSubmissionsByProblem(const std::string& problemCode);
    std::optional<SubmissionSummary> getSubmissionById(int id);
    bool deleteSubmission(int id);
    bool deleteContestant(const std::string& studentId);
    void clearAll();

private:
    SubmissionDAO();
    ~SubmissionDAO() = default;

    std::string generateSessionDefaultName(int id);
    void saveCurrentSessionState();
    void loadCurrentSessionState();

    mutable std::mutex m_mutex;
    std::vector<SubmissionSummary> m_localSubmissions;
    std::vector<SessionInfo> m_sessions;
    int m_currentSessionId = 1;
    std::string m_currentSessionName = "Phiên #1";
    int m_nextId = 1;
};

} // namespace loj
