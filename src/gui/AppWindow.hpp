#pragma once

#include "views/DashboardView.hpp"
#include "views/TestcaseManagerView.hpp"
#include "views/LeaderboardView.hpp"
#include "views/ConsoleLogView.hpp"

struct GLFWwindow;

namespace loj {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

    bool initialize(int width = 1440, int height = 900, const char* title = "HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG - LOCAL ONLINE JUDGE");
    void run();
    void shutdown();

private:
    void setupVibrantModernTheme();
    void renderUI();

    GLFWwindow* m_window = nullptr;
    DashboardView m_dashboardView;
    TestcaseManagerView m_testcaseView;
    LeaderboardView m_leaderboardView;
    ConsoleLogView m_consoleView;

    int m_activeTab = 0; // 0: Chấm bài & Dashboard, 1: Bảng xếp hạng, 2: Testcase & Lời giải mẫu, 3: Nhật ký hệ thống
};

} // namespace loj
