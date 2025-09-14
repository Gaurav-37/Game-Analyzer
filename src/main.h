#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include "ui_framework.h"

// Forward declarations
struct ProcessInfo {
    std::string name;
    DWORD pid;
    std::string windowTitle;
    bool isGame;
};

struct AnalyticsData {
    int totalScans;
    int successfulScans;
    double averageScanTime;
    std::string lastScanTime;
    int totalAnalysisRuns;
    double averageMemoryStability;
    double averageVisionAccuracy;
    std::vector<double> memoryValues;
    std::vector<double> visionConfidence;
    std::map<std::string, double> valueTrends;
    std::map<std::string, int> valueChangeCounts;
};

// Real Game Analyzer GUI Class
class RealGameAnalyzerGUI {
public:
    // Constructor and destructor
    RealGameAnalyzerGUI();
    ~RealGameAnalyzerGUI();
    
    // Main application methods
    bool createWindow();
    void run();
    
    // UI creation and management
    void createControls();
    void initializeModernUI();
    void applyModernFonts();
    void updateThemeBrushes();
    void cleanupModernUI();
    
    // Button drawing
    void drawModernButton(LPDRAWITEMSTRUCT lpDrawItem);
    
    // Dialog functions
    void showAboutDialog();
    void showSettingsDialog();
    void showHelpDialog();
    void showError(const std::string& title, const std::string& message);
    void showWarning(const std::string& title, const std::string& message);
    void showInfo(const std::string& title, const std::string& message);
    
    // Theme management
    void toggleDarkMode();
    void toggleSystemProcesses();
    
    // Process management
    void refreshProcessList();
    void selectProcess();
    void startMonitoring();
    void exportData();
    
    // Memory analysis
    void scanMemory();
    void scanGameData();
    void addSelectedToMonitor();
    void compareValues();
    void addChangedValues();
    void loadGameProfile();
    void saveGameProfile();
    void listProfiles();
    
    // Vision analysis
    void captureScreen();
    void startVisionAnalysis();
    
    // Hybrid analysis
    void runHybridAnalysis();
    void compareMemoryVision();
    
    // Analytics
    void startAnalytics();
    void showMetrics();
    void exportAnalytics();
    void openDashboard();
    void showRealTimeCharts();
    void showPerformanceMonitor();
    
    // Utility functions
    void setStatus(const char* format, ...);
    void setProgress(int percentage);
    void initializeTooltips();
    
    // Button click handler
    void onButtonClick(int buttonId);
    
    // Window procedure
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // Public members for UI framework access
    HWND hwnd;
    bool modernThemeEnabled;
    bool showSystemProcesses;
    
    // Font handles
    HFONT hModernFont;
    HFONT hBoldFont;
    HFONT hHeaderFont;
    
    // Brush handles
    HBRUSH hBackgroundBrush;
    HBRUSH hPanelBrush;
    HBRUSH hCardBrush;
    HBRUSH hButtonHoverBrush;
    HBRUSH hButtonPressedBrush;
    HBRUSH hBorderBrush;
    
    // Control handles
    HWND hRibbonPanel;
    HWND hDarkModeToggle;
    HWND hSystemProcessesToggle;
    HWND hRefreshRibbonButton;
    HWND hQuickSettingsButton;
    HWND hListBox;
    HWND hRefreshButton;
    HWND hSearchEdit;
    HWND hStartButton;
    HWND hExportButton;
    HWND hStatusLabel;
    HWND hScanButton;
    HWND hScanGameDataButton;
    HWND hProgressBar;
    HWND hAddressSearchEdit;
    HWND hAddressListBox;
    HWND hAddSelectedButton;
    HWND hCompareButton;
    HWND hValueChangeButton;
    HWND hLoadGameProfileButton;
    HWND hSaveGameProfileButton;
    HWND hListProfilesButton;
    HWND hCaptureScreenButton;
    HWND hStartVisionAnalysisButton;
    HWND hVisionStatusLabel;
    HWND hHybridAnalysisButton;
    HWND hCompareMemoryVisionButton;
    HWND hAnalyticsButton;
    HWND hShowMetricsButton;
    HWND hExportAnalyticsButton;
    HWND hDashboardButton;
    HWND hRealTimeChartsButton;
    HWND hPerformanceMonitorButton;
    HWND hAboutButton;
    HWND hSettingsButton;
    HWND hHelpButton;
    HWND hMemList;
    HWND hTooltipWindow;
    
    // Data members
    ProcessInfo* selectedProcess;
    std::vector<ProcessInfo> processes;
    std::vector<std::string> memoryAddresses;
    std::vector<std::string> discoveredAddresses;
    std::vector<std::string> memoryRegions;
    std::vector<std::string> detectedTexts;
    std::vector<BYTE> lastFrameData;
    int frameWidth;
    int frameHeight;
    bool monitoring;
    bool scanning;
    bool visionAnalyzing;
    AnalyticsData analytics;
};
