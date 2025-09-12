#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>

// Real process information structure
struct ProcessInfo {
    DWORD pid;
    std::string name;
    std::string windowTitle;
    
    ProcessInfo(DWORD p, const std::string& n, const std::string& w = "") 
        : pid(p), name(n), windowTitle(w) {}
};

// Real memory reading functionality
class MemoryReader {
public:
    static bool readMemory(DWORD pid, uintptr_t address, void* buffer, size_t size) {
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return false;
        }
        
        SIZE_T bytesRead;
        bool success = ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, &bytesRead);
        CloseHandle(hProcess);
        
        return success && (bytesRead == size);
    }
};

// Enhanced GUI with real process functionality
class RealGameAnalyzerGUI {
private:
    HWND hwnd;
    HWND hListBox;
    HWND hAddButton;
    HWND hStartButton;
    HWND hExportButton;
    HWND hRefreshButton;
    HWND hStatusLabel;
    HWND hMemList;
    HWND hAddressEdit;
    HWND hNameEdit;
    HWND hSearchEdit;
    
    std::vector<ProcessInfo> processes;
    std::vector<std::pair<std::string, uintptr_t>> memoryAddresses;
    std::atomic<bool> monitoring;
    ProcessInfo* selectedProcess;
    bool showSystemProcesses;
    
public:
    RealGameAnalyzerGUI() : hwnd(nullptr), monitoring(false), selectedProcess(nullptr), showSystemProcesses(false) {
        refreshProcesses();
    }
    
    bool createWindow() {
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "RealGameAnalyzerWindow";
        
        if (!RegisterClassEx(&wc)) {
            return false;
        }
        
        hwnd = CreateWindowEx(
            0,
            "RealGameAnalyzerWindow",
            "Game Analyzer - Real Process Monitor",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            900, 700,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            this
        );
        
        if (!hwnd) {
            return false;
        }
        
        createControls();
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        
        return true;
    }
    
