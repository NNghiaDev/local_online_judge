#include "ModelCodeRunner.hpp"
#include "sandbox/ProcessSandbox.hpp"
#include "dal/filesystem/FileSystemDAO.hpp"
#include "common/Logger.hpp"

#include <regex>

namespace loj {

namespace fs = std::filesystem;

bool ModelCodeRunner::generateOutputsForDirectory(const fs::path& modelSourcePath,
                                                 const fs::path& testcaseDir) {
    if (!fs::exists(modelSourcePath)) {
        Logger::instance().error("Model source file not found: " + modelSourcePath.string());
        return false;
    }

    if (!fs::exists(testcaseDir)) {
        Logger::instance().error("Testcase directory not found: " + testcaseDir.string());
        return false;
    }

    // 1. Compile model source
    CompilationResult comp = m_compiler.compile(modelSourcePath);
    if (!comp.success) {
        Logger::instance().error("Model solution compilation failed:\n" + comp.compilerOutput);
        return false;
    }

    // 2. Discover all input files in testcase directory
    int generatedCount = 0;
    std::regex numRegex(R"(\d+)");

    for (const auto& entry : fs::directory_iterator(testcaseDir)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        std::string lower = filename;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        bool isInput = (lower.find("in") != std::string::npos || entry.path().extension() == ".in");
        if (!isInput) continue;

        // Determine corresponding output file path
        fs::path outPath;
        std::smatch match;
        if (std::regex_search(filename, match, numRegex)) {
            outPath = testcaseDir / ("output_" + match.str() + ".txt");
        } else {
            outPath = entry.path();
            outPath.replace_extension(".out");
        }

        std::string inContent = FileSystemDAO::readFile(entry.path());

        ExecutionConfig cfg;
        cfg.executablePath = comp.outputExecutablePath;
        cfg.workingDirectory = testcaseDir;
        cfg.inputData = inContent;
        cfg.timeLimitMs = 5000;
        cfg.memoryLimitMb = 512;

        RawExecutionResult res = ProcessSandbox::execute(cfg);
        if (res.verdict == Verdict::AC && res.exitCode == 0) {
            FileSystemDAO::writeFile(outPath, res.stdOut);
            generatedCount++;
            Logger::instance().success("Generated expected output: " + outPath.filename().string());
        } else {
            Logger::instance().error("Model solution failed on input: " + filename);
        }
    }

    // Clean up temporary binary
    std::error_code ec;
    fs::remove(comp.outputExecutablePath, ec);

    Logger::instance().success("Model runner generated " + std::to_string(generatedCount) + " output files.");
    return (generatedCount > 0);
}

std::string ModelCodeRunner::runModelOnInput(const fs::path& modelSourcePath,
                                            const std::string& inputContent) {
    CompilationResult comp = m_compiler.compile(modelSourcePath);
    if (!comp.success) {
        return "";
    }

    ExecutionConfig cfg;
    cfg.executablePath = comp.outputExecutablePath;
    cfg.inputData = inputContent;
    cfg.timeLimitMs = 5000;
    cfg.memoryLimitMb = 512;

    RawExecutionResult res = ProcessSandbox::execute(cfg);

    std::error_code ec;
    fs::remove(comp.outputExecutablePath, ec);

    if (res.verdict == Verdict::AC && res.exitCode == 0) {
        return res.stdOut;
    }
    return "";
}

} // namespace loj
