#pragma once

#include "common/Types.hpp"
#include <vector>
#include <string>

namespace loj {

class LeaderboardView {
public:
    LeaderboardView();
    ~LeaderboardView() = default;

    void render();
    void refresh();

private:
    GradingMode m_currentMode = GradingMode::EQUAL;
    int m_selectedSessionId = 0; // 0: Tất cả các phiên
    std::vector<LeaderboardEntry> m_entries;
    int m_selectedEntryIndex = -1;
    char m_searchQuery[128] = ""; // Tìm kiếm nhanh MSSV hoặc Tên thí sinh

    // Quản lý xóa thí sinh
    std::string m_contestantToDeleteId;
    std::string m_contestantToDeleteName;
    bool m_showDeleteConfirmPopup = false;

    // Thông báo xuất file
    std::string m_exportStatusMessage;
};

} // namespace loj
