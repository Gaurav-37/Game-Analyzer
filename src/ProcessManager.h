#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <memory>

struct ProcessInfo {
    DWORD pid;
    std::string name;
    std::string windowTitle;
    std::string executablePath;
    DWORD parentPid;
    DWORD threadCount;
    SIZE_T workingSetSize;
    DWORD priority;
    
    ProcessInfo() : pid(0), parentPid(0), threadCount(0), workingSetSize(0), priority(0) {}
    ProcessInfo(DWORD p, const std::string& n) : pid(p), name(n), parentPid(0), threadCount(0), workingSetSize(0), priority(0) {}
};

class ProcessManager {
private:
    bool initialized;
    
public:
    ProcessManager();
    ~ProcessManager();
    
    bool initialize();
    void shutdown();
    
    // Get all running processes
    std::vector<ProcessInfo> getProcesses();
    
    // Get processes by name (case-insensitive)
    std::vector<ProcessInfo> getProcessesByName(const std::string& name);
    
    // Get process by PID
    ProcessInfo getProcessById(DWORD pid);
    
    // Check if process is still running
    bool isProcessRunning(DWORD pid);
    
    // Get process window title
    std::string getProcessWindowTitle(DWORD pid);
    
    // Get process executable path
    std::string getProcessExecutablePath(DWORD pid);
    
    // Get process memory usage
    SIZE_T getProcessMemoryUsage(DWORD pid);
    
    // Refresh process list
    void refresh();
    
private:
    std::string getProcessName(DWORD pid);
    std::string getWindowTitleByPid(DWORD pid);
    std::string getExecutablePath(DWORD pid);
    SIZE_T getWorkingSetSize(DWORD pid);
    DWORD getThreadCount(DWORD pid);
    DWORD getParentPid(DWORD pid);
    DWORD getPriority(DWORD pid);
    
    // Helper function to get process handle
    HANDLE openProcess(DWORD pid, DWORD access = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ);
    void closeProcess(HANDLE handle);
    
    // Window enumeration callback
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    
    struct WindowInfo {
        DWORD pid;
        std::string title;
    };
    static std::vector<WindowInfo> windowList;
};
