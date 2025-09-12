#pragma once

#include <vector>
#include <string>
#include <memory>
#include "GameAnalyzer.h"
#include "external/imgui/imgui.h"
#include "external/implot/implot.h"

class ChartRenderer {
private:
    bool initialized;
    std::vector<ImVec4> colors;
    int colorIndex;
    
    // Chart settings
    float chartHeight;
    bool showGrid;
    bool showLegend;
    bool autoScale;
    float lineThickness;
    
    // Data management
    std::vector<double> timeAxis;
    size_t maxDataPoints;
    
public:
    ChartRenderer();
    ~ChartRenderer();
    
    bool initialize();
    void shutdown();
    
    // Render the main chart
    void renderChart(const std::vector<MemoryAddress>& addresses);
    
    // Render individual value charts
    void renderValueChart(const MemoryAddress& address);
    
    // Render comparison chart
    void renderComparisonChart(const std::vector<MemoryAddress>& addresses);
    
    // Chart settings
    void setChartHeight(float height);
    void setShowGrid(bool show);
    void setShowLegend(bool show);
    void setAutoScale(bool autoScale);
    void setLineThickness(float thickness);
    void setMaxDataPoints(size_t maxPoints);
    
    // Color management
    ImVec4 getNextColor();
    void resetColorIndex();
    
    // Data utilities
    void updateTimeAxis(size_t dataPoints);
    std::vector<double> getTimeAxis() const;
    
    // Chart statistics
    struct ChartStats {
        double minValue;
        double maxValue;
        double avgValue;
        double currentValue;
        size_t dataPoints;
    };
    
    ChartStats calculateStats(const std::vector<double>& values);
    
private:
    // Helper functions
    void setupChartStyle();
    void renderChartBackground();
    void renderChartGrid();
    void renderChartLegend(const std::vector<MemoryAddress>& addresses);
    
    // Color generation
    ImVec4 generateColor(int index);
    ImVec4 hsvToRgb(float h, float s, float v);
    
    // Data processing
    std::vector<double> smoothData(const std::vector<double>& data, int windowSize = 3);
    std::vector<double> downsampleData(const std::vector<double>& data, int factor);
};
