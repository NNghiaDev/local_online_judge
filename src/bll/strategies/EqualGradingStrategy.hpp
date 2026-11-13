#pragma once

#include "IGradingStrategy.hpp"

namespace loj {

class EqualGradingStrategy : public IGradingStrategy {
public:
    EqualGradingStrategy() = default;
    ~EqualGradingStrategy() override = default;

    void evaluate(SubmissionSummary& submission,
                  const std::vector<TestcaseData>& testcases,
                  const ProblemSpec& problem) override;

    bool canFailFast() const override { return false; } // Must run all testcases for partial score

    GradingMode getMode() const override { return GradingMode::EQUAL; }
};

} // namespace loj
