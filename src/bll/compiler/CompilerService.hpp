#pragma once

#include "common/Types.hpp"
#include <filesystem>
#include <string>

namespace loj {

struct CompilationResult {
    bool success = false;
    std::filesystem::path outputExecutablePath;
    std::string compilerOutput;
    uint32_t compileTimeMs = 0;
};

class CompilerService {
public:
    CompilerService();
    ~CompilerService() = default;

    // Compiles a C++ source file using MinGW g++
    CompilationResult compile(const std::filesystem::path& sourcePath,
                              const std::filesystem::path& outputDir = "");

    void setCompilerPath(const std::string& path) { m_compilerPath = path; }
    const std::string& getCompilerPath() const { return m_compilerPath; }

private:
    std::string m_compilerPath = "g++";
};

} // namespace loj
