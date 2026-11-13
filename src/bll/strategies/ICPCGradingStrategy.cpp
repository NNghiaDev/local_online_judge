#include "ICPCGradingStrategy.hpp"

namespace loj {

void ICPCGradingStrategy::evaluate(SubmissionSummary& submission,
                                   const std::vector<TestcaseData>& testcases,
                                   const ProblemSpec& problem) {
    if (submission.testcaseResults.empty() || testcases.empty()) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = 0.0;
        return;
    }

    bool allAccepted = true;
    Verdict firstFailedVerdict = Verdict::AC;

    for (auto& tcResult : submission.testcaseResults) {
        if (tcResult.verdict != Verdict::AC) {
            allAccepted = false;
            tcResult.score = 0.0;
            if (firstFailedVerdict == Verdict::AC) {
                firstFailedVerdict = tcResult.verdict;
            }
        }
    }

    if (allAccepted && submission.testcaseResults.size() == testcases.size()) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = problem.totalScore;
        // Assign proportional points to each testcase for detailed report
        double pts = problem.totalScore / static_cast<double>(testcases.size());
        for (auto& tr : submission.testcaseResults) {
            tr.score = pts;
        }
    } else {
        submission.overallVerdict = (firstFailedVerdict != Verdict::AC) ? firstFailedVerdict : Verdict::WA;
        submission.totalScore = 0.0;
        for (auto& tr : submission.testcaseResults) {
            tr.score = 0.0;
        }
    }
}

} // namespace loj
