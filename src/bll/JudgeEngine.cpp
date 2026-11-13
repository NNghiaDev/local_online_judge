#include "JudgeEngine.hpp"
#include "sandbox/ProcessSandbox.hpp"
#include "strategies/IGradingStrategy.hpp"
#include "dal/db/SubmissionDAO.hpp"
#include "common/Logger.hpp"

#include <iostream>
#include <algorithm>

namespace loj {

namespace fs = std::filesystem;

JudgeEngine& JudgeEngine::instance() {
    static JudgeEngine s_instance;
    return s_instance;
}

JudgeEngine::JudgeEngine() = default;

void JudgeEngine::setProgressCallback(std::function<void(const std::string&, int, int)> callback) {
    m_progressCallback = std::move(callback);
}

SubmissionSummary JudgeEngine::gradeSingleFile(const fs::path& sourceFile,
                                             const std::string& studentId,
                                             const std::string& studentName,
                                             const ProblemSpec& problem,
                                             const std::vector<TestcaseData>& testcases) {
    SubmissionSummary sub;
    sub.studentId = studentId.empty() ? "Anonymous" : studentId;
    sub.studentName = studentName.empty() ? sub.studentId : studentName;
    sub.problemId = problem.id;
    sub.problemCode = problem.problemCode;
    sub.sourceFilePath = sourceFile.string();
    sub.gradingMode = problem.gradingMode;
    sub.overallVerdict = Verdict::PENDING;

    Logger::instance().info("Starting evaluation: " + sub.studentId + " (" + sub.studentName + ") on " +
                            sub.problemCode + " [" + gradingModeToString(problem.gradingMode) + "]");

    // 1. Compilation Phase
    CompilationResult comp = m_compiler.compile(sourceFile);
    sub.compilerOutput = comp.compilerOutput;

    if (!comp.success) {
        sub.overallVerdict = Verdict::CE;
        sub.totalScore = 0.0;
        Logger::instance().error("Compilation Error (CE): " + sourceFile.filename().string());
        SubmissionDAO::instance().saveSubmission(sub);
        return sub;
    }

    // 2. Select Grading Strategy
    auto strategy = GradingStrategyFactory::create(problem.gradingMode);

    // 3. Testcase Execution Phase
    int totalCount = static_cast<int>(testcases.size());

    for (int i = 0; i < totalCount; ++i) {
        const auto& tc = testcases[i];
        if (m_progressCallback) {
            m_progressCallback(problem.problemCode, i + 1, totalCount);
        }

        TestcaseResult tr;
        tr.testcaseIndex = tc.index;

        ExecutionConfig cfg;
        cfg.executablePath = comp.outputExecutablePath;
        cfg.workingDirectory = sourceFile.parent_path();
        cfg.inputData = tc.inputContent;
        cfg.timeLimitMs = problem.timeLimitMs;
        cfg.memoryLimitMb = problem.memoryLimitMb;

        // Run binary in isolated Windows Job Object sandbox
        RawExecutionResult raw = ProcessSandbox::execute(cfg);

        tr.executionTimeMs = raw.executionTimeMs;
        tr.peakMemoryKb = raw.peakMemoryKb;
        tr.contestantOutput = raw.stdOut;
        tr.errorMessage = raw.diagnosticMessage;

        sub.totalTimeMs += tr.executionTimeMs;
        if (tr.peakMemoryKb > sub.maxMemoryKb) {
            sub.maxMemoryKb = tr.peakMemoryKb;
        }

        if (raw.verdict != Verdict::AC) {
            tr.verdict = raw.verdict;
            tr.score = 0.0;
        } else {
            // Check contestant output against expected
            CheckMode mode = problem.customCheckerPath.empty() ? CheckMode::TOKEN_DIFF : CheckMode::CUSTOM_CHECKER;
            bool isCorrect = m_checker.check(
                raw.stdOut,
                tc.expectedOutputContent,
                mode,
                problem.customCheckerPath,
                tc.inputFilePath
            );

            if (isCorrect) {
                tr.verdict = Verdict::AC;
            } else {
                tr.verdict = Verdict::WA;
                tr.score = 0.0;
                tr.errorMessage = "Output mismatch";
            }
        }

        Logger::instance().info("  Test #" + std::to_string(tr.testcaseIndex) + ": " +
                                verdictToString(tr.verdict) + " (" +
                                std::to_string(tr.executionTimeMs) + "ms, " +
                                std::to_string(tr.peakMemoryKb) + "KB)");

        sub.testcaseResults.push_back(tr);

        // If the strategy supports fail-fast (e.g. ICPC), break on first error
        if (strategy->canFailFast() && tr.verdict != Verdict::AC) {
            Logger::instance().info("  Fail-fast triggered by ICPC grading mode.");
            break;
        }
    }

    // Clean up compiled executable
    std::error_code ec;
    fs::remove(comp.outputExecutablePath, ec);

    // 4. Final Verdict & Score Calculation via Grading Strategy
    strategy->evaluate(sub, testcases, problem);

    Logger::instance().success("Verdict: " + verdictToString(sub.overallVerdict) + " (" +
                               std::to_string(static_cast<int>(sub.totalScore)) + "/" +
                               std::to_string(static_cast<int>(problem.totalScore)) + " pts)");

    // Persist to DAL
    SubmissionDAO::instance().saveSubmission(sub);
    return sub;
}

std::vector<SubmissionSummary> JudgeEngine::gradeContestantFolder(
    const ContestantFolder& contestant,
    const std::vector<ProblemSpec>& problems,
    const fs::path& testcasesRootDir) {

    std::vector<SubmissionSummary> results;
    Logger::instance().info("=== Đang chấm bài cho thí sinh: " + contestant.studentName + " (MSSV: " + contestant.studentId + ") ===");
    Logger::instance().info("Tìm thấy " + std::to_string(contestant.sourceFiles.size()) + " file mã nguồn bài tập trong thư mục.");

    for (const auto& sourceFile : contestant.sourceFiles) {
        std::string stem = sourceFile.stem().string();

        // 1. Tìm cấu hình bài toán có sẵn (nếu có)
        ProblemSpec effectiveProblem;
        bool foundPreconfigured = false;

        for (const auto& p : problems) {
            std::string codeLower = p.problemCode;
            std::string stemLower = stem;
            std::transform(codeLower.begin(), codeLower.end(), codeLower.begin(), ::tolower);
            std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);
            if (stemLower == codeLower) {
                effectiveProblem = p;
                foundPreconfigured = true;
                break;
            }
        }

        // 2. Nếu bài toán chưa cấu hình thủ công, tự động dò tìm thư mục testcase tương ứng
        fs::path probTestDir = testcasesRootDir / stem;
        if (!fs::exists(probTestDir)) {
            // Thử tìm case-insensitive trong thư mục testcases
            if (fs::exists(testcasesRootDir) && fs::is_directory(testcasesRootDir)) {
                for (const auto& dirEntry : fs::directory_iterator(testcasesRootDir)) {
                    if (dirEntry.is_directory()) {
                        std::string dirNameLower = dirEntry.path().filename().string();
                        std::string stemLower = stem;
                        std::transform(dirNameLower.begin(), dirNameLower.end(), dirNameLower.begin(), ::tolower);
                        std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);
                        if (dirNameLower == stemLower) {
                            probTestDir = dirEntry.path();
                            break;
                        }
                    }
                }
            }
        }

