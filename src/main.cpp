#include <windows.h>
#include <commctrl.h>
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
#include <iomanip>
#include <sstream>
#include <map>

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

// Memory region information
struct MemoryRegion {
    uintptr_t baseAddress;
    size_t size;
    DWORD protection;
    DWORD state;
    DWORD type;
    std::string regionType;
    
    MemoryRegion(uintptr_t addr, size_t sz, DWORD prot, DWORD st, DWORD ty) 
        : baseAddress(addr), size(sz), protection(prot), state(st), type(ty) {
        // Determine region type based on protection flags
        if (protection & PAGE_EXECUTE_READWRITE) regionType = "Executable Read/Write";
        else if (protection & PAGE_EXECUTE_READ) regionType = "Executable Read";
        else if (protection & PAGE_READWRITE) regionType = "Read/Write";
        else if (protection & PAGE_READONLY) regionType = "Read Only";
        else if (protection & PAGE_NOACCESS) regionType = "No Access";
        else regionType = "Unknown";
    }
};

// Memory scanning functionality
class MemoryScanner {
public:
    static std::vector<MemoryRegion> scanProcessMemory(DWORD pid) {
        std::vector<MemoryRegion> regions;
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return regions;
        }
        
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t address = 0;
        
        while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi))) {
            // Only include committed memory regions that are readable
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect & PAGE_READONLY || 
                 mbi.Protect & PAGE_READWRITE || 
                 mbi.Protect & PAGE_EXECUTE_READ || 
                 mbi.Protect & PAGE_EXECUTE_READWRITE)) {
                
                regions.emplace_back((uintptr_t)mbi.BaseAddress, mbi.RegionSize, 
                                   mbi.Protect, mbi.State, mbi.Type);
            }
            
            address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            
            // Prevent infinite loop
            if (address == 0) break;
        }
        
        CloseHandle(hProcess);
        return regions;
    }
    
    static std::string addressToString(uintptr_t address) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << address;
        return ss.str();
    }
    
    static std::vector<uintptr_t> findReadableAddresses(DWORD pid, const std::vector<MemoryRegion>& regions, int maxAddresses = 100) {
        std::vector<uintptr_t> addresses;
        
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return addresses;
        }
        
        for (const auto& region : regions) {
            if (addresses.size() >= maxAddresses) break;
            
            // Focus on larger regions that are more likely to contain game data
            if (region.size < 4096) continue; // Skip small regions
            
            // Sample addresses within the region more densely
            size_t step = std::max(region.size / 100, (size_t)4); // Sample every 4 bytes or region size / 100
            for (uintptr_t addr = region.baseAddress; 
                 addr < region.baseAddress + region.size && addresses.size() < maxAddresses; 
                 addr += step) {
                
                // Try to read a 4-byte value at this address
                int32_t testValue;
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, (LPCVOID)addr, &testValue, sizeof(testValue), &bytesRead) &&
                    bytesRead == sizeof(testValue)) {
                    
                    // Prefer addresses with non-zero values that look like game data
                    if (testValue != 0 && testValue != -1 && 
                        testValue >= -1000000 && testValue <= 1000000) {
                        addresses.push_back(addr);
                    }
                }
            }
        }
        
        CloseHandle(hProcess);
        return addresses;
    }
    
    static std::vector<uintptr_t> findGameDataAddresses(DWORD pid, const std::vector<MemoryRegion>& regions, int maxAddresses = 200) {
        std::vector<uintptr_t> addresses;
        
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return addresses;
        }
        
        // Focus on regions that are more likely to contain game data
        for (const auto& region : regions) {
            if (addresses.size() >= maxAddresses) break;
            
            // Skip very small regions and system regions
            if (region.size < 8192) continue;
            
            // Prefer regions with read/write access (more likely to be game data)
            if (!(region.protection & PAGE_READWRITE)) continue;
            
            // Sample more densely in promising regions
            size_t step = std::max(region.size / 200, (size_t)4);
            for (uintptr_t addr = region.baseAddress; 
                 addr < region.baseAddress + region.size && addresses.size() < maxAddresses; 
                 addr += step) {
                
                // Try to read a 4-byte value at this address
                int32_t testValue;
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, (LPCVOID)addr, &testValue, sizeof(testValue), &bytesRead) &&
                    bytesRead == sizeof(testValue)) {
                    
                    // Look for values that could be game data
                    if (testValue != 0 && testValue != -1 && 
                        testValue >= -10000 && testValue <= 10000 &&
                        (testValue % 4 == 0 || testValue % 8 == 0)) { // Often aligned values
                        addresses.push_back(addr);
                    }
                }
            }
        }
        
        CloseHandle(hProcess);
        return addresses;
    }
    
    static std::string interpretValue(int32_t value) {
        std::stringstream ss;
        
        // Check if it could be a reasonable game value
        if (value >= 0 && value <= 1000) {
            ss << " [Possible: Health/Score/Count]";
        } else if (value >= -1000 && value <= 1000) {
            ss << " [Possible: Stat/Coordinate]";
        }
        
        // Check if it's a valid ASCII character sequence
        if (value >= 32 && value <= 126) {
            char c = (char)value;
            ss << " [ASCII: '" << c << "']";
        }
        
        // Check if it could be a floating point (very rough check)
        float floatVal = *(float*)&value;
        if (floatVal >= -1000.0f && floatVal <= 1000.0f && floatVal == floatVal) { // NaN check: NaN != NaN
            ss << " [Float: " << std::fixed << std::setprecision(2) << floatVal << "]";
        }
        
        return ss.str();
    }
};

