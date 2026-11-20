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

std::vector<LeaderboardEntry> LeaderboardDAO::generateLeaderboard(GradingMode mode, int sessionId) {
    auto allSubmissions = SubmissionDAO::instance().getAllSubmissions();
    std::vector<SubmissionSummary> submissions;

    if (sessionId > 0) {
        for (const auto& s : allSubmissions) {
            if (s.sessionId == sessionId) {
                submissions.push_back(s);
            }
        }
    } else {
        submissions = allSubmissions;
    }

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

        if (agg.problemBestScore.find(sub.problemCode) == agg.problemBestScore.end() || sub.totalScore > agg.problemBestScore[sub.problemCode]) {
            agg.problemBestScore[sub.problemCode] = sub.totalScore;
            agg.problemTime[sub.problemCode] = sub.totalTimeMs;
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
        entry.sessionId = sessionId;
        entry.studentId = agg.studentId;
        entry.studentName = agg.studentName;

        for (const auto& [prob, score] : agg.problemBestScore) {
            entry.totalScore += score;
            entry.problemScores.emplace_back(prob, score);
            if (agg.problemTime.count(prob)) {
                entry.totalPenaltyTime += agg.problemTime.at(prob);
            }
        }

        for (const auto& [prob, solved] : agg.problemSolved) {
            entry.problemSolvedStatus.emplace_back(prob, solved);
            if (solved) {
                entry.solvedCount++;
            }
        }

        leaderboard.push_back(entry);
    }

    if (mode == GradingMode::ICPC) {
        // ICPC: Ưu tiên Số bài giải được (giảm dần) -> Tổng thời gian phạt (tăng dần)
        std::sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (a.solvedCount != b.solvedCount) {
                return a.solvedCount > b.solvedCount;
            }
            return a.totalPenaltyTime < b.totalPenaltyTime;
        });
    } else {
        // IOI / EQUAL: Ưu tiên Tổng điểm (giảm dần) -> Tổng thời gian chạy (tăng dần)
        std::sort(leaderboard.begin(), leaderboard.end(), [](const LeaderboardEntry& a, const LeaderboardEntry& b) {
            if (std::abs(a.totalScore - b.totalScore) > 1e-4) {
                return a.totalScore > b.totalScore;
            }
            return a.totalPenaltyTime < b.totalPenaltyTime;
        });
    }

    int currentRank = 1;
    for (size_t i = 0; i < leaderboard.size(); ++i) {
        if (i > 0) {
            bool tie = false;
            if (mode == GradingMode::ICPC) {
                tie = (leaderboard[i].solvedCount == leaderboard[i - 1].solvedCount &&
                       leaderboard[i].totalPenaltyTime == leaderboard[i - 1].totalPenaltyTime);
            } else {
                tie = (std::abs(leaderboard[i].totalScore - leaderboard[i - 1].totalScore) <= 1e-4 &&
                       leaderboard[i].totalPenaltyTime == leaderboard[i - 1].totalPenaltyTime);
            }
            if (!tie) {
                currentRank = static_cast<int>(i + 1);
            }
        }
        leaderboard[i].rank = currentRank;
    }

    return leaderboard;
}

bool LeaderboardDAO::exportToCSV(const std::filesystem::path& filePath, GradingMode mode, int sessionId) {
    auto entries = generateLeaderboard(mode, sessionId);

    std::ofstream out(filePath, std::ios::out | std::ios::binary);
    if (!out.is_open()) {
        Logger::instance().error("Không thể mở file để ghi bảng xếp hạng: " + filePath.string());
        return false;
    }

    // Ghi UTF-8 BOM
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));

    // Tiêu đề cột tương ứng theo chế độ
    if (mode == GradingMode::ICPC) {
        out << "Thứ Hạng,Mã Số Sinh Viên (MSSV),Họ Và Tên Thí Sinh,Số Bài Đúng (AC),Thời Gian Phạt (ms)\n";
        for (const auto& e : entries) {
            out << e.rank << ",\""
                << e.studentId << "\",\""
                << e.studentName << "\","
                << e.solvedCount << ","
                << e.totalPenaltyTime << "\n";
        }
    } else {
        out << "Thứ Hạng,Mã Số Sinh Viên (MSSV),Họ Và Tên Thí Sinh,Tổng Điểm Số,Thời Gian (ms)\n";
        for (const auto& e : entries) {
            out << e.rank << ",\""
                << e.studentId << "\",\""
                << e.studentName << "\","
                << std::fixed << std::setprecision(1) << e.totalScore << ","
                << e.totalPenaltyTime << "\n";
        }
    }

    out.close();
    Logger::instance().success("Đã xuất Bảng Xếp Hạng ra file CSV: " + filePath.string());
    return true;
}

bool LeaderboardDAO::exportToText(const std::filesystem::path& filePath, GradingMode mode, int sessionId) {
    auto entries = generateLeaderboard(mode, sessionId);

    std::ofstream out(filePath, std::ios::out);
    if (!out.is_open()) {
        Logger::instance().error("Không thể mở file để ghi: " + filePath.string());
        return false;
    }

    std::string sessionTitle = (sessionId > 0) ? ("Phiên #" + std::to_string(sessionId)) : "Tất cả các phiên";

    out << "========================================================================================\n";
    out << "                        BẢNG XẾP HẠNG THÍ SINH (LOCAL ONLINE JUDGE)                    \n";
    out << "                        Phiên: " << sessionTitle << " | Chế độ: " << gradingModeToString(mode) << "\n";
    out << "========================================================================================\n";

    if (mode == GradingMode::ICPC) {
        out << std::left
            << std::setw(8)  << "Hạng"
            << std::setw(18) << "MSSV"
            << std::setw(32) << "Họ Và Tên"
            << std::setw(18) << "Số Bài Đúng (AC)"
            << std::setw(18) << "Thời Gian Phạt (ms)" << "\n";
        out << "----------------------------------------------------------------------------------------\n";
        for (const auto& e : entries) {
            out << std::left
                << std::setw(8)  << e.rank
                << std::setw(18) << e.studentId
                << std::setw(32) << e.studentName
                << std::setw(18) << (std::to_string(e.solvedCount) + " bài")
                << std::setw(18) << e.totalPenaltyTime << "\n";
        }
    } else {
        out << std::left
            << std::setw(8)  << "Hạng"
            << std::setw(18) << "MSSV"
            << std::setw(32) << "Họ Và Tên"
            << std::setw(18) << "Tổng Điểm"
            << std::setw(18) << "Thời Gian (ms)" << "\n";
        out << "----------------------------------------------------------------------------------------\n";
        for (const auto& e : entries) {
            std::ostringstream scoreSs;
            scoreSs << std::fixed << std::setprecision(1) << e.totalScore << " đ";
            out << std::left
                << std::setw(8)  << e.rank
                << std::setw(18) << e.studentId
                << std::setw(32) << e.studentName
                << std::setw(18) << scoreSs.str()
                << std::setw(18) << e.totalPenaltyTime << "\n";
        }
    }
    out << "========================================================================================\n";

    out.close();
    Logger::instance().success("Đã xuất Bảng Xếp Hạng ra file TXT: " + filePath.string());
    return true;
}

} // namespace loj
