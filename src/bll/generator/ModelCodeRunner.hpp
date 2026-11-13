#pragma once

#include "compiler/CompilerService.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace loj {

class ModelCodeRunner {
public:
    ModelCodeRunner() = default;
    ~ModelCodeRunner() = default;

    // Compiles a model solution and generates .out files for all inputs in the target folder
    bool generateOutputsForDirectory(const std::filesystem::path& modelSourcePath,
                                    const std::filesystem::path& testcaseDir);

    // Compiles a model solution and runs it on a single input string
    std::string runModelOnInput(const std::filesystem::path& modelSourcePath,
                               const std::string& inputContent);

private:
    CompilerService m_compiler;
};

} // namespace loj
