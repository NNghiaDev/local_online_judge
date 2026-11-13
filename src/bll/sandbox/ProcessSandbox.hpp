#pragma once

#include "common/Types.hpp"
#include <string>
#include <cstdint>
#include <filesystem>

namespace loj {

struct ExecutionConfig {
    std::filesystem::path executablePath;
    std::filesystem::path workingDirectory;
    std::string inputData;
    uint32_t timeLimitMs = 1000;
    uint32_t memoryLimitMb = 256;
};

struct RawExecutionResult {
    Verdict verdict = Verdict::AC;
    uint32_t executionTimeMs = 0;
    uint32_t peakMemoryKb = 0;
    uint32_t exitCode = 0;
    std::string stdOut;
    std::string stdErr;
    std::string diagnosticMessage;
};

class ProcessSandbox {
public:
    ProcessSandbox() = default;
    ~ProcessSandbox() = default;

    // Executes a binary inside a Windows Job Object sandbox with anonymous pipes
    static RawExecutionResult execute(const ExecutionConfig& config);

private:
#if defined(_WIN32)
    static void readPipeFully(void* hPipe, std::string& out);
    static void writePipeFully(void* hPipe, const std::string& data);
#endif
};

} // namespace loj
