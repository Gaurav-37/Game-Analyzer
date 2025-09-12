#include "GameAnalyzer.h"
#include "MemoryReader.h"
#include "ProcessManager.h"
#include "DataExporter.h"
#include "ChartRenderer.h"
#include "ConfigManager.h"

GameAnalyzer::GameAnalyzer() 
    : selectedProcess(nullptr)
    , isMonitoring(false)
    , showProcessSelector(false)
    , showAddAddressDialog(false)
    , showSettingsDialog(false)
    , updateInterval(100)
    , maxDataPoints(1000)
    , autoSave(false)
    , dataDirectory("data")
    , selectedAddressType(0)
    , selectedProcessIndex(-1)
{
    strcpy_s(newAddressName, "");
    strcpy_s(newAddressValue, "");
}

GameAnalyzer::~GameAnalyzer() {
    shutdown();
}

bool GameAnalyzer::initialize() {
    try {
        memoryReader = std::make_unique<MemoryReader>();
        processManager = std::make_unique<ProcessManager>();
        dataExporter = std::make_unique<DataExporter>();
        chartRenderer = std::make_unique<ChartRenderer>();
        configManager = std::make_unique<ConfigManager>();
        
        if (!memoryReader->initialize()) {
            updateStatus("Failed to initialize memory reader");
            return false;
        }
        
        if (!processManager->initialize()) {
            updateStatus("Failed to initialize process manager");
            return false;
        }
        
        if (!chartRenderer->initialize()) {
            updateStatus("Failed to initialize chart renderer");
            return false;
        }
        
        if (!configManager->initialize()) {
            updateStatus("Failed to initialize config manager");
            return false;
        }
        
        loadConfiguration();
        refreshProcessList();
        
        updateStatus("Game Analyzer initialized successfully");
        return true;
        
    } catch (const std::exception& e) {
        updateStatus("Initialization error: " + std::string(e.what()));
        return false;
    }
}

void GameAnalyzer::shutdown() {
    if (isMonitoring) {
        stopMonitoring();
    }
    
    saveConfiguration();
    
    memoryReader.reset();
    processManager.reset();
    dataExporter.reset();
    chartRenderer.reset();
    configManager.reset();
}

void GameAnalyzer::update() {
    if (!isMonitoring || !selectedProcess) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(dataMutex);
    
    for (auto& address : memoryAddresses) {
        try {
            double value = 0.0;
            bool success = false;
            
            if (address.type == "int32") {
                int32_t intValue;
                success = memoryReader->readMemory(selectedProcess->pid, address.address, &intValue, sizeof(int32_t));
                value = static_cast<double>(intValue);
            }
            else if (address.type == "float") {
                float floatValue;
                success = memoryReader->readMemory(selectedProcess->pid, address.address, &floatValue, sizeof(float));
                value = static_cast<double>(floatValue);
            }
            else if (address.type == "double") {
                success = memoryReader->readMemory(selectedProcess->pid, address.address, &value, sizeof(double));
            }
            
            if (success) {
                address.change = value - address.lastValue;
                address.lastValue = value;
                address.lastUpdate = std::chrono::high_resolution_clock::now();
                
                address.values.push_back(value);
                
                // Limit data points
                if (address.values.size() > maxDataPoints) {
                    address.values.erase(address.values.begin());
                }
            }
            
        } catch (const std::exception& e) {
            // Handle read error silently
        }
    }
}

void GameAnalyzer::render() {
    renderMainMenu();
    
    if (showProcessSelector) {
        renderProcessSelector();
    }
    
    if (showAddAddressDialog) {
        renderAddAddressDialog();
    }
    
    if (showSettingsDialog) {
        renderSettingsDialog();
    }
    
    // Main content area
    ImGui::Begin("Game Analyzer", nullptr, ImGuiWindowFlags_NoCollapse);
    
    // Control buttons
    if (ImGui::Button("Select Process")) {
        showProcessSelector = true;
    }
    
    ImGui::SameLine();
    
    if (!isMonitoring) {
        if (ImGui::Button("Start Monitoring") && selectedProcess) {
            startMonitoring();
        }
    } else {
        if (ImGui::Button("Stop Monitoring")) {
            stopMonitoring();
        }
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Add Address")) {
        showAddAddressDialog = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Settings")) {
        showSettingsDialog = true;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Export Data")) {
        dataExporter->exportToCSV(memoryAddresses, "game_analysis.csv");
        updateStatus("Data exported to CSV");
    }
    
    ImGui::Separator();
    
    // Process info
    if (selectedProcess) {
        ImGui::Text("Selected Process: %s (PID: %d)", selectedProcess->name.c_str(), selectedProcess->pid);
    } else {
        ImGui::Text("No process selected");
    }
    
    ImGui::Separator();
    
    // Split into two columns
    ImGui::Columns(2, "MainColumns", true);
    
    // Left column - Current values
    ImGui::BeginChild("CurrentValues");
    renderCurrentValues();
    ImGui::EndChild();
    
    ImGui::NextColumn();
    
    // Right column - Chart
    ImGui::BeginChild("Chart");
    renderChart();
    ImGui::EndChild();
    
    ImGui::Columns(1);
    
    ImGui::End();
    
    renderStatusBar();
}

