#include "TestcaseManagerView.hpp"
#include "dal/filesystem/FileSystemDAO.hpp"
#include "bll/generator/ModelCodeRunner.hpp"
#include "common/Logger.hpp"
#include <imgui.h>

namespace loj {

TestcaseManagerView::TestcaseManagerView() = default;

void TestcaseManagerView::render() {
    float fullW = ImGui::GetContentRegionAvail().x;

    if (ImGui::BeginTabBar("TestcaseTabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
        // TAB 1: Tạo testcase thủ công
        if (ImGui::BeginTabItem("Tạo Testcase Thủ Công")) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.30f, 0.80f, 1.00f, 1.0f), "SOẠN THẢO VÀ LƯU TESTCASE");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2, "TcPathInputs", false);
            ImGui::SetColumnWidth(0, fullW * 0.60f);
            ImGui::SetColumnWidth(1, fullW * 0.40f);

            ImGui::Text("Thư mục lưu Testcase:");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##tcSaveDir", m_targetProblemDir, sizeof(m_targetProblemDir));

            ImGui::NextColumn();
            ImGui::Text("Số thứ tự Test (Index):");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##tcIdx", &m_testcaseIndex);
            if (m_testcaseIndex < 1) m_testcaseIndex = 1;

            ImGui::Columns(1);
            ImGui::Spacing();

            ImGui::Columns(2, "TcEditors", true);

            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.20f, 1.0f), "DỮ LIỆU ĐẦU VÀO (INPUT):");
            ImGui::InputTextMultiline("##ManualInput", m_inputContent, sizeof(m_inputContent),
                                      ImVec2(-1.0f, 240.0f));

            ImGui::NextColumn();

            ImGui::TextColored(ImVec4(0.20f, 0.85f, 0.40f, 1.0f), "KẾT QUẢ MONG ĐỢI (OUTPUT):");
            ImGui::InputTextMultiline("##ManualOutput", m_outputContent, sizeof(m_outputContent),
                                      ImVec2(-1.0f, 240.0f));

            ImGui::Columns(1);
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.60f, 0.38f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.72f, 0.46f, 1.0f));
            if (ImGui::Button("Lưu Testcase Vào Đĩa", ImVec2(200, 36))) {
                bool ok = FileSystemDAO::instance().saveTestcase(
                    m_targetProblemDir,
                    m_testcaseIndex,
                    m_inputContent,
                    m_outputContent
                );
                if (ok) {
                    m_testcaseIndex++;
                    m_inputContent[0] = '\0';
                    m_outputContent[0] = '\0';
                }
            }
            ImGui::PopStyleColor(2);

            ImGui::SameLine();
            if (ImGui::Button("Xóa Trắng", ImVec2(120, 36))) {
                m_inputContent[0] = '\0';
                m_outputContent[0] = '\0';
            }

            ImGui::EndTabItem();
        }

        // TAB 2: Lời giải mẫu
        if (ImGui::BeginTabItem("Sinh Output Từ Lời Giải Mẫu")) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.55f, 1.0f), "CHẠY LỜI GIẢI MẪU ĐỂ TỰ ĐỘNG SINH CÁC FILE OUTPUT");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Columns(2, "ModelCols", false);
            ImGui::SetColumnWidth(0, fullW * 0.50f);
            ImGui::SetColumnWidth(1, fullW * 0.50f);

            ImGui::Text("File mã nguồn lời giải mẫu (.cpp):");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##modelSrc", m_modelSourcePath, sizeof(m_modelSourcePath));

            ImGui::NextColumn();

            ImGui::Text("Thư mục chứa các file input cần sinh output:");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##modelTcDir", m_modelTestcaseDir, sizeof(m_modelTestcaseDir));

            ImGui::Columns(1);
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.50f, 0.90f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.60f, 0.98f, 1.0f));
            if (ImGui::Button("Biên Dịch & Tự Động Sinh Toàn Bộ File .out", ImVec2(320, 38))) {
                ModelCodeRunner runner;
                runner.generateOutputsForDirectory(m_modelSourcePath, m_modelTestcaseDir);
            }
            ImGui::PopStyleColor(2);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Chạy thử nghiệm một dữ liệu đơn lẻ:");
            ImGui::Columns(2, "InteractiveCols", true);

            ImGui::Text("Dữ liệu đầu vào thử:");
            ImGui::InputTextMultiline("##SingleInput", m_modelSingleInput, sizeof(m_modelSingleInput),
                                      ImVec2(-1.0f, 120.0f));

            if (ImGui::Button("Chạy Thử", ImVec2(140, 30))) {
                ModelCodeRunner runner;
                std::string res = runner.runModelOnInput(m_modelSourcePath, m_modelSingleInput);
                snprintf(m_modelSingleOutput, sizeof(m_modelSingleOutput), "%s", res.c_str());
            }

            ImGui::NextColumn();

            ImGui::Text("Kết quả trả về:");
            ImGui::InputTextMultiline("##SingleOutput", m_modelSingleOutput, sizeof(m_modelSingleOutput),
                                      ImVec2(-1.0f, 120.0f), ImGuiInputTextFlags_ReadOnly);

            ImGui::Columns(1);

            ImGui::EndTabItem();
        }

        // TAB 3: Trình chấm tùy chỉnh
        if (ImGui::BeginTabItem("Trình Chấm Tùy Chỉnh")) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.85f, 0.45f, 0.95f, 1.0f), "CẤU HÌNH TRÌNH CHẤM TÙY CHỈNH (CUSTOM CHECKER)");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Đường dẫn file thực thi Checker (.exe):");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputText("##customChecker", m_customCheckerPath, sizeof(m_customCheckerPath));

            ImGui::Spacing();
            ImGui::Text("Cú pháp: checker.exe <input.txt> <contestant_output.txt> <expected_output.txt>");
            ImGui::BulletText("Mã thoát 0: Đúng (AC)");
            ImGui::BulletText("Mã thoát khác 0: Sai (WA)");

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

} // namespace loj
