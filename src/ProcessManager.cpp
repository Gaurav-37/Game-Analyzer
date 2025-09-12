#include "ProcessManager.h"
#include <iostream>
#include <algorithm>
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>

std::vector<ProcessManager::WindowInfo> ProcessManager::windowList;

ProcessManager::ProcessManager() : initialized(false) {
}

ProcessManager::~ProcessManager() {
    shutdown();
}

bool ProcessManager::initialize() {
    initialized = true;
    return true;
}

void ProcessManager::shutdown() {
    initialized = false;
}

std::vector<ProcessInfo> ProcessManager::getProcesses() {
    std::vector<ProcessInfo> processes;
    
    if (!initialized) {
        return processes;
    }
    
    // Get all process IDs
    DWORD processIds[1024];
    DWORD bytesReturned;
    
    if (!EnumProcesses(processIds, sizeof(processIds), &bytesReturned)) {
        return processes;
    }
    
    DWORD processCount = bytesReturned / sizeof(DWORD);
    
    // Get window titles for all processes
    windowList.clear();
    EnumWindows(EnumWindowsProc, 0);
    
    // Create process info for each PID
    for (DWORD i = 0; i < processCount; i++) {
        DWORD pid = processIds[i];
        
        if (pid == 0) continue; // Skip system idle process
        
        ProcessInfo process;
        process.pid = pid;
        process.name = getProcessName(pid);
        
        if (process.name.empty()) {
            continue; // Skip processes we can't access
        }
        
        // Find window title for this process
        for (const auto& window : windowList) {
            if (window.pid == pid && !window.title.empty()) {
                process.windowTitle = window.title;
                break;
            }
        }
        
        process.executablePath = getExecutablePath(pid);
        process.parentPid = getParentPid(pid);
        process.threadCount = getThreadCount(pid);
        process.workingSetSize = getWorkingSetSize(pid);
        process.priority = getPriority(pid);
        
        processes.push_back(process);
    }
    
    // Sort by name for easier browsing
    std::sort(processes.begin(), processes.end(), 
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.name < b.name;
              });
    
    return processes;
}

std::vector<ProcessInfo> ProcessManager::getProcessesByName(const std::string& name) {
    auto allProcesses = getProcesses();
    std::vector<ProcessInfo> filtered;
    
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    
    for (const auto& process : allProcesses) {
        std::string lowerProcessName = process.name;
        std::transform(lowerProcessName.begin(), lowerProcessName.end(), lowerProcessName.begin(), ::tolower);
        
        if (lowerProcessName.find(lowerName) != std::string::npos) {
            filtered.push_back(process);
        }
    }
    
    return filtered;
}

ProcessInfo ProcessManager::getProcessById(DWORD pid) {
    ProcessInfo process;
    process.pid = pid;
    
    if (!isProcessRunning(pid)) {
        return process;
    }
    
    process.name = getProcessName(pid);
    process.windowTitle = getWindowTitleByPid(pid);
    process.executablePath = getExecutablePath(pid);
    process.parentPid = getParentPid(pid);
    process.threadCount = getThreadCount(pid);
    process.workingSetSize = getWorkingSetSize(pid);
    process.priority = getPriority(pid);
    
    return process;
}

bool ProcessManager::isProcessRunning(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == nullptr) {
        return false;
    }
    
    DWORD exitCode;
    BOOL result = GetExitCodeProcess(process, &exitCode);
    CloseHandle(process);
    
    return result && (exitCode == STILL_ACTIVE);
}

std::string ProcessManager::getProcessWindowTitle(DWORD pid) {
    return getWindowTitleByPid(pid);
}

std::string ProcessManager::getProcessExecutablePath(DWORD pid) {
    return getExecutablePath(pid);
}

SIZE_T ProcessManager::getProcessMemoryUsage(DWORD pid) {
    return getWorkingSetSize(pid);
}

void ProcessManager::refresh() {
    // Force refresh by clearing any cached data
    windowList.clear();
}

std::string ProcessManager::getProcessName(DWORD pid) {
    HANDLE process = openProcess(pid);
    if (process == nullptr) {
        return "";
    }
    
    char processName[MAX_PATH];
    DWORD size = MAX_PATH;
    
    if (QueryFullProcessImageNameA(process, 0, processName, &size)) {
        std::string fullPath(processName);
        size_t lastSlash = fullPath.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            closeProcess(process);
            return fullPath.substr(lastSlash + 1);
        }
        closeProcess(process);
        return fullPath;
    }
    
    closeProcess(process);
    return "";
}

std::string ProcessManager::getWindowTitleByPid(DWORD pid) {
    for (const auto& window : windowList) {
        if (window.pid == pid) {
            return window.title;
        }
    }
    return "";
}

std::string ProcessManager::getExecutablePath(DWORD pid) {
    HANDLE process = openProcess(pid);
    if (process == nullptr) {
        return "";
    }
    
    char path[MAX_PATH];
    DWORD size = MAX_PATH;
    
    if (QueryFullProcessImageNameA(process, 0, path, &size)) {
        closeProcess(process);
        return std::string(path);
    }
    
    closeProcess(process);
    return "";
}

SIZE_T ProcessManager::getWorkingSetSize(DWORD pid) {
    HANDLE process = openProcess(pid);
    if (process == nullptr) {
        return 0;
    }
    
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
        closeProcess(process);
        return pmc.WorkingSetSize;
    }
    
    closeProcess(process);
    return 0;
}

DWORD ProcessManager::getThreadCount(DWORD pid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    THREADENTRY32 te;
    te.dwSize = sizeof(THREADENTRY32);
    
    DWORD threadCount = 0;
    if (Thread32First(snapshot, &te)) {
        do {
            if (te.th32OwnerProcessID == pid) {
                threadCount++;
            }
        } while (Thread32Next(snapshot, &te));
    }
    
    CloseHandle(snapshot);
    return threadCount;
}

DWORD ProcessManager::getParentPid(DWORD pid) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    if (Process32First(snapshot, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                CloseHandle(snapshot);
                return pe.th32ParentProcessID;
            }
        } while (Process32Next(snapshot, &pe));
    }
    
    CloseHandle(snapshot);
    return 0;
}

DWORD ProcessManager::getPriority(DWORD pid) {
    HANDLE process = openProcess(pid);
    if (process == nullptr) {
        return 0;
    }
    
    DWORD priority = GetPriorityClass(process);
    closeProcess(process);
    
    return priority;
}

HANDLE ProcessManager::openProcess(DWORD pid, DWORD access) {
    return OpenProcess(access, FALSE, pid);
}

void ProcessManager::closeProcess(HANDLE handle) {
    if (handle != nullptr) {
        CloseHandle(handle);
    }
}

BOOL CALLBACK ProcessManager::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    
    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));
    
    if (strlen(windowTitle) > 0) {
        WindowInfo info;
        info.pid = pid;
        info.title = std::string(windowTitle);
        windowList.push_back(info);
    }
    
    return TRUE;
}
