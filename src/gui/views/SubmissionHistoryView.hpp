#pragma once

#include "common/Types.hpp"
#include <vector>
#include <string>

namespace loj {

class SubmissionHistoryView {
public:
    SubmissionHistoryView();
    ~SubmissionHistoryView() = default;

    void render();
    void refresh();

private:
    std::vector<SubmissionSummary> m_submissions;
    int m_selectedSubmissionIndex = -1;

    // Filters
    int m_filterSessionId = 0; // 0: Tất cả
    char m_filterStudent[64] = "";
    int m_filterVerdictIdx = 0; // 0: Tất cả, 1: AC, 2: WA, 3: TLE, 4: MLE, 5: RTE, 6: CE

    std::string m_exportStatus;
};

} // namespace loj
