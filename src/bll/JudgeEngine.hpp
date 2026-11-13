#pragma once

#include "common/Types.hpp"
#include "compiler/CompilerService.hpp"
#include "checker/CheckerService.hpp"
#include "dal/filesystem/FileSystemDAO.hpp"
#include <filesystem>
#include <vector>
#include <functional>

namespace loj {

class JudgeEngine {
public:
    static JudgeEngine& instance();

    // Set callback for live grading progress notifications in the GUI
    void setProgressCallback(std::function<void(const std::string&, int, int)> callback);

    // Grade a single source file against a problem specification and testcases
    SubmissionSummary gradeSingleFile(const std::filesystem::path& sourceFile,
                                     const std::string& studentId,
                                     const std::string& studentName,
                                     const ProblemSpec& problem,
                                     const std::vector<TestcaseData>& testcases);

    // Grade an entire contestant folder (contains multiple problem solutions)
    std::vector<SubmissionSummary> gradeContestantFolder(const ContestantFolder& contestant,
                                                        const std::vector<ProblemSpec>& problems,
                                                        const std::filesystem::path& testcasesRootDir);

    // Grade a batch root directory containing multiple contestant folders
    std::vector<SubmissionSummary> gradeBatch(const std::filesystem::path& batchRootDir,
                                             const std::vector<ProblemSpec>& problems,
                                             const std::filesystem::path& testcasesRootDir);

private:
    JudgeEngine();
    ~JudgeEngine() = default;

    CompilerService m_compiler;
    CheckerService m_checker;
    std::function<void(const std::string&, int, int)> m_progressCallback;
};

} // namespace loj