        if (!fs::exists(probTestDir)) {
            Logger::instance().warning("Bỏ qua bài [" + sourceFile.filename().string() +
                                       "]: Không tìm thấy thư mục testcase tương ứng tại: " + probTestDir.string());
            continue;
        }

        if (!foundPreconfigured) {
            // Kế thừa thông số thời gian/bộ nhớ mặc định từ problem đầu tiên nếu có
            if (!problems.empty()) {
                effectiveProblem = problems.front();
            }
            effectiveProblem.problemCode = stem;
            effectiveProblem.problemName = stem;
        }

        auto testcases = FileSystemDAO::instance().loadTestcases(probTestDir);
        if (testcases.empty()) {
            Logger::instance().warning("Thư mục testcase cho bài [" + stem + "] trống: " + probTestDir.string());
            continue;
        }

        Logger::instance().info("-> Tiến hành chấm bài: [" + stem + "] (" + std::to_string(testcases.size()) + " testcases)...");
        auto sub = gradeSingleFile(sourceFile, contestant.studentId, contestant.studentName, effectiveProblem, testcases);
        results.push_back(sub);
    }

    return results;
}

std::vector<SubmissionSummary> JudgeEngine::gradeBatch(
    const fs::path& batchRootDir,
    const std::vector<ProblemSpec>& problems,
    const fs::path& testcasesRootDir) {

    std::vector<SubmissionSummary> allResults;
    auto contestants = FileSystemDAO::instance().scanBatchDirectory(batchRootDir);

    Logger::instance().info("Starting batch grading of " + std::to_string(contestants.size()) + " contestants...");

    for (const auto& c : contestants) {
        auto res = gradeContestantFolder(c, problems, testcasesRootDir);
        allResults.insert(allResults.end(), res.begin(), res.end());
    }

    Logger::instance().success("Batch grading finished. Total submissions processed: " + std::to_string(allResults.size()));
    return allResults;
}

} // namespace loj
