#include "LeaderboardView.hpp"
#include "dal/db/LeaderboardDAO.hpp"
#include "dal/db/SubmissionDAO.hpp"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <set>
#include <cmath>

namespace loj {

LeaderboardView::LeaderboardView() {
    refresh();
}

void LeaderboardView::refresh() {
    m_entries = LeaderboardDAO::instance().generateLeaderboard(m_currentMode, m_selectedSessionId);
    if (m_selectedEntryIndex >= static_cast<int>(m_entries.size())) {
        m_selectedEntryIndex = -1;
    }
}

void LeaderboardView::render() {
    float availW = ImGui::GetContentRegionAvail().x;
    auto sessions = SubmissionDAO::instance().getAllSessions();

    // =========================================================================
    // 1. THANH ĐIỀU HƯỚNG VÀ BỘ LỌC GỌN GÀNG, KHÔNG THÔNG BÁO RƯỜM RÀ
    // =========================================================================
    std::string currentSessionName = "Tất cả các phiên";
    for (const auto& s : sessions) {
        if (s.id == m_selectedSessionId) {
            currentSessionName = s.sessionName + (s.isCompleted ? " [Đã xong]" : " [Đang mở]");
            break;
        }
    }

    ImGui::TextColored(ImVec4(1.00f, 0.82f, 0.20f, 1.0f), "BẢNG XẾP HẠNG THÍ SINH");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.40f, 0.75f, 1.00f, 1.0f), "(%s)", currentSessionName.c_str());

    if (!m_exportStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f), "| %s", m_exportStatusMessage.c_str());
    }

    ImGui::Spacing();

    // Dòng điều khiển: Lọc phiên, Tạo phiên, Chế độ xếp hạng, Tìm kiếm, Làm mới, Xuất file
    std::vector<std::string> sessionNames = {"Tất cả các phiên"};
    int currentSessionComboIdx = 0;
    for (size_t i = 0; i < sessions.size(); ++i) {
        std::string label = sessions[i].sessionName + (sessions[i].isCompleted ? " [Đã xong]" : " [Đang mở]");
        sessionNames.push_back(label);
        if (sessions[i].id == m_selectedSessionId) {
            currentSessionComboIdx = static_cast<int>(i + 1);
        }
    }

    ImGui::Text("Phiên:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::BeginCombo("##SessionCombo", sessionNames[currentSessionComboIdx].c_str())) {
        for (int i = 0; i < static_cast<int>(sessionNames.size()); ++i) {
            bool isSelected = (currentSessionComboIdx == i);
            if (ImGui::Selectable(sessionNames[i].c_str(), isSelected)) {
                if (i == 0) {
                    m_selectedSessionId = 0; // Tất cả
                } else {
                    m_selectedSessionId = sessions[i - 1].id;
                }
                refresh();
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.55f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.65f, 0.95f, 1.0f));
    if (ImGui::Button("+ Phiên Mới", ImVec2(95, 26))) {
        int newId = SubmissionDAO::instance().startNewSession();
        m_selectedSessionId = newId;
        refresh();
    }
    ImGui::PopStyleColor(2);

    if (m_selectedSessionId > 0) {
        // Nút Xóa Phiên
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.22f, 0.22f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.00f));
        if (ImGui::Button("Xóa Phiên", ImVec2(80, 26))) {
            m_sessionToDeleteId = m_selectedSessionId;
            m_showDeleteSessionModal = true;
        }
        ImGui::PopStyleColor(2);

        // Nút Đặt làm phiên đang chấm (nếu chưa phải phiên hiện tại)
        if (m_selectedSessionId != SubmissionDAO::instance().getCurrentSessionId()) {
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.60f, 0.40f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.75f, 0.48f, 1.00f));
            if (ImGui::Button("Đặt Làm Phiên Hiện Tại", ImVec2(165, 26))) {
                SubmissionDAO::instance().setCurrentSession(m_selectedSessionId);
                refresh();
            }
            ImGui::PopStyleColor(2);
        } else {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.45f, 1.0f), "[Đang Chấm]");
        }
    }

    ImGui::SameLine(0, 14.0f);
    ImGui::Text("Chế độ:");
    ImGui::SameLine();
    int modeIdx = static_cast<int>(m_currentMode);
    const char* modes[] = {"ICPC (Bài giải & Phạt)", "IOI (Tổng điểm tích lũy)", "Equal (Chia đều điểm)"};
    ImGui::SetNextItemWidth(185.0f);
    if (ImGui::Combo("##ModeFilterCombo", &modeIdx, modes, IM_ARRAYSIZE(modes))) {
        m_currentMode = static_cast<GradingMode>(modeIdx);
        refresh();
    }

    ImGui::SameLine(0, 14.0f);
    ImGui::SetNextItemWidth(150.0f);
    ImGui::InputTextWithHint("##LbSearch", "Tìm MSSV / Tên...", m_searchQuery, sizeof(m_searchQuery));

    ImGui::SameLine(0, 14.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.35f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.45f, 0.58f, 1.0f));
    if (ImGui::Button("Làm Mới", ImVec2(75, 26))) {
        refresh();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.65f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.78f, 0.48f, 1.0f));
    if (ImGui::Button("Xuất CSV", ImVec2(80, 26))) {
        std::string fileName = (m_selectedSessionId > 0) ?
            ("bang_xep_hang_phien_" + std::to_string(m_selectedSessionId) + ".csv") : "bang_xep_hang_tat_ca.csv";
        std::filesystem::path csvPath = std::filesystem::current_path() / fileName;
        if (LeaderboardDAO::instance().exportToCSV(csvPath, m_currentMode, m_selectedSessionId)) {
            m_exportStatusMessage = "Đã xuất: " + fileName;
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.35f, 0.75f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.50f, 0.45f, 0.88f, 1.0f));
    if (ImGui::Button("Xuất TXT", ImVec2(80, 26))) {
        std::string fileName = (m_selectedSessionId > 0) ?
            ("bang_xep_hang_phien_" + std::to_string(m_selectedSessionId) + ".txt") : "bang_xep_hang_tat_ca.txt";
        std::filesystem::path txtPath = std::filesystem::current_path() / fileName;
        if (LeaderboardDAO::instance().exportToText(txtPath, m_currentMode, m_selectedSessionId)) {
            m_exportStatusMessage = "Đã xuất: " + fileName;
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // =========================================================================
    // 2. LỌC DỮ LIỆU THEO TÌM KIẾM
    // =========================================================================
    std::string searchLower = m_searchQuery;
    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);

    std::vector<LeaderboardEntry> filteredEntries;
    for (const auto& entry : m_entries) {
        std::string idLower = entry.studentId;
        std::string nameLower = entry.studentName;
        std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);

        if (searchLower.empty() || idLower.find(searchLower) != std::string::npos || nameLower.find(searchLower) != std::string::npos) {
            filteredEntries.push_back(entry);
        }
    }

    // Lấy danh sách các bài toán con trong phiên chấm (được sắp xếp gọn gàng A-Z)
    std::vector<std::string> sortedProblems = LeaderboardDAO::instance().getSessionProblemCodes(m_selectedSessionId);
    if (sortedProblems.empty()) {
        std::set<std::string> pSet;
        for (const auto& e : filteredEntries) {
            for (const auto& [p, _] : e.problemDetails) pSet.insert(p);
            for (const auto& [p, _] : e.problemScores) pSet.insert(p);
            for (const auto& [p, _] : e.problemSolvedStatus) pSet.insert(p);
        }
        sortedProblems.assign(pSet.begin(), pSet.end());
    }

    // =========================================================================
    // 3. BẢNG XẾP HẠNG TÙY BIẾN CỘT CHUẨN XÁC THEO TỪNG CHẾ ĐỘ
    // =========================================================================
    static ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                       ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
                                       ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                       ImGuiTableFlags_ScrollY | ImGuiTableFlags_ScrollX;

    float tableHeight = (m_selectedEntryIndex >= 0) ? 310.0f : ImGui::GetContentRegionAvail().y - 10.0f;
    if (tableHeight < 180.0f) tableHeight = 180.0f;

    // -------------------------------------------------------------------------
    // A. CHẾ ĐỘ ICPC: KHÔNG CÓ CỘT TỔNG ĐIỂM
    // Cột: Hạng | MSSV | Họ và Tên | Số Bài Đúng | Thời Gian Phạt | [Bài 1] | [Bài 2] ... | Thao Tác
    // -------------------------------------------------------------------------
    if (m_currentMode == GradingMode::ICPC) {
        int numCols = 6 + static_cast<int>(sortedProblems.size());
        if (ImGui::BeginTable("BangXepHang_ICPC", numCols, tableFlags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupColumn("Hạng", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("MSSV", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Họ Và Tên", ImGuiTableColumnFlags_WidthFixed, 190.0f);
            ImGui::TableSetupColumn("Số Bài Đúng", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Thời Gian (ms)", ImGuiTableColumnFlags_WidthFixed, 120.0f);

            for (const auto& pCode : sortedProblems) {
                std::string colTitle = "[" + pCode + "]";
                ImGui::TableSetupColumn(colTitle.c_str(), ImGuiTableColumnFlags_WidthFixed, 95.0f);
            }
            if (sortedProblems.empty()) {
                ImGui::TableSetupColumn("Bài Giải", ImGuiTableColumnFlags_WidthFixed, 95.0f);
            }

            ImGui::TableSetupColumn("Thao Tác", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableHeadersRow();

            if (filteredEntries.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.98f, 0.75f, 0.20f, 1.0f), "Chưa có dữ liệu bài nộp trong phiên này.");
                if (m_selectedSessionId > 0) {
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::SmallButton("Xem Tất Cả Các Phiên")) {
                        m_selectedSessionId = 0;
                        refresh();
                    }
                }
            }

            for (int i = 0; i < static_cast<int>(filteredEntries.size()); ++i) {
                const auto& entry = filteredEntries[i];
                ImGui::TableNextRow();

                // Tô màu top 3
                if (entry.rank == 1) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(234, 179, 8, 45));
                } else if (entry.rank == 2) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(148, 163, 184, 35));
                } else if (entry.rank == 3) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(180, 83, 9, 35));
                }

                // Cột 0: Thứ hạng
                ImGui::TableSetColumnIndex(0);
                if (entry.rank == 1)      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1 Vàng");
                else if (entry.rank == 2) ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "#2 Bạc");
                else if (entry.rank == 3) ImGui::TextColored(ImVec4(0.9f, 0.60f, 0.3f, 1.0f), "#3 Đồng");
                else                      ImGui::Text("#%d", entry.rank);

                // Cột 1: MSSV (Chọn dòng)
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Selectable(entry.studentId.c_str(), m_selectedEntryIndex == i, ImGuiSelectableFlags_SpanAllColumns)) {
                    m_selectedEntryIndex = i;
                }

                // Cột 2: Họ và Tên
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", entry.studentName.c_str());

                // Cột 3: Số bài đúng (AC) - ICPC (Không dùng ký tự lạ gây dấu ? trên giao diện)
                ImGui::TableSetColumnIndex(3);
                ImGui::TextColored(ImVec4(0.20f, 0.95f, 0.45f, 1.0f), "%d bài", entry.solvedCount);

                // Cột 4: Thời gian phạt (ms)
                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%u ms", entry.totalPenaltyTime);

                // Cột từng bài con (Tách riêng từng cột bài)
                int colIdx = 5;
                for (const auto& pCode : sortedProblems) {
                    ImGui::TableSetColumnIndex(colIdx++);
                    auto it = entry.problemDetails.find(pCode);
                    if (it != entry.problemDetails.end()) {
                        const auto& pDet = it->second;
                        if (pDet.isAC) {
                            ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f), "AC (%ums)", pDet.timeMs);
                        } else if (pDet.attempted) {
                            std::string vStr = verdictToString(pDet.verdict);
                            ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "%s", vStr.c_str());
                        } else {
                            ImGui::TextDisabled("-");
                        }
                    } else {
                        // Fallback check backwards-compatible problemSolvedStatus
                        bool found = false;
                        for (const auto& [code, isAc] : entry.problemSolvedStatus) {
                            if (code == pCode) {
                                found = true;
                                if (isAc) ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f), "AC");
                                else ImGui::TextColored(ImVec4(0.90f, 0.30f, 0.30f, 1.0f), "WA");
                                break;
                            }
                        }
                        if (!found) ImGui::TextDisabled("-");
                    }
                }
                if (sortedProblems.empty()) {
                    ImGui::TableSetColumnIndex(colIdx++);
                    ImGui::TextDisabled("-");
                }

                // Cột Thao tác: Xóa thí sinh
                ImGui::TableSetColumnIndex(colIdx++);
                std::string btnDelId = "Xóa##" + entry.studentId;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.22f, 0.22f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.00f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
                if (ImGui::Button(btnDelId.c_str(), ImVec2(60, 24))) {
                    m_contestantToDeleteId = entry.studentId;
                    m_contestantToDeleteName = entry.studentName;
                    m_showDeleteConfirmPopup = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }

            ImGui::EndTable();
        }
    }
    // -------------------------------------------------------------------------
    // B. CHẾ ĐỘ IOI / EQUAL: KHÔNG CÓ CỘT SỐ BÀI ĐÚNG
    // Cột: Hạng | MSSV | Họ và Tên | [Bài 1] | [Bài 2] ... | Tổng Điểm | Thời Gian | Thao Tác
    // -------------------------------------------------------------------------
    else {
        int numCols = 6 + static_cast<int>(sortedProblems.size());
        if (ImGui::BeginTable("BangXepHang_Score", numCols, tableFlags, ImVec2(0, tableHeight))) {
            ImGui::TableSetupColumn("Hạng", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableSetupColumn("MSSV", ImGuiTableColumnFlags_WidthFixed, 130.0f);
            ImGui::TableSetupColumn("Họ Và Tên", ImGuiTableColumnFlags_WidthFixed, 190.0f);

            for (const auto& pCode : sortedProblems) {
                std::string colTitle = "[" + pCode + "]";
                ImGui::TableSetupColumn(colTitle.c_str(), ImGuiTableColumnFlags_WidthFixed, 95.0f);
            }
            if (sortedProblems.empty()) {
                ImGui::TableSetupColumn("Điểm Bài", ImGuiTableColumnFlags_WidthFixed, 95.0f);
            }

            ImGui::TableSetupColumn("Tổng Điểm", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Thời Gian (ms)", ImGuiTableColumnFlags_WidthFixed, 115.0f);
            ImGui::TableSetupColumn("Thao Tác", ImGuiTableColumnFlags_WidthFixed, 75.0f);
            ImGui::TableHeadersRow();

            if (filteredEntries.empty()) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.98f, 0.75f, 0.20f, 1.0f), "Chưa có dữ liệu bài nộp trong phiên này.");
                if (m_selectedSessionId > 0) {
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::SmallButton("Xem Tất Cả Các Phiên")) {
                        m_selectedSessionId = 0;
                        refresh();
                    }
                }
            }

            for (int i = 0; i < static_cast<int>(filteredEntries.size()); ++i) {
                const auto& entry = filteredEntries[i];
                ImGui::TableNextRow();

                // Tô màu top 3
                if (entry.rank == 1) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(234, 179, 8, 45));
                } else if (entry.rank == 2) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(148, 163, 184, 35));
                } else if (entry.rank == 3) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(180, 83, 9, 35));
                }

                // Cột 0: Thứ hạng
                ImGui::TableSetColumnIndex(0);
                if (entry.rank == 1)      ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "#1 Vàng");
                else if (entry.rank == 2) ImGui::TextColored(ImVec4(0.8f, 0.85f, 0.9f, 1.0f), "#2 Bạc");
                else if (entry.rank == 3) ImGui::TextColored(ImVec4(0.9f, 0.60f, 0.3f, 1.0f), "#3 Đồng");
                else                      ImGui::Text("#%d", entry.rank);

                // Cột 1: MSSV (Chọn dòng)
                ImGui::TableSetColumnIndex(1);
                if (ImGui::Selectable(entry.studentId.c_str(), m_selectedEntryIndex == i, ImGuiSelectableFlags_SpanAllColumns)) {
                    m_selectedEntryIndex = i;
                }

                // Cột 2: Họ và Tên
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%s", entry.studentName.c_str());

                // Cột từng bài con (Tách riêng từng cột bài)
                int colIdx = 3;
                for (const auto& pCode : sortedProblems) {
                    ImGui::TableSetColumnIndex(colIdx++);
                    auto it = entry.problemDetails.find(pCode);
                    if (it != entry.problemDetails.end()) {
                        const auto& pDet = it->second;
                        if (pDet.attempted) {
                            if (pDet.score > 0.0) {
                                ImGui::TextColored(ImVec4(0.20f, 0.85f, 0.40f, 1.0f), "%.1f đ", pDet.score);
                            } else {
                                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "0.0 đ");
                            }
                        } else {
                            ImGui::TextDisabled("-");
                        }
                    } else {
                        // Fallback check backwards-compatible problemScores
                        bool found = false;
                        for (const auto& [code, score] : entry.problemScores) {
                            if (code == pCode) {
                                found = true;
                                if (score > 0.0) ImGui::TextColored(ImVec4(0.20f, 0.85f, 0.40f, 1.0f), "%.1f đ", score);
                                else ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.0f), "0.0 đ");
                                break;
                            }
                        }
                        if (!found) ImGui::TextDisabled("-");
                    }
                }
                if (sortedProblems.empty()) {
                    ImGui::TableSetColumnIndex(colIdx++);
                    ImGui::TextDisabled("-");
                }

                // Cột: Tổng điểm
                ImGui::TableSetColumnIndex(colIdx++);
                ImGui::TextColored(ImVec4(0.25f, 0.80f, 1.00f, 1.0f), "%.1f đ", entry.totalScore);

                // Cột: Thời gian chạy (ms)
                ImGui::TableSetColumnIndex(colIdx++);
                ImGui::Text("%u ms", entry.totalPenaltyTime);

                // Cột Thao tác: Xóa thí sinh
                ImGui::TableSetColumnIndex(colIdx++);
                std::string btnDelId = "Xóa##" + entry.studentId;
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.22f, 0.22f, 0.85f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.00f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
                if (ImGui::Button(btnDelId.c_str(), ImVec2(60, 24))) {
                    m_contestantToDeleteId = entry.studentId;
                    m_contestantToDeleteName = entry.studentName;
                    m_showDeleteConfirmPopup = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(2);
            }

            ImGui::EndTable();
        }
    }

    // =========================================================================
    // 4. CHI TIẾT BÀI THI CỦA THÍ SINH ĐƯỢC CHỌN
    // =========================================================================
    if (m_selectedEntryIndex >= 0 && m_selectedEntryIndex < static_cast<int>(filteredEntries.size())) {
        const auto& selected = filteredEntries[m_selectedEntryIndex];
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::Columns(2, "DetailHeaderCols", false);
        ImGui::SetColumnWidth(0, availW - 200.0f);

        ImGui::TextColored(ImVec4(0.35f, 0.85f, 1.00f, 1.0f),
                           "Chi tiết các bài thi của thí sinh: %s (MSSV: %s)",
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
                if (m_selectedSessionId > 0 && sub.sessionId != m_selectedSessionId) {
                    continue;
                }

                ImVec4 verdictCol = ImVec4(0.20f, 0.90f, 0.40f, 1.0f);
                std::string vStr = verdictToString(sub.overallVerdict);
                if (sub.overallVerdict == Verdict::WA)       verdictCol = ImVec4(1.00f, 0.30f, 0.30f, 1.0f);
                else if (sub.overallVerdict == Verdict::TLE) verdictCol = ImVec4(1.00f, 0.65f, 0.15f, 1.0f);
                else if (sub.overallVerdict == Verdict::MLE) verdictCol = ImVec4(0.85f, 0.40f, 1.00f, 1.0f);
                else if (sub.overallVerdict == Verdict::CE)  verdictCol = ImVec4(0.90f, 0.20f, 0.20f, 1.0f);

                ImGui::Bullet();
                if (m_currentMode == GradingMode::ICPC) {
                    ImGui::Text("[%s] Bài: %s", sub.sessionName.c_str(), sub.problemCode.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(verdictCol, "[%s]", vStr.c_str());
                    ImGui::SameLine();
                    ImGui::Text("| Thời gian: %u ms | Bộ nhớ: %u KB | Nộp lúc: %s",
                                sub.totalTimeMs, sub.maxMemoryKb, sub.submittedAt.c_str());
                } else {
                    ImGui::Text("[%s] Bài: %s", sub.sessionName.c_str(), sub.problemCode.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(verdictCol, "[%s]", vStr.c_str());
                    ImGui::SameLine();
                    ImGui::Text("| Điểm: %.1f đ | Thời gian: %u ms | Bộ nhớ: %u KB | Nộp lúc: %s",
                                sub.totalScore, sub.totalTimeMs, sub.maxMemoryKb, sub.submittedAt.c_str());
                }
            }
        }
    }

    // Modal popup xác nhận xóa
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
        ImGui::Text("Bạn có chắc chắn muốn xóa thí sinh:");
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s (MSSV: %s)?",
                           m_contestantToDeleteName.c_str(), m_contestantToDeleteId.c_str());
        ImGui::TextDisabled("Tất cả các bài nộp của thí sinh này sẽ bị xóa khỏi hệ thống.");
        ImGui::Spacing();

        if (ImGui::Button("Xóa Vĩnh Viễn", ImVec2(140, 30))) {
            SubmissionDAO::instance().deleteContestant(m_contestantToDeleteId);
            m_showDeleteConfirmPopup = false;
            m_selectedEntryIndex = -1;
            refresh();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Hủy Bỏ", ImVec2(100, 30))) {
            m_showDeleteConfirmPopup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Modal popup xác nhận xóa phiên
    if (m_showDeleteSessionModal) {
        ImGui::OpenPopup("Xác Nhận Xóa Phiên##Modal");
    }

    if (ImGui::BeginPopupModal("Xác Nhận Xóa Phiên##Modal", &m_showDeleteSessionModal, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "XÁC NHẬN XÓA PHIÊN CHẤM:");
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::Text("Bạn có chắc chắn muốn xóa [Phiên #%d] không?", m_sessionToDeleteId);
        ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.2f, 1.0f), "CẢNH BÁO: Mọi bài nộp và kết quả chấm thuộc phiên này sẽ bị xóa hoàn toàn.");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.80f, 0.22f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.30f, 0.30f, 1.0f));
        if (ImGui::Button("Xóa Phiên Này", ImVec2(140, 30))) {
            SubmissionDAO::instance().deleteSession(m_sessionToDeleteId);
            m_selectedSessionId = 0;
            m_showDeleteSessionModal = false;
            refresh();
            ImGui::CloseCurrentPopup();
        }
        ImGui::PopStyleColor(2);

        ImGui::SameLine(0, 14.0f);
        if (ImGui::Button("Hủy Bỏ", ImVec2(100, 30))) {
            m_showDeleteSessionModal = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

} // namespace loj
