#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <chrono>

namespace loj {

enum class Verdict {
    AC,      // Accepted
    WA,      // Wrong Answer
    TLE,     // Time Limit Exceeded
    MLE,     // Memory Limit Exceeded
    RTE,     // Runtime Error
    CE,      // Compilation Error
    PENDING  // In progress
};

inline std::string verdictToString(Verdict v) {
    switch (v) {
        case Verdict::AC:      return "AC";
        case Verdict::WA:      return "WA";
        case Verdict::TLE:     return "TLE";
        case Verdict::MLE:     return "MLE";
        case Verdict::RTE:     return "RTE";
        case Verdict::CE:      return "CE";
        case Verdict::PENDING: return "PENDING";
        default:               return "UNKNOWN";
    }
}

inline Verdict stringToVerdict(const std::string& str) {
    if (str == "AC") return Verdict::AC;
    if (str == "WA") return Verdict::WA;
    if (str == "TLE") return Verdict::TLE;
    if (str == "MLE") return Verdict::MLE;
    if (str == "RTE") return Verdict::RTE;
    if (str == "CE") return Verdict::CE;
    return Verdict::PENDING;
}

enum class GradingMode {
    ICPC,
    IOI,
    EQUAL
};

inline std::string gradingModeToString(GradingMode mode) {
    switch (mode) {
        case GradingMode::ICPC:  return "ICPC";
        case GradingMode::IOI:   return "IOI";
        case GradingMode::EQUAL: return "EQUAL";
        default:                 return "EQUAL";
    }
}

inline GradingMode stringToGradingMode(const std::string& str) {
    if (str == "ICPC") return GradingMode::ICPC;
    if (str == "IOI") return GradingMode::IOI;
    return GradingMode::EQUAL;
}

// Single Testcase data
struct TestcaseData {
    int index = 0;
    std::string inputFilePath;
    std::string expectedOutputFilePath;
    std::string inputContent;
    std::string expectedOutputContent;
    int subtaskGroup = 1;      // Used for IOI subtasks
    double maxPoints = 0.0;    // Configured point weight
};

// Testcase execution result
struct TestcaseResult {
    int testcaseIndex = 0;
    Verdict verdict = Verdict::PENDING;
    uint32_t executionTimeMs = 0;
    uint32_t peakMemoryKb = 0;
    double score = 0.0;
    std::string contestantOutput;
    std::string errorMessage;
};

// Grading Session Info
struct SessionInfo {
    int id = 1;
    std::string sessionName = "Phiên #1";
    std::string createdAt;
    bool isCompleted = false;
};

// Submission entity & result
struct SubmissionSummary {
    int id = 0;
    int sessionId = 1;
    std::string sessionName = "Phiên #1";
    int contestantId = 0;
    std::string studentId;
    std::string studentName;
    int problemId = 0;
    std::string problemCode;
    std::string sourceFilePath;
    Verdict overallVerdict = Verdict::PENDING;
    double totalScore = 0.0;
    uint32_t totalTimeMs = 0;
    uint32_t maxMemoryKb = 0;
    std::string compilerOutput;
    GradingMode gradingMode = GradingMode::EQUAL;
    std::vector<TestcaseResult> testcaseResults;
    std::string submittedAt;
};

// Leaderboard row entry
struct LeaderboardEntry {
    int rank = 1;
    int sessionId = 0; // 0: Tất cả các phiên, hoặc session ID cụ thể
    std::string studentId;
    std::string studentName;
    int solvedCount = 0;
    double totalScore = 0.0;
    uint32_t totalPenaltyTime = 0; // For ICPC mode
    std::vector<std::pair<std::string, double>> problemScores; // problemCode -> score (for IOI/Equal)
    std::vector<std::pair<std::string, bool>> problemSolvedStatus; // problemCode -> isAC (for ICPC)
};

// Problem specifications
struct ProblemSpec {
    int id = 0;
    std::string problemCode;
    std::string problemName;
    uint32_t timeLimitMs = 1000;
    uint32_t memoryLimitMb = 256;
    GradingMode gradingMode = GradingMode::EQUAL;
    double totalScore = 100.0;
    std::string customCheckerPath; // Optional path to custom checker executable
    std::vector<double> ioiPoints; // Configured point weight per testcase for IOI mode
};

} // namespace loj
