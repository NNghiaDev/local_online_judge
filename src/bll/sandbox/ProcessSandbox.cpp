#include "ProcessSandbox.hpp"
#include "common/Logger.hpp"

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

#include <chrono>
#include <vector>
#include <iostream>

namespace loj {

#if defined(_WIN32)

void ProcessSandbox::readPipeFully(void* hPipe, std::string& out) {
    HANDLE handle = static_cast<HANDLE>(hPipe);
    char buffer[4096];
    DWORD bytesRead = 0;
    while (ReadFile(handle, buffer, sizeof(buffer), &bytesRead, NULL) && bytesRead > 0) {
        out.append(buffer, bytesRead);
    }
}

void ProcessSandbox::writePipeFully(void* hPipe, const std::string& data) {
    HANDLE handle = static_cast<HANDLE>(hPipe);
    DWORD bytesWritten = 0;
    DWORD totalWritten = 0;
    while (totalWritten < data.size()) {
        DWORD toWrite = static_cast<DWORD>(data.size() - totalWritten);
        if (!WriteFile(handle, data.data() + totalWritten, toWrite, &bytesWritten, NULL)) {
            break;
        }
        totalWritten += bytesWritten;
    }
}

RawExecutionResult ProcessSandbox::execute(const ExecutionConfig& config) {
    RawExecutionResult result;
    result.verdict = Verdict::AC;

    // 1. Create Pipes for stdin, stdout, and stderr
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hChildStd_IN_Rd = NULL;
    HANDLE hChildStd_IN_Wr = NULL;
    HANDLE hChildStd_OUT_Rd = NULL;
    HANDLE hChildStd_OUT_Wr = NULL;
    HANDLE hChildStd_ERR_Rd = NULL;
    HANDLE hChildStd_ERR_Wr = NULL;

    if (!CreatePipe(&hChildStd_IN_Rd, &hChildStd_IN_Wr, &saAttr, 0) ||
        !CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0) ||
        !CreatePipe(&hChildStd_ERR_Rd, &hChildStd_ERR_Wr, &saAttr, 0)) {
        result.verdict = Verdict::RTE;
        result.diagnosticMessage = "Failed to create I/O pipes";
        return result;
    }

    // Ensure parent ends of the pipes are not inherited by the child
    SetHandleInformation(hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0);

