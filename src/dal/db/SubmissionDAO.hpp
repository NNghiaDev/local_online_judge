#pragma once

#include "common/Types.hpp"
#include <vector>
#include <mutex>
#include <optional>

namespace loj {

class SubmissionDAO {
public:
    static SubmissionDAO& instance();

    bool saveSubmission(SubmissionSummary& submission);
    bool loadFromDatabase();
    std::vector<SubmissionSummary> getAllSubmissions();
    std::vector<SubmissionSummary> getSubmissionsByContestant(const std::string& studentId);
    std::vector<SubmissionSummary> getSubmissionsByProblem(const std::string& problemCode);
    std::optional<SubmissionSummary> getSubmissionById(int id);
    bool deleteContestant(const std::string& studentId);
    void clearAll();

private:
    SubmissionDAO() = default;
    ~SubmissionDAO() = default;

    std::mutex m_mutex;
    std::vector<SubmissionSummary> m_localSubmissions;
    int m_nextId = 1;
};

} // namespace loj
