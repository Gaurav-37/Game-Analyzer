#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>

#include "external/imgui/imgui.h"
#include "external/implot/implot.h"

struct MemoryAddress {
    std::string name;
    uintptr_t address;
    std::string type; // "int32", "float", "double"
    std::vector<double> values;
    double lastValue;
    double change;
    std::chrono::high_resolution_clock::time_point lastUpdate;
    
    MemoryAddress(const std::string& n, uintptr_t addr, const std::string& t)
        : name(n), address(addr), type(t), lastValue(0.0), change(0.0) {}
};

struct ProcessInfo {
    DWORD pid;
    std::string name;
    std::string windowTitle;
    
    ProcessInfo(DWORD p, const std::string& n, const std::string& w = "")
        : pid(p), name(n), windowTitle(w) {}
};

class MemoryReader;
class ProcessManager;
class DataExporter;
class ChartRenderer;
class ConfigManager;

class GameAnalyzer {
private:
    std::unique_ptr<MemoryReader> memoryReader;
    std::unique_ptr<ProcessManager> processManager;
    std::unique_ptr<DataExporter> dataExporter;
    std::unique_ptr<ChartRenderer> chartRenderer;
    std::unique_ptr<ConfigManager> configManager;
    
    std::vector<MemoryAddress> memoryAddresses;
    std::vector<ProcessInfo> processes;
    ProcessInfo* selectedProcess;
    
    std::atomic<bool> isMonitoring;
    std::atomic<bool> showProcessSelector;
    std::atomic<bool> showAddAddressDialog;
    std::atomic<bool> showSettingsDialog;
    
    std::string statusMessage;
    std::chrono::high_resolution_clock::time_point lastStatusUpdate;
    
    // Settings
    int updateInterval; // milliseconds
    int maxDataPoints;
    bool autoSave;
    std::string dataDirectory;
    
    // UI state
    char newAddressName[256];
    char newAddressValue[64];
    int selectedAddressType;
    int selectedProcessIndex;
    
    std::mutex dataMutex;

public:
    GameAnalyzer();
    ~GameAnalyzer();
    
    bool initialize();
    void shutdown();
    void update();
    void render();
    
private:
    void renderMainMenu();
    void renderProcessSelector();
    void renderAddAddressDialog();
    void renderSettingsDialog();
    void renderCurrentValues();
    void renderChart();
    void renderStatusBar();
    
    void startMonitoring();
    void stopMonitoring();
    void addMemoryAddress();
    void removeMemoryAddress(int index);
    void refreshProcessList();
    void selectProcess(int index);
    
    void updateStatus(const std::string& message);
    void saveConfiguration();
    void loadConfiguration();
    
    // Helper functions
    std::string formatAddress(uintptr_t address);
    std::string formatValue(double value, const std::string& type);
    ImVec4 getValueColor(double value, double change);
};
