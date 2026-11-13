#include "FileSystemDAO.hpp"
#include "common/Logger.hpp"
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <algorithm>

namespace loj {

namespace fs = std::filesystem;

FileSystemDAO& FileSystemDAO::instance() {
    static FileSystemDAO s_instance;
    return s_instance;
}

bool FileSystemDAO::parseContestantInfo(const std::string& folderName, std::string& outName, std::string& outId) {
    // Expected format: StudentName_ID (e.g., "NguyenVanA_22010001" or "JohnDoe_ID123")
    auto lastUnderscore = folderName.find_last_of('_');
    if (lastUnderscore != std::string::npos && lastUnderscore > 0 && lastUnderscore < folderName.size() - 1) {
        outName = folderName.substr(0, lastUnderscore);
        outId = folderName.substr(lastUnderscore + 1);
        return true;
    }
    // Fallback if no underscore is present
    outName = folderName;
    outId = folderName;
    return false;
}

ContestantFolder FileSystemDAO::scanContestantFolder(const fs::path& folderPath) {
    ContestantFolder cf;
    cf.folderPath = folderPath;
    cf.folderName = folderPath.filename().string();
    parseContestantInfo(cf.folderName, cf.studentName, cf.studentId);

    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        return cf;
    }

    // Discover all .cpp source files inside the contestant directory
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            // Case-insensitive check for .cpp
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            if (ext == ".cpp") {
                cf.sourceFiles.push_back(entry.path());
            }
        }
    }

    return cf;
}

std::vector<ContestantFolder> FileSystemDAO::scanBatchDirectory(const fs::path& batchRootPath) {
    std::vector<ContestantFolder> folders;

    if (!fs::exists(batchRootPath) || !fs::is_directory(batchRootPath)) {
        Logger::instance().error("Batch directory does not exist: " + batchRootPath.string());
        return folders;
    }

    for (const auto& entry : fs::directory_iterator(batchRootPath)) {
        if (entry.is_directory()) {
            auto cf = scanContestantFolder(entry.path());
            if (!cf.sourceFiles.empty()) {
                folders.push_back(std::move(cf));
            }
        }
    }

    Logger::instance().info("Discovered " + std::to_string(folders.size()) + " contestant folders in batch directory.");
    return folders;
}

std::vector<TestcaseData> FileSystemDAO::loadTestcases(const fs::path& testcaseDir) {
    std::vector<TestcaseData> testcases;
    if (!fs::exists(testcaseDir) || !fs::is_directory(testcaseDir)) {
        Logger::instance().warning("Testcase directory does not exist: " + testcaseDir.string());
        return testcases;
    }

    // Map: index/baseName -> pair of inputPath, outputPath
    struct FilePair {
        fs::path inputPath;
        fs::path outputPath;
    };
    std::map<int, FilePair> indexedPairs;
    int autoIndex = 1;

    // Regex to match common testcase conventions:
    // 1. in: input_1.txt, input1.txt, 1.in, test1.in
    // 2. out: output_1.txt, output1.txt, 1.out, 1.ans, test1.out
    std::regex numRegex(R"(\d+)");

    for (const auto& entry : fs::directory_iterator(testcaseDir)) {
        if (!entry.is_regular_file()) continue;

        std::string filename = entry.path().filename().string();
        std::string lowerName = filename;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

        // Find numbers in filename to pair them
        std::smatch match;
        int idx = autoIndex;
        if (std::regex_search(filename, match, numRegex)) {
            idx = std::stoi(match.str());
        }

        if (lowerName.find("in") != std::string::npos || entry.path().extension() == ".in") {
            indexedPairs[idx].inputPath = entry.path();
        } else if (lowerName.find("out") != std::string::npos || entry.path().extension() == ".out" || entry.path().extension() == ".ans") {
            indexedPairs[idx].outputPath = entry.path();
        }
    }

    for (const auto& [idx, pair] : indexedPairs) {
        if (!pair.inputPath.empty() && !pair.outputPath.empty()) {
            TestcaseData tc;
            tc.index = idx;
            tc.inputFilePath = pair.inputPath.string();
            tc.expectedOutputFilePath = pair.outputPath.string();
            tc.inputContent = readFile(pair.inputPath);
            tc.expectedOutputContent = readFile(pair.outputPath);
            testcases.push_back(tc);
        }
    }

    // Sort by testcase index
    std::sort(testcases.begin(), testcases.end(), [](const TestcaseData& a, const TestcaseData& b) {
        return a.index < b.index;
    });

    Logger::instance().info("Loaded " + std::to_string(testcases.size()) + " testcases from: " + testcaseDir.string());
    return testcases;
}

bool FileSystemDAO::saveTestcase(const fs::path& problemDir, int index,
                                 const std::string& inputContent, const std::string& outputContent) {
    try {
        if (!fs::exists(problemDir)) {
            fs::create_directories(problemDir);
        }
        fs::path inPath = problemDir / ("input_" + std::to_string(index) + ".txt");
        fs::path outPath = problemDir / ("output_" + std::to_string(index) + ".txt");

        if (!writeFile(inPath, inputContent) || !writeFile(outPath, outputContent)) {
            return false;
        }

        Logger::instance().success("Saved testcase #" + std::to_string(index) + " to " + problemDir.string());
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to save testcase: " + std::string(e.what()));
        return false;
    }
}

std::string FileSystemDAO::readFile(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

bool FileSystemDAO::writeFile(const fs::path& filePath, const std::string& content) {
    std::ofstream file(filePath, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;
    file.write(content.data(), content.size());
    return true;
}

} // namespace loj
