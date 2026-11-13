#pragma once

#include "common/Types.hpp"
#include <string>
#include <filesystem>

namespace loj {

enum class CheckMode {
    TOKEN_DIFF,      // Standard token-by-token comparison (whitespace/newline agnostic)
    EXACT_DIFF,      // Exact binary string match
    CUSTOM_CHECKER   // Custom binary checker execution
};

class CheckerService {
public:
    CheckerService() = default;
    ~CheckerService() = default;

    // Checks contestant output against expected output
    bool check(const std::string& contestantOutput,
               const std::string& expectedOutput,
               CheckMode mode = CheckMode::TOKEN_DIFF,
               const std::filesystem::path& customCheckerExe = "",
               const std::filesystem::path& inputFilePath = "");

private:
    static bool tokenDiff(const std::string& contestant, const std::string& expected);
    static bool exactDiff(const std::string& contestant, const std::string& expected);
    static bool runCustomChecker(const std::filesystem::path& checkerExe,
                                 const std::filesystem::path& inputFile,
                                 const std::string& contestantOutput,
                                 const std::string& expectedOutput);
};

} // namespace loj
