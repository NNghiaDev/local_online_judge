#pragma once

#include <string>
#include <vector>

namespace loj {

class TestcaseManagerView {
public:
    TestcaseManagerView();
    ~TestcaseManagerView() = default;

    void render();

private:
    // Manual Testcase Creation
    char m_targetProblemDir[256] = "testcases/PROBLEM_A";
    int m_testcaseIndex = 1;
    char m_inputContent[4096] = "";
    char m_outputContent[4096] = "";

    // Model Code Runner
    char m_modelSourcePath[256] = "solutions/model_solution.cpp";
    char m_modelTestcaseDir[256] = "testcases/PROBLEM_A";
    char m_modelSingleInput[2048] = "";
    char m_modelSingleOutput[2048] = "";

    // Custom Checker
    char m_customCheckerPath[256] = "";
};

} // namespace loj
