#include "EqualGradingStrategy.hpp"
#include "ICPCGradingStrategy.hpp"
#include "IOIGradingStrategy.hpp"

namespace loj {

void EqualGradingStrategy::evaluate(SubmissionSummary& submission,
                                   const std::vector<TestcaseData>& testcases,
                                   const ProblemSpec& problem) {
    if (submission.testcaseResults.empty() || testcases.empty()) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = 0.0;
        return;
    }

    int totalCount = static_cast<int>(testcases.size());
    double pointsPerTest = problem.totalScore / static_cast<double>(totalCount);

    double totalAwarded = 0.0;
    int passedCount = 0;
    Verdict firstFailingVerdict = Verdict::AC;

    for (auto& tr : submission.testcaseResults) {
        if (tr.verdict == Verdict::AC) {
            tr.score = pointsPerTest;
            totalAwarded += pointsPerTest;
            passedCount++;
        } else {
            tr.score = 0.0;
            if (firstFailingVerdict == Verdict::AC) {
                firstFailingVerdict = tr.verdict;
            }
        }
    }

    submission.totalScore = totalAwarded;

    if (passedCount == totalCount) {
        submission.overallVerdict = Verdict::AC;
        submission.totalScore = problem.totalScore;
    } else {
        submission.overallVerdict = (firstFailingVerdict != Verdict::AC) ? firstFailingVerdict : Verdict::WA;
    }
}

std::unique_ptr<IGradingStrategy> GradingStrategyFactory::create(GradingMode mode) {
    switch (mode) {
        case GradingMode::ICPC:
            return std::make_unique<ICPCGradingStrategy>();
        case GradingMode::IOI:
            return std::make_unique<IOIGradingStrategy>();
        case GradingMode::EQUAL:
        default:
            return std::make_unique<EqualGradingStrategy>();
    }
}

} // namespace loj
