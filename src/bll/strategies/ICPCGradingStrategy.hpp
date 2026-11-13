#pragma once

#include "IGradingStrategy.hpp"

namespace loj {

class ICPCGradingStrategy : public IGradingStrategy {
public:
    ICPCGradingStrategy() = default;
    ~ICPCGradingStrategy() override = default;

    void evaluate(SubmissionSummary& submission,
                  const std::vector<TestcaseData>& testcases,
                  const ProblemSpec& problem) override;

    bool canFailFast() const override { return true; }

    GradingMode getMode() const override { return GradingMode::ICPC; }
};

} // namespace loj
