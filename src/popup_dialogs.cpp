#include "popup_dialogs.h"
#include "main.h" // For RealGameAnalyzerGUI class
#include <vector>
#include <sstream>

// Main dialog functions
void PopupDialogs::showAboutDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatAboutContent();
    ModernDialog* dialog = new ModernDialog(parent->hwnd, parent, "About Game Analyzer", content, 600, 500);
    // Dialog will use parent's theme state
    dialog->show();
}

void PopupDialogs::showSettingsDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatSettingsContent(parent);
    ModernDialog* dialog = new ModernDialog(parent->hwnd, parent, "Settings", content, 450, 300);
    // Dialog will use parent's theme state
    dialog->show();
}

void PopupDialogs::showHelpDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatHelpContent();
    ModernDialog* dialog = new ModernDialog(parent->hwnd, parent, "Help & Guide", content, 700, 600);
    // Dialog will use parent's theme state
    dialog->show();
}

void PopupDialogs::showErrorDialog(RealGameAnalyzerGUI* parent, const std::string& title, const std::string& message) {
    ModernDialog* dialog = new ModernDialog(parent->hwnd, parent, title, message, 400, 200);
    dialog->show();
    parent->setStatus("Error: %s", message.c_str());
}

void PopupDialogs::showWarningDialog(RealGameAnalyzerGUI* parent, const std::string& title, const std::string& message) {
    ModernDialog dialog(parent->hwnd, parent, title, message, 400, 200);
    dialog.show();
    parent->setStatus("Warning: %s", message.c_str());
}

void PopupDialogs::showInfoDialog(RealGameAnalyzerGUI* parent, const std::string& title, const std::string& message) {
    ModernDialog dialog(parent->hwnd, parent, title, message, 400, 200);
    dialog.show();
    parent->setStatus("Info: %s", message.c_str());
}

// Analytics dialogs
void PopupDialogs::showDashboardDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatDashboardContent(parent);
    ModernDialog dialog(parent->hwnd, parent, "Professional Dashboard", content, 800, 700);
    dialog.show();
}

void PopupDialogs::showChartsDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatChartsContent(parent);
    ModernDialog dialog(parent->hwnd, parent, "Real-time Charts", content, 700, 600);
    dialog.show();
}

void PopupDialogs::showMonitorDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatMonitorContent(parent);
    ModernDialog dialog(parent->hwnd, parent, "Performance Monitor", content, 750, 650);
    dialog.show();
}

void PopupDialogs::showMetricsDialog(RealGameAnalyzerGUI* parent) {
    std::string content = formatMetricsContent(parent);
    ModernDialog dialog(parent->hwnd, parent, "Analytics Metrics", content, 600, 500);
    dialog.show();
}

// Analysis dialogs
void PopupDialogs::showVisionResultsDialog(RealGameAnalyzerGUI* parent, const std::vector<std::string>& detectedTexts) {
    std::string content = formatVisionResultsContent(detectedTexts);
    ModernDialog dialog(parent->hwnd, parent, "Vision Analysis Results", content, 500, 400);
    dialog.show();
}

void PopupDialogs::showHybridAnalysisDialog(RealGameAnalyzerGUI* parent, const std::string& results) {
    std::string content = formatHybridAnalysisContent(results);
    ModernDialog dialog(parent->hwnd, parent, "Hybrid Analysis Results", content, 700, 600);
    dialog.show();
}

void PopupDialogs::showMemoryComparisonDialog(RealGameAnalyzerGUI* parent, const std::string& comparison) {
    std::string content = formatMemoryComparisonContent(comparison);
    ModernDialog dialog(parent->hwnd, parent, "Memory Comparison", content, 600, 500);
    dialog.show();
}

// Profile dialogs
void PopupDialogs::showProfilesListDialog(RealGameAnalyzerGUI* parent, const std::string& profilesList) {
    std::string content = formatProfilesListContent(profilesList);
    ModernDialog dialog(parent->hwnd, parent, "Game Profiles", content, 500, 400);
    dialog.show();
}

