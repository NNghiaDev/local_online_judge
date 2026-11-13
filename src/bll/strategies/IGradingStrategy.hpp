#pragma once

#include "common/Types.hpp"
#include <vector>
#include <memory>

namespace loj {

// Strategy Pattern Interface for Grading Modes
class IGradingStrategy {
public:
    virtual ~IGradingStrategy() = default;

    // Evaluates testcase results according to the strategy and computes final score and verdict
    virtual void evaluate(SubmissionSummary& submission,
                          const std::vector<TestcaseData>& testcases,
                          const ProblemSpec& problem) = 0;

    // Returns whether this strategy supports fail-fast (stopping on first failure)
    virtual bool canFailFast() const = 0;

    // Strategy identification
    virtual GradingMode getMode() const = 0;
};

// Factory for creating grading strategies
class GradingStrategyFactory {
public:
    static std::unique_ptr<IGradingStrategy> create(GradingMode mode);
};

} // namespace loj
