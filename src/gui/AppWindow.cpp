#include "AppWindow.hpp"
#include "common/Logger.hpp"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <filesystem>

namespace loj {

static void glfwErrorCallback(int error, const char* description) {
    Logger::instance().error("Lỗi GLFW " + std::to_string(error) + ": " + description);
}

AppWindow::AppWindow() = default;

AppWindow::~AppWindow() {
    shutdown();
}

void AppWindow::setupVibrantModernTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Kích thước & Bo tròn mềm mại hiện đại
    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    style.ScrollbarRounding = 8.0f;

    style.WindowPadding     = ImVec2(14.0f, 14.0f);
    style.FramePadding      = ImVec2(10.0f, 6.0f);
    style.ItemSpacing       = ImVec2(10.0f, 8.0f);
    style.ItemInnerSpacing  = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize     = 14.0f;

    // Bảng màu rực rỡ, hiện đại, độ tương phản cao (Vibrant Slate & Electric Blue)
    colors[ImGuiCol_Text]                  = ImVec4(0.96f, 0.97f, 0.99f, 1.00f); // Trắng sáng rõ chữ
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.55f, 0.60f, 0.68f, 1.00f);
    
    // Nền cửa sổ: Màu Slate sẫm cao cấp (không bị đen kịt u tối)
    colors[ImGuiCol_WindowBg]              = ImVec4(0.12f, 0.14f, 0.19f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.15f, 0.18f, 0.24f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.16f, 0.19f, 0.26f, 0.98f);
    colors[ImGuiCol_Border]                = ImVec4(0.24f, 0.29f, 0.38f, 0.85f); // Viền sáng rõ nét
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Ô nhập liệu (Input frames)
    colors[ImGuiCol_FrameBg]               = ImVec4(0.18f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.24f, 0.29f, 0.40f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.28f, 0.34f, 0.46f, 1.00f);

    // Thanh tiêu đề
    colors[ImGuiCol_TitleBg]               = ImVec4(0.10f, 0.12f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.16f, 0.22f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.10f, 0.12f, 0.16f, 0.75f);

    // Menu bar
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.14f, 0.17f, 0.23f, 1.00f);

    // Thanh cuộn
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.12f, 0.14f, 0.18f, 0.60f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.28f, 0.34f, 0.44f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.35f, 0.42f, 0.55f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.42f, 0.50f, 0.65f, 1.00f);

    // Checkbox & Sliders: Xanh dương điện quang (Electric Blue)
    colors[ImGuiCol_CheckMark]             = ImVec4(0.22f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(0.22f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.38f, 0.75f, 1.00f, 1.00f);

    // Nút bấm: Màu sắc nổi bật, tràn đầy năng lượng
    colors[ImGuiCol_Button]                = ImVec4(0.18f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.25f, 0.55f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.14f, 0.38f, 0.75f, 1.00f);

    // Header & Selection
    colors[ImGuiCol_Header]                = ImVec4(0.22f, 0.32f, 0.46f, 0.80f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.28f, 0.40f, 0.58f, 0.90f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.32f, 0.46f, 0.66f, 1.00f);

    // Tabs
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.20f, 0.28f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.26f, 0.45f, 0.75f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.20f, 0.40f, 0.78f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.14f, 0.17f, 0.24f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.18f, 0.26f, 0.38f, 1.00f);

    // Bảng biểu (Tables)
    colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.20f, 0.25f, 0.34f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.28f, 0.35f, 0.48f, 1.00f);
    colors[ImGuiCol_TableBorderLight]      = ImVec4(0.22f, 0.28f, 0.38f, 0.70f);
    colors[ImGuiCol_TableRowBg]            = ImVec4(0.14f, 0.17f, 0.22f, 0.70f);
    colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.16f, 0.19f, 0.26f, 0.70f);
}