void GameAnalyzer::renderMainMenu() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Configuration")) {
                saveConfiguration();
                updateStatus("Configuration saved");
            }
            if (ImGui::MenuItem("Load Configuration")) {
                loadConfiguration();
                updateStatus("Configuration loaded");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                // Handle exit
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Tools")) {
            if (ImGui::MenuItem("Refresh Process List")) {
                refreshProcessList();
            }
            if (ImGui::MenuItem("Clear Data")) {
                std::lock_guard<std::mutex> lock(dataMutex);
                for (auto& address : memoryAddresses) {
                    address.values.clear();
                }
                updateStatus("Data cleared");
            }
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Show about dialog
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void GameAnalyzer::renderProcessSelector() {
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Select Game Process", &showProcessSelector);
    
    if (ImGui::Button("Refresh")) {
        refreshProcessList();
    }
    
    ImGui::Separator();
    
    if (ImGui::BeginListBox("Processes", ImVec2(-1, -1))) {
        for (int i = 0; i < processes.size(); i++) {
            const auto& process = processes[i];
            std::string label = process.name + " (PID: " + std::to_string(process.pid) + ")";
            
            if (ImGui::Selectable(label.c_str(), selectedProcessIndex == i)) {
                selectProcess(i);
            }
        }
        ImGui::EndListBox();
    }
    
    ImGui::End();
}

void GameAnalyzer::renderAddAddressDialog() {
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Add Memory Address", &showAddAddressDialog);
    
    ImGui::InputText("Name", newAddressName, sizeof(newAddressName));
    ImGui::InputText("Address (0x...)", newAddressValue, sizeof(newAddressValue));
    
    const char* types[] = { "int32", "float", "double" };
    ImGui::Combo("Type", &selectedAddressType, types, IM_ARRAYSIZE(types));
    
    ImGui::Separator();
    
    if (ImGui::Button("Add")) {
        addMemoryAddress();
        showAddAddressDialog = false;
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel")) {
        showAddAddressDialog = false;
    }
    
    ImGui::End();
}

void GameAnalyzer::renderSettingsDialog() {
    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Settings", &showSettingsDialog);
    
    ImGui::SliderInt("Update Interval (ms)", &updateInterval, 10, 1000);
    ImGui::SliderInt("Max Data Points", &maxDataPoints, 100, 10000);
    ImGui::Checkbox("Auto Save", &autoSave);
    
    ImGui::Separator();
    
    if (ImGui::Button("Apply")) {
        saveConfiguration();
        updateStatus("Settings applied");
    }
    
    ImGui::SameLine();
    
    if (ImGui::Button("Reset to Defaults")) {
        updateInterval = 100;
        maxDataPoints = 1000;
        autoSave = false;
    }
    
    ImGui::End();
}

void GameAnalyzer::renderCurrentValues() {
    ImGui::Text("Current Values");
    ImGui::Separator();
    
    std::lock_guard<std::mutex> lock(dataMutex);
    
    for (const auto& address : memoryAddresses) {
        ImGui::PushID(address.name.c_str());
        
        ImGui::Text("%s", address.name.c_str());
        ImGui::SameLine(150);
        
        ImVec4 color = getValueColor(address.lastValue, address.change);
        ImGui::TextColored(color, "%s", formatValue(address.lastValue, address.type).c_str());
        
        if (address.change != 0.0) {
            ImGui::SameLine();
            ImGui::Text("(%+.2f)", address.change);
        }
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 50);
        
        if (ImGui::Button("X")) {
            // Remove address (will be handled in next frame)
        }
        
        ImGui::PopID();
    }
    
    if (memoryAddresses.empty()) {
        ImGui::Text("No memory addresses configured");
        ImGui::Text("Click 'Add Address' to start monitoring");
    }
}

