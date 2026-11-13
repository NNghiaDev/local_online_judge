#include "ConsoleLogView.hpp"
#include "common/Logger.hpp"
#include <imgui.h>
#include <algorithm>

namespace loj {

ConsoleLogView::ConsoleLogView() = default;

void ConsoleLogView::render() {
    float availW = ImGui::GetContentRegionAvail().x;

    // Header Toolbar dàn đều không thanh cuộn
    ImGui::Columns(2, "ConsoleBarCols", false);
    ImGui::SetColumnWidth(0, availW - 300.0f);
    ImGui::SetColumnWidth(1, 300.0f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.30f, 0.42f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.38f, 0.52f, 1.0f));
    if (ImGui::Button("Xóa Nhật Ký", ImVec2(130, 28))) {
        Logger::instance().clear();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::Checkbox("Tự động cuộn xuống (Auto-scroll)", &m_autoScroll);

    ImGui::NextColumn();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##LogFilter", "Tìm kiếm trong log...", m_filterBuffer, sizeof(m_filterBuffer));

    ImGui::Columns(1);
    ImGui::Separator();
    ImGui::Spacing();

    // Vùng hiển thị log
    float scrollHeight = ImGui::GetContentRegionAvail().y - 10.0f;
    if (scrollHeight < 150.0f) scrollHeight = 150.0f;

    ImGui::BeginChild("ConsoleScrollArea", ImVec2(0, scrollHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

    auto logs = Logger::instance().getLogs();
    std::string filterStr = m_filterBuffer;
    std::transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

    if (logs.empty()) {
        ImGui::TextDisabled("Nhật ký trống.");
    }

    for (const auto& entry : logs) {
        if (!filterStr.empty()) {
            std::string msgLower = entry.message;
            std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);
            if (msgLower.find(filterStr) == std::string::npos) {
                continue;
            }
        }

        ImVec4 colTimestamp = ImVec4(0.55f, 0.65f, 0.78f, 1.0f);
        ImVec4 colMessage = ImVec4(0.92f, 0.94f, 0.98f, 1.0f);

        if (entry.level == LogLevel::SUCCESS) {
            colMessage = ImVec4(0.25f, 0.92f, 0.50f, 1.0f);
        } else if (entry.level == LogLevel::WARNING) {
            colMessage = ImVec4(1.00f, 0.82f, 0.25f, 1.0f);
        } else if (entry.level == LogLevel::ERR) {
            colMessage = ImVec4(1.00f, 0.38f, 0.38f, 1.0f);
        } else if (entry.level == LogLevel::INFO) {
            colMessage = ImVec4(0.38f, 0.82f, 1.00f, 1.0f);
        }

        ImGui::TextColored(colTimestamp, "[%s]", entry.timestamp.c_str());
        ImGui::SameLine();
        ImGui::TextColored(colMessage, "%s", entry.message.c_str());
    }

    if (m_autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

} // namespace loj
