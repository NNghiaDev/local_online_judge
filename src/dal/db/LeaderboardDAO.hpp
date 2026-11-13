#pragma once

#include "common/Types.hpp"
#include <vector>
#include <filesystem>

namespace loj {

class LeaderboardDAO {
public:
    static LeaderboardDAO& instance();

    std::vector<LeaderboardEntry> generateLeaderboard(GradingMode mode);

    // Xuất danh sách bảng xếp hạng ra file CSV (tương thích Microsoft Excel)
    bool exportToCSV(const std::filesystem::path& filePath, GradingMode mode);

    // Xuất danh sách bảng xếp hạng ra file văn bản (TXT)
    bool exportToText(const std::filesystem::path& filePath, GradingMode mode);

private:
    LeaderboardDAO() = default;
    ~LeaderboardDAO() = default;
};

} // namespace loj