void GameAnalyzer::renderChart() {
    ImGui::Text("Value History");
    ImGui::Separator();
    
    if (ImPlot::BeginPlot("Memory Values", ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Time", "Value");
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, maxDataPoints, ImGuiCond_Always);
        
        std::lock_guard<std::mutex> lock(dataMutex);
        
        for (const auto& address : memoryAddresses) {
            if (!address.values.empty()) {
                ImPlot::PlotLine(address.name.c_str(), 
                               address.values.data(), 
                               static_cast<int>(address.values.size()));
            }
        }
        
        ImPlot::EndPlot();
    }
}

void GameAnalyzer::renderStatusBar() {
    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetIO().DisplaySize.y - 20));
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x, 20));
    ImGui::Begin("StatusBar", nullptr, 
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
    
    ImGui::Text("%s", statusMessage.c_str());
    
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    
    if (isMonitoring) {
        ImGui::Text("Monitoring: ON");
    } else {
        ImGui::Text("Monitoring: OFF");
    }
    
    ImGui::End();
}

void GameAnalyzer::startMonitoring() {
    if (!selectedProcess) {
        updateStatus("No process selected");
        return;
    }
    
    isMonitoring = true;
    updateStatus("Monitoring started for " + selectedProcess->name);
}

void GameAnalyzer::stopMonitoring() {
    isMonitoring = false;
    updateStatus("Monitoring stopped");
}

void GameAnalyzer::addMemoryAddress() {
    if (strlen(newAddressName) == 0 || strlen(newAddressValue) == 0) {
        updateStatus("Please enter both name and address");
        return;
    }
    
    uintptr_t address = 0;
    try {
        address = std::stoull(newAddressValue, nullptr, 16);
    } catch (const std::exception&) {
        updateStatus("Invalid address format");
        return;
    }
    
    const char* types[] = { "int32", "float", "double" };
    std::string type = types[selectedAddressType];
    
    std::lock_guard<std::mutex> lock(dataMutex);
    memoryAddresses.emplace_back(newAddressName, address, type);
    
    strcpy_s(newAddressName, "");
    strcpy_s(newAddressValue, "");
    
    updateStatus("Memory address added: " + std::string(newAddressName));
}

void GameAnalyzer::removeMemoryAddress(int index) {
    if (index >= 0 && index < memoryAddresses.size()) {
        std::lock_guard<std::mutex> lock(dataMutex);
        memoryAddresses.erase(memoryAddresses.begin() + index);
        updateStatus("Memory address removed");
    }
}

void GameAnalyzer::refreshProcessList() {
    processes = processManager->getProcesses();
    updateStatus("Process list refreshed");
}

void GameAnalyzer::selectProcess(int index) {
    if (index >= 0 && index < processes.size()) {
        selectedProcess = &processes[index];
        selectedProcessIndex = index;
        updateStatus("Selected process: " + selectedProcess->name);
    }
}

void GameAnalyzer::updateStatus(const std::string& message) {
    statusMessage = message;
    lastStatusUpdate = std::chrono::high_resolution_clock::now();
}

void GameAnalyzer::saveConfiguration() {
    // Implementation for saving configuration
}

void GameAnalyzer::loadConfiguration() {
    // Implementation for loading configuration
}

std::string GameAnalyzer::formatAddress(uintptr_t address) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << address;
    return oss.str();
}

std::string GameAnalyzer::formatValue(double value, const std::string& type) {
    std::ostringstream oss;
    
    if (type == "int32") {
        oss << static_cast<int32_t>(value);
    } else if (type == "float") {
        oss << std::fixed << std::setprecision(2) << static_cast<float>(value);
    } else if (type == "double") {
        oss << std::fixed << std::setprecision(4) << value;
    }
    
    return oss.str();
}

ImVec4 GameAnalyzer::getValueColor(double value, double change) {
    if (change > 0) {
        return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); // Green
    } else if (change < 0) {
        return ImVec4(1.0f, 0.0f, 0.0f, 1.0f); // Red
    } else {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // White
    }
}
