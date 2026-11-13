#pragma once

#include "IGradingStrategy.hpp"

namespace loj {

class IOIGradingStrategy : public IGradingStrategy {
public:
    IOIGradingStrategy() = default;
    ~IOIGradingStrategy() override = default;

    void evaluate(SubmissionSummary& submission,
                  const std::vector<TestcaseData>& testcases,
                  const ProblemSpec& problem) override;

    bool canFailFast() const override { return false; } // Must run all subtasks for partial score

    GradingMode getMode() const override { return GradingMode::IOI; }
};

} // namespace loj
