#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include "ui_framework.h"

// Forward declaration
class RealGameAnalyzerGUI;

// Modern Popup Dialog System
class PopupDialogs {
public:
    // Main dialog functions
    static void showAboutDialog(RealGameAnalyzerGUI* parent);
    static void showSettingsDialog(RealGameAnalyzerGUI* parent);
    static void showHelpDialog(RealGameAnalyzerGUI* parent);
    static void showErrorDialog(RealGameAnalyzerGUI* parent, const std::string& title, const std::string& message);
    static void showWarningDialog(RealGameAnalyzerGUI* parent, const std::string& title, const std::string& message);
    static void showInfoDialog(RealGameAnalyzerGUI* parent, const std::string& title, const std::string& message);
    
    // Analytics dialogs
    static void showDashboardDialog(RealGameAnalyzerGUI* parent);
    static void showChartsDialog(RealGameAnalyzerGUI* parent);
    static void showMonitorDialog(RealGameAnalyzerGUI* parent);
    static void showMetricsDialog(RealGameAnalyzerGUI* parent);
    
    // Analysis dialogs
    static void showVisionResultsDialog(RealGameAnalyzerGUI* parent, const std::vector<std::string>& detectedTexts);
    static void showHybridAnalysisDialog(RealGameAnalyzerGUI* parent, const std::string& results);
    static void showMemoryComparisonDialog(RealGameAnalyzerGUI* parent, const std::string& comparison);
    
    // Profile dialogs
    static void showProfilesListDialog(RealGameAnalyzerGUI* parent, const std::string& profilesList);
    
private:
    // Helper functions
    static std::string formatAboutContent();
    static std::string formatSettingsContent(RealGameAnalyzerGUI* parent);
    static std::string formatHelpContent();
    static std::string formatDashboardContent(RealGameAnalyzerGUI* parent);
    static std::string formatChartsContent(RealGameAnalyzerGUI* parent);
    static std::string formatMonitorContent(RealGameAnalyzerGUI* parent);
    static std::string formatMetricsContent(RealGameAnalyzerGUI* parent);
    static std::string formatVisionResultsContent(const std::vector<std::string>& detectedTexts);
    static std::string formatHybridAnalysisContent(const std::string& results);
    static std::string formatMemoryComparisonContent(const std::string& comparison);
    static std::string formatProfilesListContent(const std::string& profilesList);
    
    // Dialog styling
    static void applyDialogStyling(HWND dialog, RealGameAnalyzerGUI* parent);
    static void centerDialogOnScreen(HWND dialog, int width, int height);
};
