#include "LeaderboardDAO.hpp"
#include "SubmissionDAO.hpp"
#include "common/Logger.hpp"
#include <map>
#include <algorithm>
#include <fstream>
#include <iomanip>

namespace loj {

LeaderboardDAO& LeaderboardDAO::instance() {
    static LeaderboardDAO s_instance;
    return s_instance;
}

std::vector<LeaderboardEntry> LeaderboardDAO::generateLeaderboard(GradingMode mode) {
    auto submissions = SubmissionDAO::instance().getAllSubmissions();

    struct ContestantAgg {
        std::string studentId;
        std::string studentName;
        std::map<std::string, double> problemBestScore;
        std::map<std::string, bool> problemSolved;
        std::map<std::string, uint32_t> problemTime;
    };

    std::map<std::string, ContestantAgg> aggMap;

    for (const auto& sub : submissions) {
        auto& agg = aggMap[sub.studentId];
        agg.studentId = sub.studentId;
        if (!sub.studentName.empty()) {
            agg.studentName = sub.studentName;
        } else if (agg.studentName.empty()) {
            agg.studentName = sub.studentId;
        }

        double currentBest = agg.problemBestScore[sub.problemCode];
        if (sub.totalScore > currentBest) {
            agg.problemBestScore[sub.problemCode] = sub.totalScore;
        }

        if (sub.overallVerdict == Verdict::AC) {
            agg.problemSolved[sub.problemCode] = true;
            if (agg.problemTime[sub.problemCode] == 0 || sub.totalTimeMs < agg.problemTime[sub.problemCode]) {
                agg.problemTime[sub.problemCode] = sub.totalTimeMs;
            }
        }
    }

    std::vector<LeaderboardEntry> leaderboard;
    for (const auto& [studentId, agg] : aggMap) {
        LeaderboardEntry entry;
        entry.studentId = agg.studentId;
        entry.studentName = agg.studentName;

        for (const auto& [prob, score] : agg.problemBestScore) {
            entry.totalScore += score;
            entry.problemScores.emplace_back(prob, score);
        }

        for (const auto& [prob, solved] : agg.problemSolved) {
            if (solved) {
                entry.solvedCount++;
                entry.totalPenaltyTime += agg.problemTime.at(prob);
            }
        }

        leaderboard.push_back(entry);
    }

    if (mode == GradingMode::ICPC) {
        std::sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.solvedCount != b.solvedCount) {
                return a.solvedCount > b.solvedCount;
            }
            return a.totalPenaltyTime < b.totalPenaltyTime;
        });
    } else {
        std::sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.totalScore != b.totalScore) {
                return a.totalScore > b.totalScore;
            }
            return a.totalPenaltyTime < b.totalPenaltyTime;
        });
    }

    for (size_t i = 0; i < leaderboard.size(); ++i) {
        leaderboard[i].rank = static_cast<int>(i + 1);
    }

    return leaderboard;
}

bool LeaderboardDAO::exportToCSV(const std::filesystem::path& filePath, GradingMode mode) {
    auto entries = generateLeaderboard(mode);

    std::ofstream out(filePath, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
        Logger::instance().error("Không thể mở file để ghi bảng xếp hạng: " + filePath.string());
        return false;
    }

    // Ghi UTF-8 BOM để Excel hiển thị tiếng Việt có dấu chuẩn 100%
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));

    // Header CSV
    out << "Thứ Hạng,Mã Số Sinh Viên (MSSV),Họ Và Tên Thí Sinh,Số Bài Đúng (AC),Tổng Điểm Số,Thời Gian Phạt (ms)\n";

    for (const auto& e : entries) {
        out << e.rank << ",\""
            << e.studentId << "\",\""
            << e.studentName << "\","
            << e.solvedCount << ","
            << std::fixed << std::setprecision(1) << e.totalScore << ","
            << e.totalPenaltyTime << "\n";
    }

    out.close();
    Logger::instance().success("Đã xuất Bảng Xếp Hạng ra file CSV: " + filePath.string());
    return true;
}

bool LeaderboardDAO::exportToText(const std::filesystem::path& filePath, GradingMode mode) {
    auto entries = generateLeaderboard(mode);

    std::ofstream out(filePath, std::ios::out);
    if (!out.is_open()) {
        Logger::instance().error("Không thể mở file để ghi: " + filePath.string());
        return false;
    }

    out << "========================================================================================\n";
    out << "                        BẢNG XẾP HẠNG THÍ SINH (LOCAL ONLINE JUDGE)                    \n";
    out << "                        Chế độ chấm: " << gradingModeToString(mode) << "\n";
    out << "========================================================================================\n";
    out << std::left
        << std::setw(8)  << "Hạng"
        << std::setw(18) << "MSSV"
        << std::setw(30) << "Họ Và Tên"
        << std::setw(14) << "Số Bài Đúng"
        << std::setw(14) << "Tổng Điểm"
        << std::setw(16) << "Thời Gian (ms)" << "\n";
    out << "----------------------------------------------------------------------------------------\n";

    for (const auto& e : entries) {
        out << std::left
            << std::setw(8)  << ("#" + std::to_string(e.rank))
            << std::setw(18) << e.studentId
            << std::setw(30) << e.studentName
            << std::setw(14) << e.solvedCount
            << std::setw(14) << (std::to_string(static_cast<int>(e.totalScore)) + " đ")
            << std::setw(16) << e.totalPenaltyTime << "\n";
    }
    out << "========================================================================================\n";

    out.close();
    Logger::instance().success("Đã xuất Bảng Xếp Hạng ra file TXT: " + filePath.string());
    return true;
}

} // namespace loj
