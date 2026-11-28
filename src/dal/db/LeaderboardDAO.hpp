#pragma once

#include "common/Types.hpp"
#include <vector>
#include <filesystem>

namespace loj {

class LeaderboardDAO {
public:
    static LeaderboardDAO& instance();

    // Sinh bảng xếp hạng theo chế độ chấm và phiên chấm (sessionId = 0: Tất cả các phiên)
    std::vector<LeaderboardEntry> generateLeaderboard(GradingMode mode, int sessionId = 0);

    // Lấy danh sách các mã bài toán đã được nộp trong phiên (được sắp xếp theo thứ tự chữ cái A-Z)
    std::vector<std::string> getSessionProblemCodes(int sessionId = 0);

    // Xuất danh sách bảng xếp hạng ra file CSV
    bool exportToCSV(const std::filesystem::path& filePath, GradingMode mode, int sessionId = 0);

    // Xuất danh sách bảng xếp hạng ra file văn bản (TXT)
    bool exportToText(const std::filesystem::path& filePath, GradingMode mode, int sessionId = 0);

private:
    LeaderboardDAO() = default;
    ~LeaderboardDAO() = default;
};

} // namespace loj
