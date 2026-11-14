#pragma once

#include "views/DashboardView.hpp"
#include "views/TestcaseManagerView.hpp"
#include "views/LeaderboardView.hpp"
#include "views/SubmissionHistoryView.hpp"
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
    LeaderboardView m_leaderboardView;
    SubmissionHistoryView m_historyView;
    TestcaseManagerView m_testcaseView;
    ConsoleLogView m_consoleView;

    int m_activeTab = 0; // 0: Chấm bài, 1: Bảng xếp hạng, 2: Lịch sử chấm, 3: Testcase, 4: Nhật ký Console
};

} // namespace loj
