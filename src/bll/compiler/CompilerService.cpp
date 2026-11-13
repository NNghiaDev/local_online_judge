#include "CompilerService.hpp"
#include "sandbox/ProcessSandbox.hpp"
#include "common/Logger.hpp"

#include <chrono>
#include <sstream>

namespace loj {

namespace fs = std::filesystem;

CompilerService::CompilerService() = default;

CompilationResult CompilerService::compile(const fs::path& sourcePath, const fs::path& outputDir) {
    CompilationResult result;
    result.success = false;

    if (!fs::exists(sourcePath)) {
        result.compilerOutput = "Source file does not exist: " + sourcePath.string();
        return result;
    }

    fs::path targetDir = outputDir.empty() ? sourcePath.parent_path() : outputDir;
    if (!fs::exists(targetDir)) {
        fs::create_directories(targetDir);
    }

    std::string baseStem = sourcePath.stem().string();
    fs::path exePath = targetDir / (baseStem + "_exec.exe");
    result.outputExecutablePath = exePath;

    // Remove old executable if present
    if (fs::exists(exePath)) {
        std::error_code ec;
        fs::remove(exePath, ec);
    }

    // Command: g++ -O3 -std=c++20 "source.cpp" -o "binary.exe"
    std::string cmd = m_compilerPath + " -O3 -std=c++20 \"" + sourcePath.string() +
                      "\" -o \"" + exePath.string() + "\"";

    Logger::instance().info("Compiling: " + sourcePath.filename().string() + " -> " + exePath.filename().string());

    auto startTime = std::chrono::steady_clock::now();

    // Execute compiler through ProcessSandbox with a generous 15s timeout
    ExecutionConfig execConfig;
    execConfig.executablePath = m_compilerPath;
    execConfig.workingDirectory = targetDir;
    // For MinGW g++, we can invoke cmd.exe /c or directly g++.exe
    // Let's invoke via cmd.exe /c to handle path quotes and flags portably on Windows
    std::string fullCmd = "cmd.exe /c " + cmd;
    execConfig.executablePath = "C:\\Windows\\System32\\cmd.exe /c " + cmd;
    execConfig.timeLimitMs = 20000;
    execConfig.memoryLimitMb = 1024;

    RawExecutionResult raw = ProcessSandbox::execute(execConfig);

    auto endTime = std::chrono::steady_clock::now();
    result.compileTimeMs = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
    );

    result.compilerOutput = raw.stdOut + (raw.stdErr.empty() ? "" : ("\n" + raw.stdErr));
    if (!raw.diagnosticMessage.empty() && raw.verdict != Verdict::AC) {
        result.compilerOutput += "\n" + raw.diagnosticMessage;
    }

    // Check if output executable was actually generated
    if (fs::exists(exePath) && raw.exitCode == 0) {
        result.success = true;
        Logger::instance().success("Compiled successfully in " + std::to_string(result.compileTimeMs) + " ms");
    } else {
        result.success = false;
        Logger::instance().error("Compilation failed: " + sourcePath.filename().string());
    }

    return result;
}

} // namespace loj