    // 2. Configure Windows Job Object for Memory & Process Sandboxing
    HANDLE hJob = CreateJobObjectW(NULL, NULL);
    if (hJob != NULL) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {0};
        // Enforce hard memory limit and ensure all child processes die if parent terminates
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (config.memoryLimitMb > 0) {
            jeli.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            jeli.ProcessMemoryLimit = static_cast<SIZE_T>(config.memoryLimitMb) * 1024 * 1024;
        }
        SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jeli, sizeof(jeli));
    }

    // 3. Setup Process Startup Information
    STARTUPINFOW si = {0};
    si.cb = sizeof(STARTUPINFOW);
    si.hStdError = hChildStd_ERR_Wr;
    si.hStdOutput = hChildStd_OUT_Wr;
    si.hStdInput = hChildStd_IN_Rd;
    si.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION pi = {0};

    std::wstring cmdLine = config.executablePath.wstring();
    std::wstring workDir = config.workingDirectory.empty() ?
                          config.executablePath.parent_path().wstring() :
                          config.workingDirectory.wstring();

    auto startTime = std::chrono::steady_clock::now();

    // Start process suspended so we can attach Job Object before the first instruction runs
    BOOL success = CreateProcessW(
        NULL,
        cmdLine.data(),
        NULL,
        NULL,
        TRUE, // Inherit pipe handles
        CREATE_SUSPENDED | CREATE_NO_WINDOW,
        NULL,
        workDir.empty() ? NULL : workDir.c_str(),
        &si,
        &pi
    );

    if (!success) {
        DWORD err = GetLastError();
        result.verdict = Verdict::RTE;
        result.diagnosticMessage = "CreateProcessW failed with error code: " + std::to_string(err);
        CloseHandle(hChildStd_IN_Rd);  CloseHandle(hChildStd_IN_Wr);
        CloseHandle(hChildStd_OUT_Rd); CloseHandle(hChildStd_OUT_Wr);
        CloseHandle(hChildStd_ERR_Rd); CloseHandle(hChildStd_ERR_Wr);
        if (hJob) CloseHandle(hJob);
        return result;
    }

    // Attach to Job Object
    if (hJob != NULL) {
        AssignProcessToJobObject(hJob, pi.hProcess);
    }

    // Resume execution
    ResumeThread(pi.hThread);

    // Close child-side pipe handles in parent process so EOF will be received on output pipes
    CloseHandle(hChildStd_IN_Rd);
    CloseHandle(hChildStd_OUT_Wr);
    CloseHandle(hChildStd_ERR_Wr);

    // Feed stdin to child process
    if (!config.inputData.empty()) {
        writePipeFully(hChildStd_IN_Wr, config.inputData);
    }
    // Close write end of stdin to signal EOF to the contestant's program
    CloseHandle(hChildStd_IN_Wr);

    // 4. Wait with Time Limit (TLE enforcement)
    DWORD waitResult = WaitForSingleObject(pi.hProcess, config.timeLimitMs);
    auto endTime = std::chrono::steady_clock::now();
    result.executionTimeMs = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()
    );

    bool timedOut = false;
    if (waitResult == WAIT_TIMEOUT) {
        timedOut = true;
        result.verdict = Verdict::TLE;
        result.executionTimeMs = config.timeLimitMs;
        result.diagnosticMessage = "Time Limit Exceeded (" + std::to_string(config.timeLimitMs) + " ms)";
        
        // Terminate entire job object tree
        if (hJob != NULL) {
            TerminateJobObject(hJob, 1);
        } else {
            TerminateProcess(pi.hProcess, 1);
        }
        WaitForSingleObject(pi.hProcess, 500);
    }

    // 5. Query Peak Memory Consumption (MLE detection)
    SIZE_T peakMemoryBytes = 0;
    if (hJob != NULL) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION queryInfo = {0};
        if (QueryInformationJobObject(hJob, JobObjectExtendedLimitInformation, &queryInfo, sizeof(queryInfo), NULL)) {
            peakMemoryBytes = queryInfo.PeakProcessMemoryUsed;
        }
    }
    if (peakMemoryBytes == 0) {
        PROCESS_MEMORY_COUNTERS pmc = {0};
        if (GetProcessMemoryInfo(pi.hProcess, &pmc, sizeof(pmc))) {
            peakMemoryBytes = pmc.PeakWorkingSetSize;
        }
    }

    result.peakMemoryKb = static_cast<uint32_t>(peakMemoryBytes / 1024);
    uint32_t memoryLimitKb = config.memoryLimitMb * 1024;
    if (config.memoryLimitMb > 0 && result.peakMemoryKb > memoryLimitKb) {
        result.verdict = Verdict::MLE;
        result.diagnosticMessage = "Memory Limit Exceeded (" + std::to_string(result.peakMemoryKb / 1024) +
                                   " MB > " + std::to_string(config.memoryLimitMb) + " MB)";
    }

    // 6. Read stdout and stderr
    readPipeFully(hChildStd_OUT_Rd, result.stdOut);
    readPipeFully(hChildStd_ERR_Rd, result.stdErr);
    CloseHandle(hChildStd_OUT_Rd);
    CloseHandle(hChildStd_ERR_Rd);

    // 7. Check Exit Code (RTE detection)
    DWORD exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    result.exitCode = exitCode;

    if (!timedOut && result.verdict != Verdict::MLE) {
        if (exitCode != 0) {
            result.verdict = Verdict::RTE;
            // Check common NTSTATUS error codes
            if (exitCode == 0xC0000005) {
                result.diagnosticMessage = "Runtime Error: Access Violation (Segmentation Fault - 0xC0000005)";
            } else if (exitCode == 0xC00000FD) {
                result.diagnosticMessage = "Runtime Error: Stack Overflow (0xC00000FD)";
            } else if (exitCode == 0xC0000094) {
                result.diagnosticMessage = "Runtime Error: Integer Division by Zero (0xC0000094)";
            } else {
                result.diagnosticMessage = "Runtime Error: Process exited with non-zero code " + std::to_string(exitCode);
            }
        }
    }

    // Clean up handles
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (hJob != NULL) {
        CloseHandle(hJob);
    }

    return result;
}

#else

RawExecutionResult ProcessSandbox::execute(const ExecutionConfig& config) {
    RawExecutionResult res;
    res.verdict = Verdict::RTE;
    res.diagnosticMessage = "ProcessSandbox currently only targets Windows";
    return res;
}

#endif

} // namespace loj