// Helper functions - Content formatting
std::string PopupDialogs::formatAboutContent() {
    std::string about = "GAME ANALYZER v3.0\n";
    about += "===========================================================\n\n";
    about += "Professional Gaming Analytics Platform\n";
    about += "Advanced Memory Analysis - Vision Processing - Analytics Dashboard\n\n";
    about += "FEATURES:\n";
    about += "------------------------------------------------------------------\n";
    about += "- Advanced Memory Scanning & Monitoring\n";
    about += "- DirectX 11 Screen Capture & OCR Analysis\n";
    about += "- Hybrid Memory + Vision Correlation\n";
    about += "- Professional Analytics Dashboard\n";
    about += "- Real-time Performance Monitoring\n";
    about += "- Game Profile Management\n";
    about += "- CSV Data Export & Analytics\n\n";
    about += "TECHNICAL SPECIFICATIONS:\n";
    about += "------------------------------------------------------------------\n";
    about += "- Built with C++17 & Windows API\n";
    about += "- DirectX 11 Graphics Integration\n";
    about += "- Multi-threaded Analysis Engine\n";
    about += "- Bloomberg Terminal-style Interface\n";
    about += "- Professional-grade Performance Metrics\n\n";
    about += "COMPATIBILITY:\n";
    about += "------------------------------------------------------------------\n";
    about += "- Windows 10/11 (x64)\n";
    about += "- DirectX 11 Compatible Graphics\n";
    about += "- Administrator privileges recommended\n\n";
    about += "(C) 2024 Game Analyzer - Professional Gaming Analytics\n";
    about += "Version 3.0 - Complete Analytics Platform";
    
    return about;
}

std::string PopupDialogs::formatSettingsContent(RealGameAnalyzerGUI* parent) {
    std::string settings = "GAME ANALYZER SETTINGS\n\n";
    settings += "Current Settings:\n";
    settings += "- Modern Dark Theme: " + std::string(parent->modernThemeEnabled ? "ENABLED" : "DISABLED") + "\n";
    settings += "- Show System Processes: " + std::string(parent->showSystemProcesses ? "ENABLED" : "DISABLED") + "\n\n";
    settings += "To change settings, use the top ribbon controls.";
    
    return settings;
}

std::string PopupDialogs::formatHelpContent() {
    std::string content = "GAME ANALYZER HELP & GUIDE\n";
    content += "===========================================================\n\n";
    
    content += "QUICK START GUIDE:\n";
    content += "------------------------------------------------------------------\n";
    content += "1. Select a game process from the list\n";
    content += "2. Click 'Scan Game Data' to find memory addresses\n";
    content += "3. Click 'Compare Values' to capture baseline\n";
    content += "4. Play your game and click 'Show Changed Values'\n";
    content += "5. Changed addresses auto-add to monitoring\n";
    content += "6. Click 'Start Monitoring' for real-time tracking\n\n";
    
    content += "PRO TIPS:\n";
    content += "- Use 'Scan Game Data' for faster, targeted results\n";
    content += "- Save profiles after finding good addresses\n";
    content += "- Run as Administrator for better memory access\n";
    content += "- Use search boxes to filter processes and addresses\n\n";
    
    content += "ADVANCED FEATURES:\n";
    content += "------------------------------------------------------------------\n";
    content += "VISION ANALYSIS:\n";
    content += "- Capture screen + OCR text detection\n";
    content += "- Analyze game UI elements\n";
    content += "- Detect text changes in real-time\n\n";
    content += "HYBRID ANALYSIS:\n";
    content += "- Combine memory + vision data\n";
    content += "- Cross-reference different data sources\n";
    content += "- Get comprehensive game state analysis\n\n";
    content += "ANALYTICS DASHBOARD:\n";
    content += "- Professional performance metrics\n";
    content += "- Real-time charts and graphs\n";
    content += "- Export data for external analysis\n\n";
    
    content += "TROUBLESHOOTING:\n";
    content += "------------------------------------------------------------------\n";
    content += "'Access Denied' Error:\n";
    content += "- Run as Administrator\n";
    content += "- Check if antivirus is blocking access\n";
    content += "- Ensure game is running\n\n";
    content += "No Addresses Found:\n";
    content += "- Try different scanning methods\n";
    content += "- Use 'Scan Game Data' instead of full scan\n";
    content += "- Check if game is actively running\n\n";
    content += "Process Not Listed:\n";
    content += "- Click 'Refresh Processes'\n";
    content += "- Enable 'Show System Processes'\n";
    content += "- Restart the game\n\n";
    content += "Vision Not Working:\n";
    content += "- Check DirectX 11 compatibility\n";
    content += "- Update graphics drivers\n";
    content += "- Try different capture area\n\n";
    
    content += "For additional support, check the documentation or contact support.";
    
    return content;
}