// Enhanced GUI with real process functionality
class RealGameAnalyzerGUI {
private:
    HWND hwnd;
    HWND hListBox;
    HWND hStartButton;
    HWND hExportButton;
    HWND hRefreshButton;
    HWND hStatusLabel;
    HWND hMemList;
    HWND hSearchEdit;
    HWND hScanButton;
    HWND hAddressListBox;
    HWND hAddSelectedButton;
    HWND hProgressBar;
    HWND hCompareButton;
    HWND hValueChangeButton;
    HWND hLoadGameProfileButton;
    HWND hSaveGameProfileButton;
    HWND hScanGameDataButton;
    HWND hAddressSearchEdit;
    
    std::vector<ProcessInfo> processes;
    std::vector<ProcessInfo*> filteredProcesses;
    std::vector<std::pair<std::string, uintptr_t>> memoryAddresses;
    std::vector<MemoryRegion> memoryRegions;
    std::vector<uintptr_t> discoveredAddresses;
    std::map<uintptr_t, int32_t> previousValues;
    std::vector<bool> addressChecked;
    std::atomic<bool> monitoring;
    std::atomic<bool> scanning;
    ProcessInfo* selectedProcess;
    bool showSystemProcesses;
    
public:
    RealGameAnalyzerGUI() : hwnd(nullptr), monitoring(false), scanning(false), selectedProcess(nullptr), showSystemProcesses(false) {
        refreshProcesses();
    }
    
    bool createWindow() {
        // Initialize common controls for progress bar
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);
        
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "RealGameAnalyzerWindow";
        