bool AppWindow::initialize(int width, int height, const char* title) {
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit()) {
        Logger::instance().error("Không thể khởi tạo thư viện GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, title, NULL, NULL);
    if (!m_window) {
        Logger::instance().error("Không thể tạo cửa sổ GLFW");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Bật V-Sync chống giật/xé hình

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Nạp Font hỗ trợ Tiếng Việt (Segoe UI / Tahoma)
    const char* fontPath = "C:\\Windows\\Fonts\\segoeui.ttf";
    if (std::filesystem::exists(fontPath)) {
        ImFontConfig fontConfig;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        io.Fonts->AddFontFromFileTTF(fontPath, 18.0f, &fontConfig, io.Fonts->GetGlyphRangesVietnamese());
        Logger::instance().success("Đã tải Font Segoe UI hỗ trợ Tiếng Việt thành công.");
    } else {
        io.Fonts->AddFontDefault();
        Logger::instance().warning("Không tìm thấy Segoe UI, dùng font mặc định.");
    }

    setupVibrantModernTheme();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Logger::instance().success("Giao diện Local Online Judge đã sẵn sàng!");
    return true;
}

void AppWindow::renderUI() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                   ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 14.0f));
    ImGui::Begin("MainWorkspace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // =========================================================================
    // THANH TIÊU ĐỀ & ĐIỀU HƯỚNG DÀN ĐỀU (KHÔNG BỊ CUỘN, KHÔNG BỊ CHE)
    // =========================================================================
    ImGui::BeginChild("HeaderBanner", ImVec2(0, 84), true,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    ImGui::TextColored(ImVec4(0.22f, 0.70f, 1.00f, 1.0f), "LOCAL ONLINE JUDGE");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.70f, 0.75f, 0.85f, 1.0f), "|  Hệ thống chấm thi lập trình tự động");

    ImGui::Spacing();

    // Dàn đều 4 nút điều hướng theo toàn bộ chiều rộng hiện có
    float availWidth = ImGui::GetContentRegionAvail().x;
    float tabSpacing = 8.0f;
    float tabWidth = (availWidth - (tabSpacing * 3.0f)) / 4.0f;
    if (tabWidth < 160.0f) tabWidth = 160.0f;

    auto renderTabButton = [this, tabWidth](int tabIdx, const char* label, const ImVec4& activeCol) {
        bool isActive = (m_activeTab == tabIdx);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, activeCol);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(activeCol.x * 1.15f, activeCol.y * 1.15f, activeCol.z * 1.15f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.22f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.32f, 0.44f, 1.0f));
        }

        if (ImGui::Button(label, ImVec2(tabWidth, 36))) {
            m_activeTab = tabIdx;
        }
        ImGui::PopStyleColor(2);
    };

    renderTabButton(0, "CHẤM BÀI & DASHBOARD", ImVec4(0.18f, 0.50f, 0.95f, 1.0f));
    ImGui::SameLine(0, tabSpacing);
    renderTabButton(1, "BẢNG XẾP HẠNG",       ImVec4(0.95f, 0.65f, 0.15f, 1.0f));
    ImGui::SameLine(0, tabSpacing);
    renderTabButton(2, "QUẢN LÝ TESTCASE",      ImVec4(0.15f, 0.75f, 0.50f, 1.0f));
    ImGui::SameLine(0, tabSpacing);
    renderTabButton(3, "NHẬT KÝ CONSOLE",       ImVec4(0.65f, 0.35f, 0.95f, 1.0f));

    ImGui::EndChild();

    ImGui::Spacing();

    // Vùng nội dung hiển thị theo tab đang chọn
    if (m_activeTab == 0) {
        m_dashboardView.render();
    } else if (m_activeTab == 1) {
        m_leaderboardView.render();
    } else if (m_activeTab == 2) {
        m_testcaseView.render();
    } else if (m_activeTab == 3) {
        m_consoleView.render();
    }

    ImGui::End();
}

void AppWindow::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderUI();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        // Nền phía sau: Màu Slate sâu thanh lịch
        glClearColor(0.09f, 0.11f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

void AppWindow::shutdown() {
    if (m_window) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_window);
        m_window = nullptr;
        glfwTerminate();
    }
}

} // namespace loj