std::string PopupDialogs::formatDashboardContent(RealGameAnalyzerGUI* parent) {
    if (!parent->selectedProcess) {
        return "No process selected. Please select a process first.";
    }
    
    std::string dashboard = "PROFESSIONAL GAMING ANALYTICS DASHBOARD\n";
    dashboard += "===========================================================\n\n";
    if (parent->selectedProcess) {
        dashboard += "GAME: " + parent->selectedProcess->name + " (PID: " + std::to_string(parent->selectedProcess->pid) + ")\n";
    } else {
        dashboard += "GAME: No process selected\n";
    }
    dashboard += "Session Duration: " + std::to_string(parent->analytics.totalAnalysisRuns) + " analysis cycles\n\n";
    dashboard += "PERFORMANCE OVERVIEW:\n";
    dashboard += "------------------------------------------------------------------\n";
    dashboard += "- Memory Stability: " + std::to_string((int)parent->analytics.averageMemoryStability) + "%\n";
    dashboard += "- Vision Accuracy: " + std::to_string((int)parent->analytics.averageVisionAccuracy) + "%\n";
    dashboard += "- Data Points: " + std::to_string(parent->analytics.memoryValues.size()) + " memory, " + 
                std::to_string(parent->analytics.visionConfidence.size()) + " vision\n\n";
    dashboard += "RECOMMENDATIONS:\n";
    dashboard += "------------------------------------------------------------------\n";
    if (parent->analytics.averageMemoryStability < 70) {
        dashboard += "WARNING: Consider running as Administrator for better memory access\n";
    }
    if (parent->analytics.averageVisionAccuracy < 70) {
        dashboard += "WARNING: Try capturing different screen areas for better text detection\n";
    }
    dashboard += "Continue monitoring for performance optimization opportunities\n";
    
    return dashboard;
}

std::string PopupDialogs::formatChartsContent(RealGameAnalyzerGUI* parent) {
    std::string charts = "REAL-TIME PERFORMANCE CHARTS\n";
    charts += "===========================================================\n\n";
    charts += "MEMORY VALUES TREND:\n";
    charts += "------------------------------------------------------------------\n";
    charts += "Memory stability: " + std::to_string((int)parent->analytics.averageMemoryStability) + "%\n\n";
    charts += "VISION ACCURACY TREND:\n";
    charts += "------------------------------------------------------------------\n";
    charts += "Vision accuracy: " + std::to_string((int)parent->analytics.averageVisionAccuracy) + "%\n\n";
    charts += "PERFORMANCE SUMMARY:\n";
    charts += "------------------------------------------------------------------\n";
    charts += "- Memory Stability: " + std::to_string((int)parent->analytics.averageMemoryStability) + "%\n";
    charts += "- Vision Accuracy: " + std::to_string((int)parent->analytics.averageVisionAccuracy) + "%\n";
    charts += "- Total Samples: " + std::to_string(parent->analytics.memoryValues.size() + parent->analytics.visionConfidence.size()) + "\n";
    charts += "- Analysis Runs: " + std::to_string(parent->analytics.totalAnalysisRuns) + "\n";
    
    return charts;
}

