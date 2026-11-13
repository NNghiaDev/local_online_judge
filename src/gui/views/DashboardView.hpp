#pragma once

#include "common/Types.hpp"
#include <string>
#include <vector>

namespace loj {

enum class TargetMode {
    SINGLE_FILE,
    SINGLE_CONTESTANT,
    BATCH_DIRECTORY
};

class DashboardView {
public:
    DashboardView();
    ~DashboardView() = default;

    void render();

private:
    TargetMode m_targetMode = TargetMode::SINGLE_FILE;

    // Chấm một file đơn
    char m_singleSourcePath[256] = "submissions/sample_solution.cpp";
    char m_studentId[64] = "22010001";
    char m_studentName[128] = "Nguyễn Văn A";
    char m_problemCode[64] = "SUM";
    char m_testcaseDir[256] = "testcases/SUM";
    int m_timeLimitMs = 1000;
    int m_memoryLimitMb = 256;
    int m_gradingModeIdx = 2; // Mặc định: Equal (Chia đều điểm)

    // Chấm thư mục 1 thí sinh
    char m_contestantFolderPath[256] = "contestants/NguyenVanA_22010001";

    // Chấm hàng loạt nhiều thí sinh
    char m_batchRootPath[256] = "batch_contestants";
    char m_globalTestcasesRoot[256] = "testcases";

    // Cài đặt MySQL / TiDB Cloud
    char m_dbHost[256] = "gateway01.ap-northeast-1.prod.aws.tidbcloud.com";
    int m_dbPort = 4000;
    char m_dbUser[128] = "3MWwCRpirN1aT5H.root";
    char m_dbPass[128] = "xUMcKr5Rdf9ICZvs";
    char m_dbName[128] = "test";

    // Tiến độ chấm & Kết nối CSDL
    float m_progress = 0.0f;
    std::string m_statusMessage = "Hệ thống sẵn sàng";
    bool m_isGrading = false;
    bool m_isConnecting = false;
};

} // namespace loj