        if (!RegisterClassEx(&wc)) {
            // Check if class is already registered (this is normal for subsequent runs)
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
                return false;
            }
        }
        
        hwnd = CreateWindowEx(
            0,
            "RealGameAnalyzerWindow",
            "Game Analyzer - Real Process Monitor",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1400, 900,
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
        
        hListBox = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            20, 50, 350, 200, hwnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);
        
        hRefreshButton = CreateWindow("BUTTON", "Refresh Processes", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            390, 50, 140, 25, hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("BUTTON", "Show System Processes", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            390, 80, 160, 25, hwnd, (HMENU)5, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("BUTTON", "Select Highlighted Process", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            390, 110, 160, 25, hwnd, (HMENU)9, GetModuleHandle(nullptr), nullptr);
        
        // Search functionality
        CreateWindow("STATIC", "Search:", WS_VISIBLE | WS_CHILD,
            20, 260, 50, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hSearchEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            70, 260, 250, 25, hwnd, (HMENU)6, GetModuleHandle(nullptr), nullptr);
        
        // Populate process list
        refreshProcessList();
        
        // Main Control Buttons
        hStartButton = CreateWindow("BUTTON", "Start Monitoring", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
             20, 295, 130, 30, hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
        
        hExportButton = CreateWindow("BUTTON", "Export Data", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
             160, 295, 120, 30, hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
        
        // Status Label
        hStatusLabel = CreateWindow("STATIC", "Ready - Select a process to begin", WS_VISIBLE | WS_CHILD,
             20, 335, 600, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Memory Scanning Section
        CreateWindow("STATIC", "Memory Scanning:", WS_VISIBLE | WS_CHILD,
            20, 365, 150, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hScanButton = CreateWindow("BUTTON", "Scan Process Memory", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            20, 390, 160, 30, hwnd, (HMENU)7, GetModuleHandle(nullptr), nullptr);
        
        hScanGameDataButton = CreateWindow("BUTTON", "Scan Game Data", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            190, 390, 160, 30, hwnd, (HMENU)14, GetModuleHandle(nullptr), nullptr);
        
        hProgressBar = CreateWindow(PROGRESS_CLASS, "", WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
            20, 430, 400, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("STATIC", "Discovered Addresses:", WS_VISIBLE | WS_CHILD,
            20, 460, 150, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("STATIC", "Search:", WS_VISIBLE | WS_CHILD,
            20, 485, 50, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hAddressSearchEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            70, 485, 200, 25, hwnd, (HMENU)15, GetModuleHandle(nullptr), nullptr);
        
        hAddressListBox = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_EXTENDEDSEL,
            20, 515, 450, 150, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Button layout in two columns
        hAddSelectedButton = CreateWindow("BUTTON", "Add Selected to Monitor", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            480, 515, 180, 30, hwnd, (HMENU)8, GetModuleHandle(nullptr), nullptr);
        
        hCompareButton = CreateWindow("BUTTON", "Compare Values", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            480, 550, 180, 30, hwnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);
        
        hValueChangeButton = CreateWindow("BUTTON", "Add Changed to Monitor", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
             480, 585, 180, 30, hwnd, (HMENU)11, GetModuleHandle(nullptr), nullptr);
        
        hLoadGameProfileButton = CreateWindow("BUTTON", "Load Game Profile", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            480, 620, 180, 30, hwnd, (HMENU)12, GetModuleHandle(nullptr), nullptr);
        
        hSaveGameProfileButton = CreateWindow("BUTTON", "Save Game Profile", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            480, 655, 180, 30, hwnd, (HMENU)13, GetModuleHandle(nullptr), nullptr);
        
        // Memory Addresses List
        CreateWindow("STATIC", "Monitored Addresses:", WS_VISIBLE | WS_CHILD,
            20, 675, 150, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hMemList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            20, 700, 640, 120, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // No sample addresses - start with empty list
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
        
        // Store filtered processes for selection matching
        filteredProcesses.clear();
        
        for (auto& process : processes) {
            std::string display = process.name + " (PID: " + std::to_string(process.pid) + ")";
            
            // If search is empty or process name contains search text, add it
            if (search.empty()) {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                filteredProcesses.push_back(&process);
            } else {
                std::string processName = process.name;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
                if (processName.find(search) != std::string::npos) {
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                    filteredProcesses.push_back(&process);
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
            case 7: // Scan Process Memory
                scanProcessMemory();
                break;
            case 8: // Add Selected Addresses
                addSelectedAddresses();
                break;
            case 9: // Select Highlighted Process
                onProcessSelection();
                break;
            case 10: // Compare Values
                compareValues();
                break;
            case 11: // Add Changed to Monitor
                showChangedValues();
                break;
            case 12: // Load Game Profile
                loadGameProfile();
                break;
            case 13: // Save Game Profile
                saveGameProfile();
                break;
            case 14: // Scan Game Data
                scanGameData();
                break;
        }
    }
    
    void onProcessSelection() {
        int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
        char debugMsg[200];
        sprintf(debugMsg, "Selection event: sel=%d, filteredProcesses.size()=%zu", sel, filteredProcesses.size());
        SetWindowText(hStatusLabel, debugMsg);
        
        if (sel != LB_ERR && sel < (int)filteredProcesses.size()) {
            selectedProcess = filteredProcesses[sel];
            char status[200];
            sprintf(status, "Selected: %s (PID: %d)", selectedProcess->name.c_str(), selectedProcess->pid);
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "No valid process selected");
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
    
    void scanProcessMemory() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (scanning) {
            SetWindowText(hStatusLabel, "Memory scan already in progress...");
            return;
        }
        
        scanning = true;
        SetWindowText(hScanButton, "Scanning...");
        EnableWindow(hScanButton, FALSE);
        SetWindowText(hStatusLabel, "Scanning process memory...");
        
        // Clear previous results
        SendMessage(hAddressListBox, LB_RESETCONTENT, 0, 0);
        discoveredAddresses.clear();
        addressChecked.clear();
        
        // Start scanning in a separate thread
        std::thread([this]() {
            try {
                // Step 1: Get memory regions
                SetWindowText(hStatusLabel, "Discovering memory regions...");
                memoryRegions = MemoryScanner::scanProcessMemory(selectedProcess->pid);
                
                if (memoryRegions.empty()) {
                    SetWindowText(hStatusLabel, "No readable memory regions found");
                    goto cleanup;
                }
                
                char status[200];
                sprintf(status, "Found %zu memory regions, scanning for readable addresses...", memoryRegions.size());
                SetWindowText(hStatusLabel, status);
                
                // Step 2: Find readable addresses
                discoveredAddresses = MemoryScanner::findReadableAddresses(selectedProcess->pid, memoryRegions, 200);
                
                // Initialize checkbox state
                addressChecked.resize(discoveredAddresses.size(), false);
                
                // Step 3: Populate the address list using filter method
                filterAddressList();
                
                sprintf(status, "Scan complete! Found %zu readable addresses", discoveredAddresses.size());
                SetWindowText(hStatusLabel, status);
                
            } catch (const std::exception& e) {
                SetWindowText(hStatusLabel, "Error during memory scan");
            }
            
        cleanup:
            scanning = false;
            SetWindowText(hScanButton, "Scan Process Memory");
            EnableWindow(hScanButton, TRUE);
        }).detach();
    }
    
    void filterAddressList() {
        // Get search text
        char searchText[256];
        GetWindowText(hAddressSearchEdit, searchText, sizeof(searchText));
        std::string search = searchText;
        
        // Clear and repopulate the list with filtered results
        SendMessage(hAddressListBox, LB_RESETCONTENT, 0, 0);
        
        for (size_t i = 0; i < discoveredAddresses.size(); ++i) {
            uintptr_t addr = discoveredAddresses[i];
            std::string addrStr = MemoryScanner::addressToString(addr);
            
            // Try to read the value at this address
            int32_t value = 0;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &value, sizeof(value))) {
                std::string interpretation = MemoryScanner::interpretValue(value);
                std::string display = addrStr + " (Value: " + std::to_string(value) + ")" + interpretation;
                
                // Add checkbox to display
                std::string checkboxDisplay = (addressChecked.size() > i && addressChecked[i]) ? "[✓] " : "[ ] ";
                checkboxDisplay += display;
                
                // Filter based on search text (case-insensitive)
                if (search.empty()) {
                    SendMessage(hAddressListBox, LB_ADDSTRING, 0, (LPARAM)checkboxDisplay.c_str());
                } else {
                    // Convert both strings to lowercase for case-insensitive search
                    // Search in the full checkboxDisplay text (includes checkbox and *** markers)
                    std::string checkboxDisplayLower = checkboxDisplay;
                    std::string searchLower = search;
                    std::transform(checkboxDisplayLower.begin(), checkboxDisplayLower.end(), checkboxDisplayLower.begin(), ::tolower);
                    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
                    
                    if (checkboxDisplayLower.find(searchLower) != std::string::npos) {
                        SendMessage(hAddressListBox, LB_ADDSTRING, 0, (LPARAM)checkboxDisplay.c_str());
                    }
                }
            }
        }
    }
    
    void toggleAddressCheckbox() {
        int sel = SendMessage(hAddressListBox, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)discoveredAddresses.size()) {
            // Toggle the checkbox state
            if (sel < (int)addressChecked.size()) {
                addressChecked[sel] = !addressChecked[sel];
                // Refresh the display
                filterAddressList();
            }
        }
    }
    
    void addSelectedAddresses() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        int addedCount = 0;
        for (size_t i = 0; i < discoveredAddresses.size(); ++i) {
            if (i < addressChecked.size() && addressChecked[i]) {
                uintptr_t addr = discoveredAddresses[i];
                std::string addrStr = MemoryScanner::addressToString(addr);
                std::string name = "Discovered_" + std::to_string(addedCount + 1);
                
                // Check if address already exists
                bool exists = false;
                for (const auto& memAddr : memoryAddresses) {
                    if (memAddr.second == addr) {
                        exists = true;
                        break;
                    }
                }
                
                if (!exists) {
                    addMemoryAddress(addrStr, name);
                    addedCount++;
                }
            }
        }
        
        if (addedCount == 0) {
            SetWindowText(hStatusLabel, "No checked addresses to add. Click on [ ] to check addresses.");
        } else {
            char status[100];
            sprintf(status, "Added %d new addresses to monitoring list", addedCount);
            SetWindowText(hStatusLabel, status);
        }
    }
    
    void compareValues() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (discoveredAddresses.empty()) {
            SetWindowText(hStatusLabel, "Please scan memory first");
            return;
        }
        
        SetWindowText(hStatusLabel, "Reading current values for comparison...");
        
        // Read current values and store them for comparison
        for (uintptr_t addr : discoveredAddresses) {
            int32_t value = 0;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &value, sizeof(value))) {
                previousValues[addr] = value;
            }
        }
        
        char status[200];
        sprintf(status, "Stored %zu values for comparison. Now do something in-game and click 'Show Changed Values'", previousValues.size());
        SetWindowText(hStatusLabel, status);
    }
    
    void showChangedValues() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (previousValues.empty()) {
            SetWindowText(hStatusLabel, "Please click 'Compare Values' first");
            return;
        }
        
        SetWindowText(hStatusLabel, "Checking for changed values and adding to monitoring...");
        
        int changedCount = 0;
        for (uintptr_t addr : discoveredAddresses) {
            int32_t currentValue = 0;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &currentValue, sizeof(currentValue))) {
                auto it = previousValues.find(addr);
                if (it != previousValues.end()) {
                    int32_t previousValue = it->second;
                    
                    if (currentValue != previousValue) {
                        // Address value changed - add it to monitoring
                        std::string addrStr = MemoryScanner::addressToString(addr);
                        std::string name = "Changed_" + std::to_string(changedCount + 1);
                        
                        // Check if address already exists in monitoring
                        bool exists = false;
                        for (const auto& memAddr : memoryAddresses) {
                            if (memAddr.second == addr) {
                                exists = true;
                                break;
                            }
                        }
                        
                        if (!exists) {
                            addMemoryAddress(addrStr, name);
                            changedCount++;
                        }
                    }
                }
            }
        }
        
        char status[200];
        if (changedCount > 0) {
            sprintf(status, "Found %d changed values! Added them to monitoring list.", changedCount);
        } else {
            sprintf(status, "No changed values found. Try doing something different in-game.");
        }
        SetWindowText(hStatusLabel, status);
    }
    
    void saveGameProfile() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (memoryAddresses.empty()) {
            SetWindowText(hStatusLabel, "No addresses to save");
            return;
        }
        
        // Create filename based on process name
        std::string filename = selectedProcess->name + "_profile.txt";
        
        FILE* file = fopen(filename.c_str(), "w");
        if (file) {
            fprintf(file, "# Game Profile for %s\n", selectedProcess->name.c_str());
            fprintf(file, "# Generated by Game Analyzer\n\n");
            
            for (const auto& memAddr : memoryAddresses) {
                fprintf(file, "0x%llX %s\n", (unsigned long long)memAddr.second, memAddr.first.c_str());
            }
            
            fclose(file);
            
            char status[200];
            sprintf(status, "Saved %zu addresses to %s", memoryAddresses.size(), filename.c_str());
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "Error: Could not save profile file!");
        }
    }
    
    void loadGameProfile() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        // Create filename based on process name
        std::string filename = selectedProcess->name + "_profile.txt";
        
        FILE* file = fopen(filename.c_str(), "r");
        if (file) {
            char line[256];
            int loadedCount = 0;
            
            while (fgets(line, sizeof(line), file)) {
                // Skip comments and empty lines
                if (line[0] == '#' || line[0] == '\n') continue;
                
                // Parse address and name
                uintptr_t address;
                char name[100];
                if (sscanf(line, "0x%llX %99s", (unsigned long long*)&address, name) == 2) {
                    // Check if address already exists
                    bool exists = false;
                    for (const auto& memAddr : memoryAddresses) {
                        if (memAddr.second == address) {
                            exists = true;
                            break;
                        }
                    }
                    
                    if (!exists) {
                        std::string addrStr = MemoryScanner::addressToString(address);
                        addMemoryAddress(addrStr, name);
                        loadedCount++;
                    }
                }
            }
            
            fclose(file);
            
            char status[200];
            sprintf(status, "Loaded %d addresses from %s", loadedCount, filename.c_str());
            SetWindowText(hStatusLabel, status);
        } else {
            char status[200];
            sprintf(status, "No profile found for %s. Save one first!", selectedProcess->name.c_str());
            SetWindowText(hStatusLabel, status);
        }
    }
    
    void scanGameData() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (scanning) {
            SetWindowText(hStatusLabel, "Memory scan already in progress...");
            return;
        }
        
        scanning = true;
        SetWindowText(hScanGameDataButton, "Scanning...");
        EnableWindow(hScanGameDataButton, FALSE);
        SetWindowText(hStatusLabel, "Scanning for game data (focused scan)...");
        
        // Clear previous results
        SendMessage(hAddressListBox, LB_RESETCONTENT, 0, 0);
        discoveredAddresses.clear();
        addressChecked.clear();
        
        // Start scanning in a separate thread
        std::thread([this]() {
            try {
                // Step 1: Get memory regions
                SetWindowText(hStatusLabel, "Discovering memory regions...");
                memoryRegions = MemoryScanner::scanProcessMemory(selectedProcess->pid);
                
                if (memoryRegions.empty()) {
                    SetWindowText(hStatusLabel, "No readable memory regions found");
                    goto cleanup;
                }
                
                char status[200];
                sprintf(status, "Found %zu memory regions, scanning for game data...", memoryRegions.size());
                SetWindowText(hStatusLabel, status);
                
                // Step 2: Find game data addresses (more focused scan)
                discoveredAddresses = MemoryScanner::findGameDataAddresses(selectedProcess->pid, memoryRegions, 200);
                
                // Initialize checkbox state
                addressChecked.resize(discoveredAddresses.size(), false);
                
                // Step 3: Populate the address list using filter method
                filterAddressList();
                
                sprintf(status, "Game data scan complete! Found %zu addresses. Try Compare Values now!", discoveredAddresses.size());
                SetWindowText(hStatusLabel, status);
                
            } catch (const std::exception& e) {
                SetWindowText(hStatusLabel, "Error during game data scan");
            }
            
        cleanup:
            scanning = false;
            SetWindowText(hScanGameDataButton, "Scan Game Data");
            EnableWindow(hScanGameDataButton, TRUE);
        }).detach();
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
                    } else if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 10) {
                        pThis->onProcessSelection();
                    } else if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == 10) {
                        // Double-click on process list
                        pThis->onProcessSelection();
                    } else if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 0) {
                        // Click on address list - toggle checkbox
                        pThis->toggleAddressCheckbox();
                    } else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 6) {
                        // Search text changed
                        pThis->refreshProcessList();
                    } else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 15) {
                        // Address search text changed
                        pThis->filterAddressList();
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
    // Create a mutex to prevent multiple instances
    HANDLE hMutex = CreateMutex(nullptr, TRUE, "GameAnalyzerSingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(nullptr, "Game Analyzer is already running!", "Error", MB_OK | MB_ICONWARNING);
        return 1;
    }
    
    RealGameAnalyzerGUI app;
    
    if (!app.createWindow()) {
        MessageBox(nullptr, "Failed to create window!", "Error", MB_OK | MB_ICONERROR);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }
    
    app.run();
    
    // Cleanup
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
