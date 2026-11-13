#include "LeaderboardView.hpp"
#include "dal/db/LeaderboardDAO.hpp"
#include "dal/db/SubmissionDAO.hpp"
#include <imgui.h>
#include <filesystem>

namespace loj {

LeaderboardView::LeaderboardView() {
    refresh();
}

void LeaderboardView::refresh() {
    m_entries = LeaderboardDAO::instance().generateLeaderboard(m_currentMode);
    if (m_selectedEntryIndex >= static_cast<int>(m_entries.size())) {
        m_selectedEntryIndex = -1;
    }
}

void LeaderboardView::render() {
    float availW = ImGui::GetContentRegionAvail().x;

    // =========================================================================
    // THANH CÔNG CỤ DÀN ĐỀU VỚI CHỨC NĂNG XUẤT BẢNG XẾP HẠNG
    // =========================================================================
    ImGui::Columns(2, "LbHeaderCols", false);
    ImGui::SetColumnWidth(0, availW - 580.0f);
    ImGui::SetColumnWidth(1, 580.0f);

    ImGui::TextColored(ImVec4(1.00f, 0.82f, 0.20f, 1.0f), "BẢNG XẾP HẠNG THÍ SINH");
    if (!m_exportStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f), "| %s", m_exportStatusMessage.c_str());
    }

    ImGui::NextColumn();

    // 1. Bộ lọc chế độ
    int modeIdx = static_cast<int>(m_currentMode);
    const char* modes[] = {"ICPC", "IOI", "Equal"};
    ImGui::SetNextItemWidth(110.0f);
    if (ImGui::Combo("##ModeFilterCombo", &modeIdx, modes, IM_ARRAYSIZE(modes))) {
        m_currentMode = static_cast<GradingMode>(modeIdx);
        refresh();
    }

    // 2. Nút Làm Mới
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.50f, 0.90f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.60f, 1.00f, 1.0f));
    if (ImGui::Button("Làm Mới", ImVec2(90, 28))) {
        refresh();
    }
    ImGui::PopStyleColor(2);

    // 3. NÚT XUẤT FILE CSV (EXCEL)
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.65f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.78f, 0.48f, 1.0f));
    if (ImGui::Button("Xuất CSV (Excel)", ImVec2(150, 28))) {
        std::filesystem::path csvPath = std::filesystem::current_path() / "bang_xep_hang.csv";
        if (LeaderboardDAO::instance().exportToCSV(csvPath, m_currentMode)) {
            m_exportStatusMessage = "Đã xuất: bang_xep_hang.csv";
        }
    }
    ImGui::PopStyleColor(2);

    // 4. NÚT XUẤT FILE TXT
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.35f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.45f, 0.88f, 1.0f));
    if (ImGui::Button("Xuất TXT", ImVec2(100, 28))) {
        std::filesystem::path txtPath = std::filesystem::current_path() / "bang_xep_hang.txt";
        if (LeaderboardDAO::instance().exportToText(txtPath, m_currentMode)) {
            m_exportStatusMessage = "Đã xuất: bang_xep_hang.txt";
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Spacing();

    // =========================================================================
    // BẢNG ĐIỂM CHI TIẾT
    // =========================================================================
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                       ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                       ImGuiTableFlags_ScrollY;

    float tableHeight = (m_selectedEntryIndex >= 0) ? 320.0f : ImGui::GetContentRegionAvail().y - 10.0f;
    if (tableHeight < 200.0f) tableHeight = 200.0f;

    if (ImGui::BeginTable("BangXepHangChiTiet", 7, tableFlags, ImVec2(0, tableHeight))) {
        ImGui::TableSetupColumn("Hạng", ImGuiTableColumnFlags_WidthFixed, 75.0f);
        ImGui::TableSetupColumn("MSSV", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableSetupColumn("Họ Và Tên", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Số Bài Đúng", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Tổng Điểm", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Thời Gian (ms)", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Thao Tác", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        if (m_entries.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Chưa có dữ liệu bài nộp.");
        }

        for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
            const auto& entry = m_entries[i];
            ImGui::TableNextRow();

            // Huân chương Vàng, Bạc, Đồng cho top 3
            if (entry.rank == 1) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(234, 179, 8, 55));
            } else if (entry.rank == 2) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(148, 163, 184, 45));
            } else if (entry.rank == 3) {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(180, 83, 9, 45));
            }

            ImGui::TableSetColumnIndex(0);
            if (entry.rank == 1)      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1 Vàng");
            else if (entry.rank == 2) ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "#2 Bạc");
            else if (entry.rank == 3) ImGui::TextColored(ImVec4(0.9f, 0.60f, 0.3f, 1.0f), "#3 Đồng");
            else                      ImGui::Text("#%d", entry.rank);

            ImGui::TableSetColumnIndex(1);
            if (ImGui::Selectable(entry.studentId.c_str(), m_selectedEntryIndex == i, ImGuiSelectableFlags_SpanAllColumns)) {
                m_selectedEntryIndex = i;
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", entry.studentName.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextColored(ImVec4(0.20f, 0.85f, 0.40f, 1.0f), "%d bài", entry.solvedCount);

            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(ImVec4(0.25f, 0.70f, 1.00f, 1.0f), "%.1f", entry.totalScore);

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%u", entry.totalPenaltyTime);

            // Nút Xóa thí sinh
            ImGui::TableSetColumnIndex(6);
            std::string btnDelId = "Xóa##" + entry.studentId;
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.22f, 0.22f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.00f));
            if (ImGui::Button(btnDelId.c_str(), ImVec2(65, 22))) {
                m_contestantToDeleteId = entry.studentId;
                m_contestantToDeleteName = entry.studentName;
                m_showDeleteConfirmPopup = true;
            }
            ImGui::PopStyleColor(2);
        }

        ImGui::EndTable();
    }

    // =========================================================================
    // CHI TIẾT THÍ SINH ĐƯỢC CHỌN
    // =========================================================================
    if (m_selectedEntryIndex >= 0 && m_selectedEntryIndex < static_cast<int>(m_entries.size())) {
        const auto& selected = m_entries[m_selectedEntryIndex];
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Columns(2, "DetailHeaderCols", false);
        ImGui::SetColumnWidth(0, availW - 220.0f);

        ImGui::TextColored(ImVec4(0.30f, 0.80f, 1.00f, 1.0f),
                           "Chi tiết bài thi: %s (MSSV: %s)",
                           selected.studentName.c_str(), selected.studentId.c_str());

        ImGui::NextColumn();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.98f, 0.30f, 0.30f, 1.0f));
        if (ImGui::Button("Xóa Thí Sinh Này", ImVec2(180, 26))) {
            m_contestantToDeleteId = selected.studentId;
            m_contestantToDeleteName = selected.studentName;
            m_showDeleteConfirmPopup = true;
        }
        ImGui::PopStyleColor(2);

        ImGui::Columns(1);
        ImGui::Spacing();

        auto contestantSubs = SubmissionDAO::instance().getSubmissionsByContestant(selected.studentId);
        if (contestantSubs.empty()) {
            ImGui::TextDisabled("Chưa có lịch sử bài thi.");
        } else {
            for (const auto& sub : contestantSubs) {
                ImVec4 verdictCol = ImVec4(0.20f, 0.90f, 0.40f, 1.0f);
                std::string vStr = verdictToString(sub.overallVerdict);
                if (sub.overallVerdict == Verdict::WA)       verdictCol = ImVec4(1.00f, 0.30f, 0.30f, 1.0f);
                else if (sub.overallVerdict == Verdict::TLE) verdictCol = ImVec4(1.00f, 0.65f, 0.15f, 1.0f);
                else if (sub.overallVerdict == Verdict::MLE) verdictCol = ImVec4(0.85f, 0.40f, 1.00f, 1.0f);
                else if (sub.overallVerdict == Verdict::CE)  verdictCol = ImVec4(0.90f, 0.20f, 0.20f, 1.0f);

                ImGui::Bullet();
                ImGui::Text("Bài: [%s]", sub.problemCode.c_str());
                ImGui::SameLine();
                ImGui::TextColored(verdictCol, "[%s]", vStr.c_str());
                ImGui::SameLine();
                ImGui::Text("| Điểm: %.1f | Thời gian: %u ms | Bộ nhớ: %u KB",
                            sub.totalScore, sub.totalTimeMs, sub.maxMemoryKb);
            }
        }
    }

    // Popup xác nhận xóa
    if (m_showDeleteConfirmPopup) {
        ImGui::OpenPopup("Xác Nhận Xóa");
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(440, 180));

    if (ImGui::BeginPopupModal("Xác Nhận Xóa", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "XÁC NHẬN XÓA THÍ SINH:");
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Thí sinh: %s", m_contestantToDeleteName.c_str());
        ImGui::Text("MSSV: %s", m_contestantToDeleteId.c_str());
        ImGui::Text("Dữ liệu sẽ bị xóa khỏi bảng xếp hạng và CSDL.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.20f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.00f, 0.30f, 0.30f, 1.0f));
        if (ImGui::Button("Đồng Ý Xóa", ImVec2(140, 32))) {
            SubmissionDAO::instance().deleteContestant(m_contestantToDeleteId);
            refresh();
            m_showDeleteConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine();
        if (ImGui::Button("Hủy Bỏ", ImVec2(100, 32))) {
            m_showDeleteConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

} // namespace loj
