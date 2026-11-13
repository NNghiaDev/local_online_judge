#include "gui/AppWindow.hpp"
#include "common/Logger.hpp"

#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    // Tự động định vị thư mục gốc của dự án nếu chạy từ build/bin
    try {
        namespace fs = std::filesystem;
        fs::path exePath = (argc > 0 && argv[0]) ? fs::absolute(argv[0]) : fs::current_path();
        fs::path candidate = exePath.parent_path();
        
        while (!candidate.empty() && candidate != candidate.root_path()) {
            if (fs::exists(candidate / "testcases") || fs::exists(candidate / "CMakeLists.txt")) {
                fs::current_path(candidate);
                break;
            }
            candidate = candidate.parent_path();
        }
    } catch (...) {
        // Fallback to default current_path
    }

    loj::Logger::instance().info("Khởi động Local Online Judge...");
    loj::Logger::instance().info("Thư mục làm việc: " + std::filesystem::current_path().string());

    loj::AppWindow app;
    if (!app.initialize(1440, 880, "LOCAL ONLINE JUDGE - HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG")) {
        std::cerr << "Lỗi: Không thể khởi tạo cửa sổ ứng dụng.\n";
        return 1;
    }

    loj::Logger::instance().success("Hệ thống đã sẵn sàng chấm bài.");
    app.run();
    app.shutdown();

    loj::Logger::instance().info("Đã đóng ứng dụng.");
    return 0;
}
