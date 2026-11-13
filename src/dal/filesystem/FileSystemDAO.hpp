#pragma once

#include "common/Types.hpp"
#include <filesystem>
#include <vector>
#include <string>

namespace loj {

struct ContestantFolder {
    std::string folderName;
    std::string studentName;
    std::string studentId;
    std::filesystem::path folderPath;
    std::vector<std::filesystem::path> sourceFiles;
};

class FileSystemDAO {
public:
    static FileSystemDAO& instance();

    // Parse contestant information from directory name: "StudentName_ID"
    static bool parseContestantInfo(const std::string& folderName, std::string& outName, std::string& outId);

    // Scan a single contestant directory for .cpp source files
    ContestantFolder scanContestantFolder(const std::filesystem::path& folderPath);

    // Scan a batch root folder containing multiple contestant directories
    std::vector<ContestantFolder> scanBatchDirectory(const std::filesystem::path& batchRootPath);

    // Scan and load testcase pairs from a problem directory
    std::vector<TestcaseData> loadTestcases(const std::filesystem::path& testcaseDir);

    // Save a new testcase to disk
    bool saveTestcase(const std::filesystem::path& problemDir, int index,
                      const std::string& inputContent, const std::string& outputContent);

    // Read full file content
    static std::string readFile(const std::filesystem::path& filePath);

    // Write file content
    static bool writeFile(const std::filesystem::path& filePath, const std::string& content);

private:
    FileSystemDAO() = default;
    ~FileSystemDAO() = default;
};

} // namespace loj