std::string PopupDialogs::formatMonitorContent(RealGameAnalyzerGUI* parent) {
    if (!parent->selectedProcess) {
        return "No process selected. Please select a process first.";
    }
    
    std::string monitor = "REAL-TIME PERFORMANCE MONITOR\n";
    monitor += "===========================================================\n\n";
    if (parent->selectedProcess) {
        monitor += "TARGET: " + parent->selectedProcess->name + " (PID: " + std::to_string(parent->selectedProcess->pid) + ")\n\n";
    } else {
        monitor += "TARGET: No process selected\n\n";
    }
    monitor += "CURRENT STATUS:\n";
    monitor += "------------------------------------------------------------------\n";
    monitor += "MEMORY ANALYSIS:\n";
    monitor += "- Monitored Addresses: " + std::to_string(parent->memoryAddresses.size()) + "\n";
    monitor += "- Discovered Addresses: " + std::to_string(parent->discoveredAddresses.size()) + "\n";
    monitor += "- Memory Regions: " + std::to_string(parent->memoryRegions.size()) + "\n";
    if (parent->analytics.totalAnalysisRuns > 0) {
        monitor += "- Stability Score: " + std::to_string((int)parent->analytics.averageMemoryStability) + "%\n";
    }
    monitor += "\nVISION ANALYSIS:\n";
    monitor += "- Captured Frames: " + (parent->lastFrameData.empty() ? "None" : std::to_string(parent->frameWidth) + "x" + std::to_string(parent->frameHeight)) + "\n";
    monitor += "- Detected Text Regions: " + std::to_string(parent->detectedTexts.size()) + "\n";
    if (parent->analytics.totalAnalysisRuns > 0) {
        monitor += "- Accuracy Score: " + std::to_string((int)parent->analytics.averageVisionAccuracy) + "%\n";
    }
    monitor += "\nPERFORMANCE RECOMMENDATIONS:\n";
    monitor += "------------------------------------------------------------------\n";
    if (parent->memoryAddresses.empty() && parent->discoveredAddresses.empty()) {
        monitor += "WARNING: No memory data - Run 'Scan Process Memory' or 'Scan Game Data'\n";
    }
    if (parent->detectedTexts.empty()) {
        monitor += "WARNING: No vision data - Run 'Capture Screen' and 'Start Vision Analysis'\n";
    }
    if (parent->analytics.totalAnalysisRuns == 0) {
        monitor += "WARNING: No analytics data - Run 'Start Analytics' for performance metrics\n";
    }
    if (!parent->memoryAddresses.empty() && !parent->detectedTexts.empty()) {
        monitor += "Run 'Hybrid Analysis' for comprehensive game insights\n";
    }
    
    return monitor;
}

std::string PopupDialogs::formatMetricsContent(RealGameAnalyzerGUI* parent) {
    std::string metrics = "ANALYTICS METRICS\n";
    metrics += "===========================================================\n\n";
    metrics += "OVERVIEW:\n";
    metrics += "- Total Analysis Runs: " + std::to_string(parent->analytics.totalAnalysisRuns) + "\n";
    metrics += "- Memory Samples: " + std::to_string(parent->analytics.memoryValues.size()) + "\n";
    metrics += "- Vision Samples: " + std::to_string(parent->analytics.visionConfidence.size()) + "\n\n";
    metrics += "PERFORMANCE METRICS:\n";
    metrics += "- Memory Stability: " + std::to_string((int)parent->analytics.averageMemoryStability) + "%\n";
    metrics += "- Vision Accuracy: " + std::to_string((int)parent->analytics.averageVisionAccuracy) + "%\n\n";
    metrics += "TRENDS:\n";
    for (const auto& trend : parent->analytics.valueTrends) {
        std::string trendDirection = trend.second > 0 ? "Increasing" : (trend.second < 0 ? "Decreasing" : "Stable");
        metrics += "- " + trend.first + ": " + trendDirection + " (" + std::to_string(trend.second) + ")\n";
    }
    metrics += "\nVALUE CHANGES:\n";
    for (const auto& change : parent->analytics.valueChangeCounts) {
        metrics += "- " + change.first + ": " + std::to_string(change.second) + " changes\n";
    }
    
    return metrics;
}

std::string PopupDialogs::formatVisionResultsContent(const std::vector<std::string>& detectedTexts) {
    std::string results = "VISION ANALYSIS RESULTS\n";
    results += "===========================================================\n\n";
    results += "Detected text regions:\n\n";
    for (size_t i = 0; i < detectedTexts.size() && i < 10; ++i) {
        results += "- " + detectedTexts[i] + "\n";
    }
    if (detectedTexts.size() > 10) {
        results += "... and " + std::to_string(detectedTexts.size() - 10) + " more regions\n";
    }
    results += "\nTotal regions detected: " + std::to_string(detectedTexts.size());
    
    return results;
}

std::string PopupDialogs::formatHybridAnalysisContent(const std::string& results) {
    return "HYBRID ANALYSIS RESULTS\n" + std::string(60, '=') + "\n\n" + results;
}

std::string PopupDialogs::formatMemoryComparisonContent(const std::string& comparison) {
    return "MEMORY COMPARISON RESULTS\n" + std::string(60, '=') + "\n\n" + comparison;
}

std::string PopupDialogs::formatProfilesListContent(const std::string& profilesList) {
    return "GAME PROFILES\n" + std::string(60, '=') + "\n\n" + profilesList;
}
