#include "DashboardView.hpp"
#include "bll/JudgeEngine.hpp"
#include "dal/db/DatabaseConnection.hpp"
#include "dal/filesystem/FileSystemDAO.hpp"
#include "common/Logger.hpp"
#include <imgui.h>
#include <thread>
#include <sstream>
#include <cmath>

namespace loj {

DashboardView::DashboardView() {
    JudgeEngine::instance().setProgressCallback([this](const std::string& prob, int current, int total) {
        if (total > 0) {
            m_progress = static_cast<float>(current) / static_cast<float>(total);
        }
        m_statusMessage = "Đang chấm bài [" + prob + "] (Test " + std::to_string(current) + "/" + std::to_string(total) + ")";
    });
}

void DashboardView::render() {
    // Không dùng BeginChild có thanh cuộn, hiển thị phẳng tự nhiên
    float fullW = ImGui::GetContentRegionAvail().x;
    float colSpacing = 14.0f;

    // =========================================================================
    // 1. CƠ SỞ DỮ LIỆU MYSQL
    // =========================================================================
    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.00f, 1.0f), "CƠ SỞ DỮ LIỆU MYSQL");
    ImGui::SameLine(fullW - 200.0f);

    bool connected = DatabaseConnection::instance().isConnected();
    bool isConnecting = DatabaseConnection::instance().isConnecting();

    if (isConnecting) {
        ImGui::TextColored(ImVec4(0.38f, 0.82f, 1.00f, 1.0f), "[ĐANG KẾT NỐI...]");
    } else if (connected) {
        ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f), "[ĐÃ KẾT NỐI]");
    } else {
        ImGui::TextColored(ImVec4(0.98f, 0.75f, 0.20f, 1.0f), "[LƯU TẠM CỤC BỘ]");
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Dòng 1: Máy chủ, Cổng, Tên Database
    float colW1 = (fullW - colSpacing * 2.0f) * 0.52f;
    float colW2 = (fullW - colSpacing * 2.0f) * 0.18f;
    float colW3 = (fullW - colSpacing * 2.0f) * 0.30f;

    ImGui::Columns(3, "DbRow1Cols", false);
    ImGui::SetColumnWidth(0, colW1);
    ImGui::SetColumnWidth(1, colW2);
    ImGui::SetColumnWidth(2, colW3);

    ImGui::Text("Máy chủ (Host):");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##dbHost", m_dbHost, sizeof(m_dbHost));

    ImGui::NextColumn();
    ImGui::Text("Cổng (Port):");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("##dbPort", &m_dbPort);

    ImGui::NextColumn();
    ImGui::Text("Tên Database (TiDB: test):");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##dbName", m_dbName, sizeof(m_dbName));

    ImGui::Columns(1);
    ImGui::Spacing();

    // Dòng 2: Tài khoản, Mật khẩu, Nút kết nối
    float colWUser = (fullW - colSpacing * 2.0f) * 0.38f;
    float colWPass = (fullW - colSpacing * 2.0f) * 0.38f;
    float colWBtn  = (fullW - colSpacing * 2.0f) * 0.24f;

    ImGui::Columns(3, "DbRow2Cols", false);
    ImGui::SetColumnWidth(0, colWUser);
    ImGui::SetColumnWidth(1, colWPass);
    ImGui::SetColumnWidth(2, colWBtn);

    ImGui::Text("Tài khoản:");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##dbUser", m_dbUser, sizeof(m_dbUser));

    ImGui::NextColumn();
    ImGui::Text("Mật khẩu:");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##dbPass", m_dbPass, sizeof(m_dbPass), ImGuiInputTextFlags_Password);

    ImGui::NextColumn();
    ImGui::Text("Thao tác:");

    if (isConnecting) {
        ImGui::BeginDisabled();
        ImGui::Button("Đang kết nối...", ImVec2(-1.0f, 32.0f));
        ImGui::EndDisabled();
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.95f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.65f, 1.00f, 1.0f));
        if (ImGui::Button("Kết Nối Lại CSDL", ImVec2(-1.0f, 32.0f))) {
            DbConfig cfg;
            cfg.host = m_dbHost;
            cfg.port = m_dbPort;
            cfg.user = m_dbUser;
            cfg.password = m_dbPass;
            cfg.database = m_dbName;

            // Kết nối chạy hoàn toàn trong thread riêng, GUI không bao giờ bị đơ
            std::thread([cfg]() {
                DatabaseConnection::instance().connect(cfg);
            }).detach();
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Columns(1);

    ImGui::Spacing();
    ImGui::Spacing();

    // =========================================================================
    // 2. ĐỐI TƯỢNG CẦN CHẤM THI
    // =========================================================================
    ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "ĐỐI TƯỢNG CẦN CHẤM THI");
    ImGui::Separator();
    ImGui::Spacing();

    int currentTarget = static_cast<int>(m_targetMode);
    float radioWidth = fullW / 3.0f;

    ImGui::Columns(3, "TargetCols", false);
    ImGui::SetColumnWidth(0, radioWidth);
    ImGui::SetColumnWidth(1, radioWidth);
    ImGui::SetColumnWidth(2, radioWidth);

    ImGui::RadioButton("1. Chấm 1 File Đơn Lẻ (.cpp)", &currentTarget, 0);
    ImGui::NextColumn();
    ImGui::RadioButton("2. Chấm 1 Thư Mục Thí Sinh", &currentTarget, 1);
    ImGui::NextColumn();
    ImGui::RadioButton("3. Chấm Hàng Loạt Cả Lớp", &currentTarget, 2);
    ImGui::Columns(1);

    m_targetMode = static_cast<TargetMode>(currentTarget);

    ImGui::Spacing();
    ImGui::Spacing();

    // =========================================================================
    // 3. THIẾT LẬP ĐƯỜNG DẪN & THÔNG SỐ BÀI THI
    // =========================================================================
    ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.55f, 1.0f), "THIẾT LẬP BÀI THI & GIỚI HẠN");
    ImGui::Separator();
    ImGui::Spacing();

    if (m_targetMode == TargetMode::SINGLE_FILE) {
        ImGui::Columns(2, "SingleParams", false);
        ImGui::SetColumnWidth(0, fullW * 0.50f);
        ImGui::SetColumnWidth(1, fullW * 0.50f);

        ImGui::Text("Đường dẫn file mã nguồn (.cpp):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##singleSrc", m_singleSourcePath, sizeof(m_singleSourcePath));

        ImGui::Spacing();
        ImGui::Columns(2, "SubStud", false);
        ImGui::Text("Mã số sinh viên (MSSV):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##studentId", m_studentId, sizeof(m_studentId));

        ImGui::NextColumn();
        ImGui::Text("Họ và tên thí sinh:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##studentName", m_studentName, sizeof(m_studentName));
        ImGui::Columns(1);

        ImGui::NextColumn();

        ImGui::Text("Mã bài toán:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##probCode", m_problemCode, sizeof(m_problemCode));

        ImGui::Spacing();
        ImGui::Text("Thư mục chứa Testcase:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##tcDir", m_testcaseDir, sizeof(m_testcaseDir));

        ImGui::Columns(1);
    } else if (m_targetMode == TargetMode::SINGLE_CONTESTANT) {
        ImGui::Columns(2, "ContestantParams", false);
        ImGui::SetColumnWidth(0, fullW * 0.50f);
        ImGui::SetColumnWidth(1, fullW * 0.50f);

        ImGui::Text("Thư mục thí sinh (Ví dụ: HoTen_MSSV):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##contestantFolder", m_contestantFolderPath, sizeof(m_contestantFolderPath));

        ImGui::NextColumn();

        ImGui::Text("Thư mục gốc chứa các bộ Testcase:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##globalTcDir1", m_globalTestcasesRoot, sizeof(m_globalTestcasesRoot));

        ImGui::Columns(1);
    } else {
        ImGui::Columns(2, "BatchParams", false);
        ImGui::SetColumnWidth(0, fullW * 0.50f);
        ImGui::SetColumnWidth(1, fullW * 0.50f);

        ImGui::Text("Thư mục gốc chứa bài làm của cả lớp:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##batchRoot", m_batchRootPath, sizeof(m_batchRootPath));

        ImGui::NextColumn();

        ImGui::Text("Thư mục gốc chứa các bộ Testcase:");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##globalTcDir2", m_globalTestcasesRoot, sizeof(m_globalTestcasesRoot));

        ImGui::Columns(1);
    }

    ImGui::Spacing();

    // Giới hạn thời gian, bộ nhớ, chế độ chấm và điểm tối đa của bài
    float limColW = (fullW - colSpacing * 3.0f) * 0.25f;
    ImGui::Columns(4, "LimitsRowCols", false);
    ImGui::SetColumnWidth(0, limColW * 0.9f);
    ImGui::SetColumnWidth(1, limColW * 0.9f);
    ImGui::SetColumnWidth(2, limColW * 1.3f);
    ImGui::SetColumnWidth(3, limColW * 0.9f);

    ImGui::Text("Thời gian (ms):");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("##tleLimit", &m_timeLimitMs);

    ImGui::NextColumn();
    ImGui::Text("Bộ nhớ (MB):");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputInt("##mleLimit", &m_memoryLimitMb);

    ImGui::NextColumn();
    ImGui::Text("Chế độ chấm điểm:");
    const char* modes[] = {
        "1. ICPC (0 hoặc Điểm tối đa, phạt thời gian)",
        "2. IOI (Định nghĩa điểm cụ thể từng test)",
        "3. Equal (Chia đều điểm tối đa theo test)"
    };
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::Combo("##gradingStrategy", &m_gradingModeIdx, modes, IM_ARRAYSIZE(modes));

    ImGui::NextColumn();
    ImGui::Text("Điểm tối đa của bài:");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputDouble("##totalScoreInput", &m_totalScore, 5.0, 10.0, "%.1f");
    if (m_totalScore <= 0.0) m_totalScore = 100.0;

    ImGui::Columns(1);
    ImGui::Spacing();

    // =========================================================================
    // CẤU HÌNH CHI TIẾT & GỢI Ý TRỰC QUAN THEO CHẾ ĐỘ CHẤM
    // =========================================================================
    if (m_gradingModeIdx == 2) { // EQUAL
        ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.00f, 1.0f),
            "* Chế độ Equal: Điểm tối đa (%.1f điểm) sẽ tự động chia đều cho số lượng testcase thực tế của bài (phụ thuộc vào số test phát hiện được, ví dụ 4 test thì mỗi test = %.1f điểm, 5 test thì mỗi test = %.1f điểm).",
            m_totalScore, m_totalScore / 4.0, m_totalScore / 5.0);
    } else if (m_gradingModeIdx == 1) { // IOI
        ImGui::Text("Phân bổ điểm cho từng test (nhập các số cách nhau bởi dấu phẩy, tương ứng với số test của bài):");
        ImGui::SetNextItemWidth(-1.0f);
        ImGui::InputText("##ioiPointsInput", m_ioiPointsStr, sizeof(m_ioiPointsStr));

        // Parse và phân tích tổng điểm hiện tại
        std::vector<double> parsedPoints;
        std::stringstream ss(m_ioiPointsStr);
        std::string token;
        double sumPts = 0.0;
        while (std::getline(ss, token, ',')) {
            try {
                size_t start = token.find_first_not_of(" \t\r\n");
                size_t end = token.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    double val = std::stod(token.substr(start, end - start + 1));
                    parsedPoints.push_back(val);
                    sumPts += val;
                }
            } catch (...) {}
        }

        if (std::abs(sumPts - m_totalScore) < 1e-4) {
            ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.40f, 1.0f),
                "Đã cấu hình cho %d test: Tổng điểm = %.1f / %.1f điểm (Đủ điểm tối đa).",
                static_cast<int>(parsedPoints.size()), sumPts, m_totalScore);
        } else {
            ImGui::TextColored(ImVec4(0.98f, 0.75f, 0.20f, 1.0f),
                "Đã cấu hình cho %d test: Tổng điểm hiện tại = %.1f / %.1f điểm (Nếu số test thực tế nhiều hơn, điểm còn lại tự chia đều).",
                static_cast<int>(parsedPoints.size()), sumPts, m_totalScore);
        }
    } else { // ICPC
        ImGui::TextColored(ImVec4(0.85f, 0.50f, 1.00f, 1.0f),
            "* Chế độ ICPC: Chỉ tính bài giải được (AC toàn bộ test) và thời gian phạt. Sai bất kỳ test nào dừng ngay (0 điểm). Bảng xếp hạng chỉ hiển thị Số Bài Đúng và Thời Gian Phạt.");
    }

    ImGui::Spacing();
    ImGui::Spacing();

    // =========================================================================
    // 4. HÀNH ĐỘNG CHẤM BÀI
    // =========================================================================
    ImGui::Separator();
    ImGui::Spacing();

    if (m_isGrading) {
        ImGui::TextColored(ImVec4(0.20f, 0.75f, 1.00f, 1.0f), "HỆ THỐNG ĐANG CHẤM BÀI...");
        ImGui::ProgressBar(m_progress, ImVec2(-1.0f, 36.0f), m_statusMessage.c_str());
    } else {
        ImGui::Columns(2, "GradingActionRow", false);
        ImGui::SetColumnWidth(0, 340.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.65f, 0.38f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.78f, 0.46f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.08f, 0.52f, 0.30f, 1.0f));

        if (ImGui::Button("  BẮT ĐẦU CHẤM BÀI  ", ImVec2(320, 42))) {
            m_isGrading = true;
            m_progress = 0.0f;
            m_statusMessage = "Đang khởi tạo...";

            std::thread([this]() {
                try {
                    ProblemSpec defaultProblem;
                    defaultProblem.problemCode = m_problemCode;
                    defaultProblem.timeLimitMs = m_timeLimitMs;
                    defaultProblem.memoryLimitMb = m_memoryLimitMb;
                    defaultProblem.gradingMode = static_cast<GradingMode>(m_gradingModeIdx);
                    defaultProblem.totalScore = m_totalScore;

                    // Parse danh sách điểm IOI nếu đang ở chế độ IOI
                    if (defaultProblem.gradingMode == GradingMode::IOI) {
                        std::stringstream ss(m_ioiPointsStr);
                        std::string token;
                        while (std::getline(ss, token, ',')) {
                            try {
                                size_t start = token.find_first_not_of(" \t\r\n");
                                size_t end = token.find_last_not_of(" \t\r\n");
                                if (start != std::string::npos && end != std::string::npos) {
                                    defaultProblem.ioiPoints.push_back(std::stod(token.substr(start, end - start + 1)));
                                }
                            } catch (...) {}
                        }
                    }

                    if (m_targetMode == TargetMode::SINGLE_FILE) {
                        auto testcases = FileSystemDAO::instance().loadTestcases(m_testcaseDir);
                        JudgeEngine::instance().gradeSingleFile(
                            m_singleSourcePath,
                            m_studentId,
                            m_studentName,
                            defaultProblem,
                            testcases
                        );
                    } else if (m_targetMode == TargetMode::SINGLE_CONTESTANT) {
                        auto cf = FileSystemDAO::instance().scanContestantFolder(m_contestantFolderPath);
                        std::vector<ProblemSpec> probs = {defaultProblem};
                        JudgeEngine::instance().gradeContestantFolder(cf, probs, m_globalTestcasesRoot);
                    } else if (m_targetMode == TargetMode::BATCH_DIRECTORY) {
                        std::vector<ProblemSpec> probs = {defaultProblem};
                        JudgeEngine::instance().gradeBatch(m_batchRootPath, probs, m_globalTestcasesRoot);
                    }
                } catch (const std::exception& e) {
                    Logger::instance().error("Lỗi khi chấm: " + std::string(e.what()));
                }
                m_isGrading = false;
                m_progress = 1.0f;
                m_statusMessage = "Chấm bài hoàn tất!";
            }).detach();
        }
        ImGui::PopStyleColor(3);

        ImGui::NextColumn();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
        ImGui::TextColored(ImVec4(0.80f, 0.85f, 0.90f, 1.0f), "Trạng thái: %s", m_statusMessage.c_str());

        ImGui::Columns(1);
    }
}

} // namespace loj
