#include "SubmissionHistoryView.hpp"
#include "dal/db/SubmissionDAO.hpp"
#include "common/Logger.hpp"
#include <imgui.h>
#include <fstream>
#include <iomanip>
#include <algorithm>

namespace loj {

SubmissionHistoryView::SubmissionHistoryView() {
    refresh();
}

void SubmissionHistoryView::refresh() {
    m_submissions = SubmissionDAO::instance().getAllSubmissions();
    if (m_selectedSubmissionIndex >= static_cast<int>(m_submissions.size())) {
        m_selectedSubmissionIndex = -1;
    }
}

void SubmissionHistoryView::render() {
    float availW = ImGui::GetContentRegionAvail().x;
    auto sessions = SubmissionDAO::instance().getAllSessions();

    // =========================================================================
    // THANH CÔNG CỤ & BỘ LỌC TÌM KIẾM
    // =========================================================================
    ImGui::Columns(2, "HistoryHeaderCols", false);
    ImGui::SetColumnWidth(0, availW - 640.0f);
    ImGui::SetColumnWidth(1, 640.0f);

    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.55f, 1.0f), "LỊCH SỬ CHẤM BÀI (TẤT CẢ CÁC PHIÊN)");
    if (!m_exportStatus.empty()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f), "| %s", m_exportStatus.c_str());
    }

    ImGui::NextColumn();

    // 1. Lọc theo Phiên
    std::vector<std::string> sessionNames = {"Tất cả các phiên"};
    int currentSessionComboIdx = 0;
    for (size_t i = 0; i < sessions.size(); ++i) {
        sessionNames.push_back(sessions[i].sessionName);
        if (sessions[i].id == m_filterSessionId) {
            currentSessionComboIdx = static_cast<int>(i + 1);
        }
    }

    ImGui::SetNextItemWidth(150.0f);
    if (ImGui::BeginCombo("##HistorySessionCombo", sessionNames[currentSessionComboIdx].c_str())) {
        for (int i = 0; i < static_cast<int>(sessionNames.size()); ++i) {
            bool isSelected = (currentSessionComboIdx == i);
            if (ImGui::Selectable(sessionNames[i].c_str(), isSelected)) {
                m_filterSessionId = (i == 0) ? 0 : sessions[i - 1].id;
                refresh();
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // 2. Tìm theo MSSV/Tên
    ImGui::SameLine();
    ImGui::SetNextItemWidth(140.0f);
    ImGui::InputTextWithHint("##FilterStudent", "MSSV / Tên...", m_filterStudent, sizeof(m_filterStudent));

    // 3. Lọc theo Kết quả
    ImGui::SameLine();
    const char* verdicts[] = {"Tất cả", "Chỉ AC", "Chỉ WA", "Chỉ TLE", "Chỉ MLE", "Chỉ RTE", "Chỉ CE"};
    ImGui::SetNextItemWidth(90.0f);
    ImGui::Combo("##FilterVerdict", &m_filterVerdictIdx, verdicts, IM_ARRAYSIZE(verdicts));

    // 4. Nút Làm Mới
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.35f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.45f, 0.58f, 1.0f));
    if (ImGui::Button("Làm Mới", ImVec2(80, 28))) {
        refresh();
    }
    ImGui::PopStyleColor(2);

    // 5. Nút Xuất Lịch Sử CSV
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.65f, 0.40f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.78f, 0.48f, 1.0f));
    if (ImGui::Button("Xuất CSV", ImVec2(100, 28))) {
        std::ofstream out("lich_su_cham_bai.csv", std::ios::out | std::ios::binary);
        if (out.is_open()) {
            const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
            out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
            out << "Mã Bài Nộp,Phiên Chấm,Thời Gian Nộp,MSSV,Họ Tên,Bài Tập,Kết Quả,Điểm Số,Thời Gian (ms),Bộ Nhớ (KB),File Mã Nguồn\n";
            for (const auto& s : m_submissions) {
                out << s.id << ",\""
                    << s.sessionName << "\",\""
                    << s.submittedAt << "\",\""
                    << s.studentId << "\",\""
                    << s.studentName << "\",\""
                    << s.problemCode << "\",\""
                    << verdictToString(s.overallVerdict) << "\","
                    << s.totalScore << ","
                    << s.totalTimeMs << ","
                    << s.maxMemoryKb << ",\""
                    << s.sourceFilePath << "\"\n";
            }
            out.close();
            m_exportStatus = "Đã xuất: lich_su_cham_bai.csv";
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Spacing();

    // =========================================================================
    // BẢNG DANH SÁCH BÀI NỘP
    // =========================================================================
    std::string searchFilter = m_filterStudent;
    std::transform(searchFilter.begin(), searchFilter.end(), searchFilter.begin(), ::tolower);

    std::vector<int> filteredIndices;
    for (int i = 0; i < static_cast<int>(m_submissions.size()); ++i) {
        const auto& s = m_submissions[i];
        if (m_filterSessionId > 0 && s.sessionId != m_filterSessionId) {
            continue;
        }

        if (m_filterVerdictIdx > 0) {
            Verdict targetV = Verdict::AC;
            if (m_filterVerdictIdx == 1) targetV = Verdict::AC;
            else if (m_filterVerdictIdx == 2) targetV = Verdict::WA;
            else if (m_filterVerdictIdx == 3) targetV = Verdict::TLE;
            else if (m_filterVerdictIdx == 4) targetV = Verdict::MLE;
            else if (m_filterVerdictIdx == 5) targetV = Verdict::RTE;
            else if (m_filterVerdictIdx == 6) targetV = Verdict::CE;
            if (s.overallVerdict != targetV) continue;
        }

        if (!searchFilter.empty()) {
            std::string idLower = s.studentId;
            std::string nameLower = s.studentName;
            std::transform(idLower.begin(), idLower.end(), idLower.begin(), ::tolower);
            std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
            if (idLower.find(searchFilter) == std::string::npos && nameLower.find(searchFilter) == std::string::npos) {
                continue;
            }
        }

        filteredIndices.push_back(i);
    }

    static ImGuiTableFlags historyTableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable |
                                              ImGuiTableFlags_Hideable | ImGuiTableFlags_RowBg |
                                              ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV |
                                              ImGuiTableFlags_ScrollY;

    float tableHeight = (m_selectedSubmissionIndex >= 0) ? 300.0f : ImGui::GetContentRegionAvail().y - 10.0f;
    if (tableHeight < 180.0f) tableHeight = 180.0f;

    if (ImGui::BeginTable("BangLichSuCham", 9, historyTableFlags, ImVec2(0, tableHeight))) {
        ImGui::TableSetupColumn("Mã (#ID)", ImGuiTableColumnFlags_WidthFixed, 65.0f);
        ImGui::TableSetupColumn("Phiên Chấm", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("MSSV", ImGuiTableColumnFlags_WidthFixed, 130.0f);
        ImGui::TableSetupColumn("Họ Và Tên", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Bài Tập", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Kết Quả", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Điểm Số", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Thời Gian", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Xóa", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        if (filteredIndices.empty()) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextDisabled("Không có bài nộp nào phù hợp với bộ lọc.");
        }

        for (int idx : filteredIndices) {
            const auto& s = m_submissions[idx];
            ImGui::TableNextRow();

            // Mã ID
            ImGui::TableSetColumnIndex(0);
            std::string idStr = "#" + std::to_string(s.id);
            if (ImGui::Selectable(idStr.c_str(), m_selectedSubmissionIndex == idx, ImGuiSelectableFlags_SpanAllColumns)) {
                m_selectedSubmissionIndex = idx;
            }

            // Phiên
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", s.sessionName.c_str());

            // MSSV
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", s.studentId.c_str());

            // Họ tên
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", s.studentName.c_str());

            // Bài tập
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%s", s.problemCode.c_str());

            // Kết quả
            ImGui::TableSetColumnIndex(5);
            ImVec4 vCol = ImVec4(0.20f, 0.90f, 0.40f, 1.0f);
            if (s.overallVerdict == Verdict::WA)       vCol = ImVec4(1.00f, 0.30f, 0.30f, 1.0f);
            else if (s.overallVerdict == Verdict::TLE) vCol = ImVec4(1.00f, 0.65f, 0.15f, 1.0f);
            else if (s.overallVerdict == Verdict::MLE) vCol = ImVec4(0.85f, 0.40f, 1.00f, 1.0f);
            else if (s.overallVerdict == Verdict::CE)  vCol = ImVec4(0.90f, 0.20f, 0.20f, 1.0f);
            ImGui::TextColored(vCol, "%s", verdictToString(s.overallVerdict).c_str());

            // Điểm
            ImGui::TableSetColumnIndex(6);
            ImGui::Text("%.1f", s.totalScore);

            // Thời gian
            ImGui::TableSetColumnIndex(7);
            ImGui::Text("%u ms", s.totalTimeMs);

            // Nút xóa
            ImGui::TableSetColumnIndex(8);
            std::string btnDel = "X##" + std::to_string(s.id);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.20f, 0.20f, 0.8f));
            if (ImGui::Button(btnDel.c_str(), ImVec2(50, 20))) {
                SubmissionDAO::instance().deleteSubmission(s.id);
                refresh();
            }
            ImGui::PopStyleColor();
        }

        ImGui::EndTable();
    }

    // =========================================================================
    // CHI TIẾT BÀI NỘP ĐƯỢC CHỌN (TESTCASE & LOG BIÊN DỊCH)
    // =========================================================================
    if (m_selectedSubmissionIndex >= 0 && m_selectedSubmissionIndex < static_cast<int>(m_submissions.size())) {
        const auto& selected = m_submissions[m_selectedSubmissionIndex];
        ImGui::Spacing();
        ImGui::Separator();

        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.00f, 1.0f),
            "CHI TIẾT BÀI NỘP #%d: %s (MSSV: %s) - BÀI [%s] - %s",
            selected.id, selected.studentName.c_str(), selected.studentId.c_str(),
            selected.problemCode.c_str(), selected.sessionName.c_str());
        ImGui::Text("Đường dẫn file: %s | Thời điểm nộp: %s", selected.sourceFilePath.c_str(), selected.submittedAt.c_str());

        // Nếu bị lỗi biên dịch (CE), hiển thị toàn bộ log biên dịch của g++
        if (selected.overallVerdict == Verdict::CE) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Thông báo lỗi biên dịch từ g++:");
            ImGui::BeginChild("CEErrorBox", ImVec2(0, 140), true);
            ImGui::TextUnformatted(selected.compilerOutput.c_str());
            ImGui::EndChild();
        } else if (!selected.testcaseResults.empty()) {
            ImGui::Spacing();
            ImGui::Text("Chi tiết kết quả từng testcase:");
            if (ImGui::BeginTable("TCRowsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(0, 140))) {
                ImGui::TableSetupColumn("Test", ImGuiTableColumnFlags_WidthFixed, 60.0f);
                ImGui::TableSetupColumn("Kết Quả", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("Thời Gian", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                ImGui::TableSetupColumn("Bộ Nhớ (RAM)", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                ImGui::TableSetupColumn("Ghi Chú / Chi Tiết Lỗi", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableHeadersRow();

                for (const auto& tc : selected.testcaseResults) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("#%d", tc.testcaseIndex);

                    ImGui::TableSetColumnIndex(1);
                    ImVec4 c = (tc.verdict == Verdict::AC) ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
                    ImGui::TextColored(c, "%s", verdictToString(tc.verdict).c_str());

                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("%u ms", tc.executionTimeMs);

                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%u KB", tc.peakMemoryKb);

                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%s", tc.errorMessage.empty() ? "-" : tc.errorMessage.c_str());
                }
                ImGui::EndTable();
            }
        }
    }
}

} // namespace loj
