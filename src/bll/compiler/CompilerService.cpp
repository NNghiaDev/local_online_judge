#include "CompilerService.hpp"
#include "sandbox/ProcessSandbox.hpp"
#include "common/Logger.hpp"

#include <chrono>
#include <sstream>

namespace loj {

namespace fs = std::filesystem;

CompilerService::CompilerService() {
    // Tự động tìm đường dẫn g++.exe chuẩn trên MinGW/MSYS2
    if (fs::exists("C:\\msys64\\ucrt64\\bin\\g++.exe")) {
        m_compilerPath = "C:\\msys64\\ucrt64\\bin\\g++.exe";
    } else if (fs::exists("C:\\msys64\\mingw64\\bin\\g++.exe")) {
        m_compilerPath = "C:\\msys64\\mingw64\\bin\\g++.exe";
    } else {
        m_compilerPath = "g++";
    }
}

CompilationResult CompilerService::compile(const fs::path& sourcePath, const fs::path& outputDir) {
    CompilationResult result;
    result.success = false;

    // Chuyển sang đường dẫn tuyệt đối chuẩn để g++ không bao giờ bị lạc thư mục
    fs::path absSource = fs::absolute(sourcePath);
    if (!fs::exists(absSource)) {
        // Thử tìm theo thư mục hiện hành
        absSource = fs::current_path() / sourcePath;
    }

    if (!fs::exists(absSource)) {
        result.compilerOutput = "File mã nguồn không tồn tại: " + sourcePath.string();
        Logger::instance().error(result.compilerOutput);
        return result;
    }

    fs::path targetDir = outputDir.empty() ? absSource.parent_path() : fs::absolute(outputDir);
    if (!fs::exists(targetDir)) {
        std::error_code ec;
        fs::create_directories(targetDir, ec);
    }

    std::string baseStem = absSource.stem().string();
    fs::path exePath = targetDir / (baseStem + "_exec.exe");
    result.outputExecutablePath = exePath;

    // Xóa file thực thi cũ nếu có
    if (fs::exists(exePath)) {
        std::error_code ec;
        fs::remove(exePath, ec);
    }

    // Câu lệnh biên dịch với đường dẫn tuyệt đối đầy đủ trong dấu nháy kép
    std::string cmd = "\"" + m_compilerPath + "\" -O3 -std=c++20 \"" + absSource.string() +
                      "\" -o \"" + exePath.string() + "\"";

    Logger::instance().info("Đang biên dịch: " + absSource.filename().string() + "...");

    auto startTime = std::chrono::steady_clock::now();

    ExecutionConfig execConfig;
    execConfig.executablePath = cmd;
    execConfig.workingDirectory = targetDir;
    execConfig.timeLimitMs = 25000;
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

    // Kiểm tra file thực thi đã sinh ra thành công hay chưa
    if (fs::exists(exePath) && raw.exitCode == 0) {
        result.success = true;
        Logger::instance().success("Biên dịch thành công [" + absSource.filename().string() + "] (" +
                               std::to_string(result.compileTimeMs) + " ms)");
    } else {
        result.success = false;
        Logger::instance().error("Lỗi biên dịch (CE) bài [" + absSource.filename().string() + "]:\n" + result.compilerOutput);
    }

    return result;
}

} // namespace loj
