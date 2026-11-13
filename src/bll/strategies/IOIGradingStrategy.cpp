#include "IOIGradingStrategy.hpp"
#include <map>

namespace loj {

void IOIGradingStrategy::evaluate(SubmissionSummary& submission,
                                  const std::vector<TestcaseData>& testcases,
                                  const ProblemSpec& problem) {
    if (submission.testcaseResults.empty() || testcases.empty()) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = 0.0;
        return;
    }

    // Map testcase index to subtask group and points
    struct SubtaskInfo {
        int group = 1;
        double maxPoints = 0.0;
        bool allPassed = true;
        int totalTests = 0;
        int passedTests = 0;
    };

    std::map<int, SubtaskInfo> subtasks;

    // Group testcases into subtasks
    for (const auto& tc : testcases) {
        auto& st = subtasks[tc.subtaskGroup];
        st.group = tc.subtaskGroup;
        st.totalTests++;
        st.maxPoints += tc.maxPoints;
    }

    // If subtasks didn't specify custom point weights, divide problem totalScore evenly across subtask groups
    if (!subtasks.empty()) {
        double pointsPerSubtask = problem.totalScore / static_cast<double>(subtasks.size());
        for (auto& [groupId, st] : subtasks) {
            if (st.maxPoints <= 0.0) {
                st.maxPoints = pointsPerSubtask;
            }
        }
    }

    // Build lookup from testcaseIndex to subtask group
    std::map<int, int> tcToGroup;
    for (const auto& tc : testcases) {
        tcToGroup[tc.index] = tc.subtaskGroup;
    }

    Verdict firstFailingVerdict = Verdict::AC;
    for (auto& tr : submission.testcaseResults) {
        int group = tcToGroup.count(tr.testcaseIndex) ? tcToGroup[tr.testcaseIndex] : 1;
        if (tr.verdict == Verdict::AC) {
            subtasks[group].passedTests++;
        } else {
            subtasks[group].allPassed = false;
            if (firstFailingVerdict == Verdict::AC) {
                firstFailingVerdict = tr.verdict;
            }
        }
        tr.score = 0.0; // Subtask points are awarded at the subtask level
    }

    // Calculate total score by summing passing subtasks
    double totalAwarded = 0.0;
    for (const auto& [groupId, st] : subtasks) {
        if (st.allPassed && st.passedTests == st.totalTests && st.totalTests > 0) {
            totalAwarded += st.maxPoints;
            // Distribute points to individual test results for UI feedback
            double ptsPerTc = st.maxPoints / static_cast<double>(st.totalTests);
            for (auto& tr : submission.testcaseResults) {
                if (tcToGroup[tr.testcaseIndex] == groupId) {
                    tr.score = ptsPerTc;
                }
            }
        }
    }

    submission.totalScore = totalAwarded;
    if (totalAwarded >= problem.totalScore - 1e-4) {
        submission.overallVerdict = Verdict::AC;
    } else if (firstFailingVerdict != Verdict::AC) {
        submission.overallVerdict = firstFailingVerdict;
    } else {
        submission.overallVerdict = Verdict::WA;
    }
}

} // namespace loj
