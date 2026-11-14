#include "IOIGradingStrategy.hpp"
#include <map>
#include <cmath>

namespace loj {

void IOIGradingStrategy::evaluate(SubmissionSummary& submission,
                                  const std::vector<TestcaseData>& testcases,
                                  const ProblemSpec& problem) {
    if (submission.testcaseResults.empty() || testcases.empty()) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = 0.0;
        return;
    }

    double totalAwarded = 0.0;
    int passedCount = 0;
    Verdict firstFailingVerdict = Verdict::AC;

    for (size_t i = 0; i < submission.testcaseResults.size(); ++i) {
        auto& tr = submission.testcaseResults[i];
        double maxPts = (i < testcases.size()) ? testcases[i].maxPoints : 0.0;
        if (maxPts <= 0.0) {
            maxPts = problem.totalScore / static_cast<double>(testcases.size());
        }

        if (tr.verdict == Verdict::AC) {
            tr.score = maxPts;
            totalAwarded += maxPts;
            passedCount++;
        } else {
            tr.score = 0.0;
            if (firstFailingVerdict == Verdict::AC) {
                firstFailingVerdict = tr.verdict;
            }
        }
    }

    submission.totalScore = totalAwarded;

    // Xét verdict toàn bài
    if (passedCount == static_cast<int>(testcases.size()) || totalAwarded >= problem.totalScore - 1e-4) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = problem.totalScore;
    } else {
        submission.overallVerdict = (firstFailingVerdict != Verdict::AC) ? firstFailingVerdict : Verdict::WA;
    }
}

} // namespace loj
