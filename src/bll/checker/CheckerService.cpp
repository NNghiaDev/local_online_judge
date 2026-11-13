#include "CheckerService.hpp"
#include "sandbox/ProcessSandbox.hpp"
#include "dal/filesystem/FileSystemDAO.hpp"
#include "common/Logger.hpp"

#include <sstream>
#include <vector>

namespace loj {

namespace fs = std::filesystem;

bool CheckerService::check(const std::string& contestantOutput,
                           const std::string& expectedOutput,
                           CheckMode mode,
                           const fs::path& customCheckerExe,
                           const fs::path& inputFilePath) {
    if (mode == CheckMode::CUSTOM_CHECKER && !customCheckerExe.empty() && fs::exists(customCheckerExe)) {
        return runCustomChecker(customCheckerExe, inputFilePath, contestantOutput, expectedOutput);
    }

    if (mode == CheckMode::EXACT_DIFF) {
        return exactDiff(contestantOutput, expectedOutput);
    }

    return tokenDiff(contestantOutput, expectedOutput);
}

bool CheckerService::tokenDiff(const std::string& contestant, const std::string& expected) {
    std::istringstream issContestant(contestant);
    std::istringstream issExpected(expected);

    std::string tokenC, tokenE;
    while (issExpected >> tokenE) {
        if (!(issContestant >> tokenC)) {
            return false; // Contestant output ended prematurely
        }
        if (tokenC != tokenE) {
            return false; // Token mismatch
        }
    }

    // Check if contestant has extra tokens
    if (issContestant >> tokenC) {
        return false;
    }

    return true;
}

bool CheckerService::exactDiff(const std::string& contestant, const std::string& expected) {
    return contestant == expected;
}

bool CheckerService::runCustomChecker(const fs::path& checkerExe,
                                      const fs::path& inputFile,
                                      const std::string& contestantOutput,
                                      const std::string& expectedOutput) {
    try {
        fs::path tempDir = fs::temp_directory_path() / "loj_checker";
        fs::create_directories(tempDir);

        fs::path cOutFile = tempDir / "contestant.out";
        fs::path eOutFile = tempDir / "expected.out";
        FileSystemDAO::writeFile(cOutFile, contestantOutput);
        FileSystemDAO::writeFile(eOutFile, expectedOutput);

        // Custom checker arguments: checker.exe <input> <contestant_output> <expected_output>
        std::string cmd = "\"" + checkerExe.string() + "\" \"" +
                          inputFile.string() + "\" \"" +
                          cOutFile.string() + "\" \"" +
                          eOutFile.string() + "\"";

        ExecutionConfig config;
        config.executablePath = "C:\\Windows\\System32\\cmd.exe /c " + cmd;
        config.workingDirectory = tempDir;
        config.timeLimitMs = 5000;
        config.memoryLimitMb = 256;

        RawExecutionResult res = ProcessSandbox::execute(config);

        std::error_code ec;
        fs::remove(cOutFile, ec);
        fs::remove(eOutFile, ec);

        return (res.verdict == Verdict::AC && res.exitCode == 0);
    } catch (const std::exception& e) {
        Logger::instance().error("Custom checker exception: " + std::string(e.what()));
        return false;
    }
}

} // namespace loj