    void createControls() {
        // Process List
        CreateWindow("STATIC", "Select Process:", WS_VISIBLE | WS_CHILD,
            20, 20, 120, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hListBox = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            20, 50, 300, 200, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hRefreshButton = CreateWindow("BUTTON", "Refresh Processes", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            330, 50, 120, 25, hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("BUTTON", "Show System Processes", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            330, 80, 150, 25, hwnd, (HMENU)5, GetModuleHandle(nullptr), nullptr);
        
        // Search functionality
        CreateWindow("STATIC", "Search:", WS_VISIBLE | WS_CHILD,
            20, 260, 50, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hSearchEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            70, 260, 200, 25, hwnd, (HMENU)6, GetModuleHandle(nullptr), nullptr);
        
        // Populate process list
        refreshProcessList();
        
        // Memory Address Input
        CreateWindow("STATIC", "Add Memory Address:", WS_VISIBLE | WS_CHILD,
            20, 295, 150, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("STATIC", "Address (hex):", WS_VISIBLE | WS_CHILD,
            20, 325, 100, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hAddressEdit = CreateWindow("EDIT", "0x", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            20, 350, 150, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("STATIC", "Name:", WS_VISIBLE | WS_CHILD,
            180, 325, 50, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hNameEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            180, 350, 100, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Buttons
        hAddButton = CreateWindow("BUTTON", "Add Address", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 385, 100, 30, hwnd, (HMENU)1, GetModuleHandle(nullptr), nullptr);
        
        hStartButton = CreateWindow("BUTTON", "Start Monitoring", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            130, 385, 120, 30, hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
        
        hExportButton = CreateWindow("BUTTON", "Export Data", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, 385, 100, 30, hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
        
        // Status Label
        hStatusLabel = CreateWindow("STATIC", "Ready - Select a process to begin", WS_VISIBLE | WS_CHILD,
            20, 425, 500, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Memory Addresses List
        CreateWindow("STATIC", "Memory Addresses:", WS_VISIBLE | WS_CHILD,
            20, 455, 150, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hMemList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            20, 485, 500, 150, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Add some sample memory addresses
        addMemoryAddress("0x12345678", "Health");
        addMemoryAddress("0x87654321", "Ammo");
        addMemoryAddress("0xABCDEF00", "Score");
    }
    
    bool isUserApplication(const std::string& processName) {
        std::string lowerName = processName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        // Comprehensive list of system/background processes to exclude
        std::vector<std::string> systemProcesses = {
            "system", "idle", "smss.exe", "csrss.exe", "wininit.exe", "winlogon.exe",
            "services.exe", "lsass.exe", "svchost.exe", "dwm.exe", "explorer.exe",
            "taskhost.exe", "taskhostw.exe", "runtimebroker.exe", "searchapp.exe",
            "conhost.exe", "audiodg.exe", "spoolsv.exe", "dllhost.exe", "wmiprvse.exe",
            "sihost.exe", "ctfmon.exe", "fontdrvhost.exe", "registry", "memory compression",
            "secure system", "nvcontainer.exe", "nvidia web helper.exe", "razer synapse",
            "asusoptimizationstartup", "asus_framework.exe", "textinputhost.exe",
            "securityhealthsystray.exe", "startmenuexperiencehost.exe", "steamwebhelper.exe",
            "steamservice.exe", "steamclient", "steamerrorreporter", "steamtours",
            "steamcompositor", "steamoverlayui", "steamwebhelper", "steamworks",
            "nvidia", "amd", "intel", "microsoft", "windows", "system32", "syswow64",
            "program files", "programdata", "appdata", "localappdata", "temp",
            "antimalware", "defender", "security", "update", "installer", "setup",
            "service", "daemon", "helper", "agent", "monitor", "tray", "notification",
            "background", "host", "broker", "manager", "controller", "driver",
            "framework", "runtime", "core", "engine", "platform", "sdk", "api"
        };
        
        // Check if it's a system process
        for (const auto& sysProc : systemProcesses) {
            if (lowerName.find(sysProc) != std::string::npos) {
                return false;
            }
        }
        
        // Only include processes that look like actual user applications
        // Must have .exe extension and not be in system directories
        if (lowerName.find(".exe") != std::string::npos) {
            // Exclude if it contains system-related keywords
            if (lowerName.find("system") != std::string::npos ||
                lowerName.find("windows") != std::string::npos ||
                lowerName.find("microsoft") != std::string::npos ||
                lowerName.find("service") != std::string::npos ||
                lowerName.find("helper") != std::string::npos ||
                lowerName.find("host") != std::string::npos ||
                lowerName.find("broker") != std::string::npos ||
                lowerName.find("container") != std::string::npos ||
                lowerName.find("framework") != std::string::npos ||
                lowerName.find("optimization") != std::string::npos ||
                lowerName.find("synapse") != std::string::npos ||
                lowerName.find("webhelper") != std::string::npos) {
                return false;
            }
            return true;
        }
        
        return false;
    }
    
    void refreshProcesses() {
        processes.clear();
        
        // Get all processes
        DWORD processIds[1024];
        DWORD cbNeeded;
        
        if (EnumProcesses(processIds, sizeof(processIds), &cbNeeded)) {
            DWORD processCount = cbNeeded / sizeof(DWORD);
            
            for (DWORD i = 0; i < processCount; i++) {
                if (processIds[i] != 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[i]);
                    if (hProcess) {
                        char processName[MAX_PATH] = {0};
                        if (GetModuleBaseName(hProcess, nullptr, processName, sizeof(processName))) {
                            std::string name = processName;
                            if (showSystemProcesses || isUserApplication(name)) {
                                processes.push_back(ProcessInfo(processIds[i], name));
                            }
                        }
                        CloseHandle(hProcess);
                    }
                }
            }
        }
    }
    
    void refreshProcessList() {
        SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
        
        // Get search text
        char searchText[256];
        GetWindowText(hSearchEdit, searchText, sizeof(searchText));
        std::string search = searchText;
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        
        for (const auto& process : processes) {
            std::string display = process.name + " (PID: " + std::to_string(process.pid) + ")";
            
            // If search is empty or process name contains search text, add it
            if (search.empty()) {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
            } else {
                std::string processName = process.name;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
                if (processName.find(search) != std::string::npos) {
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                }
            }
        }
    }
    
    void addMemoryAddress(const std::string& address, const std::string& name) {
        uintptr_t addr = std::stoull(address, nullptr, 16);
        memoryAddresses.push_back({name, addr});
        
        std::string display = address + " - " + name;
        SendMessage(hMemList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
    }
    
    void run() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    void onButtonClick(int buttonId) {
        switch (buttonId) {
            case 1: // Add Memory Address
                addMemoryAddressFromInput();
                break;
            case 2: // Start/Stop Monitoring
                toggleMonitoring();
                break;
            case 3: // Export Data
                exportData();
                break;
            case 4: // Refresh Processes
                refreshProcesses();
                refreshProcessList();
                SetWindowText(hStatusLabel, "Process list refreshed");
                break;
            case 5: // Toggle System Processes
                showSystemProcesses = !showSystemProcesses;
                refreshProcesses();
                refreshProcessList();
                char status[100];
                sprintf(status, "System processes %s", showSystemProcesses ? "shown" : "hidden");
                SetWindowText(hStatusLabel, status);
                break;
            case 6: // Search text changed
                refreshProcessList();
                break;
        }
    }
    
    void onProcessSelection() {
        int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)processes.size()) {
            selectedProcess = &processes[sel];
            char status[200];
            sprintf(status, "Selected: %s (PID: %d)", selectedProcess->name.c_str(), selectedProcess->pid);
            SetWindowText(hStatusLabel, status);
        }
    }
    
    void addMemoryAddressFromInput() {
        char address[256], name[256];
        GetWindowText(hAddressEdit, address, sizeof(address));
        GetWindowText(hNameEdit, name, sizeof(name));
        
        if (strlen(address) > 2 && strlen(name) > 0) {
            addMemoryAddress(address, name);
            SetWindowText(hAddressEdit, "0x");
            SetWindowText(hNameEdit, "");
            SetWindowText(hStatusLabel, "Memory address added successfully");
        } else {
            SetWindowText(hStatusLabel, "Please enter both address and name");
        }
    }
    
    void toggleMonitoring() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (monitoring) {
            monitoring = false;
            SetWindowText(hStartButton, "Start Monitoring");
            SetWindowText(hStatusLabel, "Monitoring stopped.");
        } else {
            monitoring = true;
            SetWindowText(hStartButton, "Stop Monitoring");
            SetWindowText(hStatusLabel, "Monitoring started...");
            
            // Start monitoring thread
            std::thread([this]() {
                int sample = 0;
                while (monitoring && selectedProcess) {
                    char status[200];
                    sprintf(status, "Monitoring %s... Sample %d", selectedProcess->name.c_str(), sample);
                    SetWindowText(hStatusLabel, status);
                    
                    // Try to read memory from selected process
                    for (const auto& memAddr : memoryAddresses) {
                        int32_t value;
                        if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                            // Successfully read memory
                            char memStatus[300];
                            sprintf(memStatus, "%s: %s = %d", status, memAddr.first.c_str(), value);
                            SetWindowText(hStatusLabel, memStatus);
                        }
                    }
                    
                    sample++;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }).detach();
        }
    }
    
    void exportData() {
        SetWindowText(hStatusLabel, "Exporting data to game_analysis.csv...");
        
        FILE* file = fopen("game_analysis.csv", "w");
        if (file) {
            fprintf(file, "Timestamp,Process,Address,Name,Value\n");
            
            time_t now = time(0);
            char timestamp[100];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            if (selectedProcess) {
                for (const auto& memAddr : memoryAddresses) {
                    int32_t value = 0;
                    if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                        fprintf(file, "%s,%s,0x%llX,%s,%d\n", 
                               timestamp, selectedProcess->name.c_str(), 
                               (unsigned long long)memAddr.second, memAddr.first.c_str(), value);
                    }
                }
            }
            
            fclose(file);
            
            char status[200];
            char cwd[256];
            getcwd(cwd, sizeof(cwd));
            sprintf(status, "Data exported to: %s/game_analysis.csv", cwd);
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "Error: Could not create export file!");
        }
    }
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        RealGameAnalyzerGUI* pThis = nullptr;
        
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (RealGameAnalyzerGUI*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (RealGameAnalyzerGUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        
        if (pThis) {
            switch (uMsg) {
                case WM_COMMAND:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        pThis->onButtonClick(LOWORD(wParam));
                    } else if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 0) {
                        pThis->onProcessSelection();
                    } else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 6) {
                        // Search text changed
                        pThis->refreshProcessList();
                    }
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
            }
        }
        
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    RealGameAnalyzerGUI app;
    
    if (!app.createWindow()) {
        MessageBox(nullptr, "Failed to create window!", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    app.run();
    return 0;
}
