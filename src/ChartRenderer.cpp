#include "ChartRenderer.h"
#include <algorithm>
#include <numeric>
#include <cmath>

ChartRenderer::ChartRenderer() 
    : initialized(false)
    , colorIndex(0)
    , chartHeight(300.0f)
    , showGrid(true)
    , showLegend(true)
    , autoScale(true)
    , lineThickness(2.0f)
    , maxDataPoints(1000)
{
    // Initialize color palette
    colors = {
        ImVec4(0.0f, 0.8f, 1.0f, 1.0f),  // Cyan
        ImVec4(0.0f, 1.0f, 0.5f, 1.0f),  // Green
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f),  // Red
        ImVec4(1.0f, 0.8f, 0.0f, 1.0f),  // Yellow
        ImVec4(0.8f, 0.0f, 1.0f, 1.0f),  // Purple
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange
        ImVec4(0.0f, 0.5f, 1.0f, 1.0f),  // Blue
        ImVec4(1.0f, 0.0f, 0.8f, 1.0f)   // Pink
    };
}

ChartRenderer::~ChartRenderer() {
    shutdown();
}

bool ChartRenderer::initialize() {
    initialized = true;
    setupChartStyle();
    return true;
}

void ChartRenderer::shutdown() {
    initialized = false;
}

void ChartRenderer::renderChart(const std::vector<MemoryAddress>& addresses) {
    if (!initialized || addresses.empty()) {
        return;
    }
    
    // Calculate chart dimensions
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.y < 50) {
        canvasSize.y = chartHeight;
    }
    
    // Setup ImPlot
    if (ImPlot::BeginPlot("Memory Values", canvasSize, ImPlotFlags_NoTitle | ImPlotFlags_NoMenus)) {
        ImPlot::SetupAxes("Time", "Value", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        
        if (showGrid) {
            ImPlot::SetupAxesLimits(0, maxDataPoints, 0, 0, ImGuiCond_Always);
        }
        
        // Render each address
        resetColorIndex();
        for (const auto& address : addresses) {
            if (!address.values.empty()) {
                ImVec4 color = getNextColor();
                ImPlot::SetNextLineStyle(color, lineThickness);
                
                // Create time axis for this address
                std::vector<double> timeAxis(address.values.size());
                std::iota(timeAxis.begin(), timeAxis.end(), 0.0);
                
                ImPlot::PlotLine(address.name.c_str(), 
                               timeAxis.data(), 
                               address.values.data(), 
                               static_cast<int>(address.values.size()));
            }
        }
        
        if (showLegend) {
            ImPlot::PlotLegend();
        }
        
        ImPlot::EndPlot();
    }
}

void ChartRenderer::renderValueChart(const MemoryAddress& address) {
    if (!initialized || address.values.empty()) {
        return;
    }
    
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.y < 50) {
        canvasSize.y = chartHeight;
    }
    
    std::string title = address.name + " (" + address.type + ")";
    
    if (ImPlot::BeginPlot(title.c_str(), canvasSize)) {
        ImPlot::SetupAxes("Time", "Value");
        
        // Create time axis
        std::vector<double> timeAxis(address.values.size());
        std::iota(timeAxis.begin(), timeAxis.end(), 0.0);
        
        // Plot the line
        ImPlot::PlotLine("Value", timeAxis.data(), address.values.data(), 
                        static_cast<int>(address.values.size()));
        
        // Add current value annotation
        if (!address.values.empty()) {
            double lastValue = address.values.back();
            ImPlot::Annotation(static_cast<double>(address.values.size() - 1), lastValue, 
                             ImVec4(1, 1, 1, 1), ImVec2(0, 0), true, 
                             ("Current: " + std::to_string(lastValue)).c_str());
        }
        
        ImPlot::EndPlot();
    }
}

void ChartRenderer::renderComparisonChart(const std::vector<MemoryAddress>& addresses) {
    if (!initialized || addresses.empty()) {
        return;
    }
    
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.y < 50) {
        canvasSize.y = chartHeight;
    }
    
    if (ImPlot::BeginPlot("Value Comparison", canvasSize)) {
        ImPlot::SetupAxes("Time", "Value");
        
        // Find the maximum data points
        size_t maxPoints = 0;
        for (const auto& address : addresses) {
            maxPoints = std::max(maxPoints, address.values.size());
        }
        
        if (maxPoints > 0) {
            // Create normalized time axis
            std::vector<double> timeAxis(maxPoints);
            for (size_t i = 0; i < maxPoints; ++i) {
                timeAxis[i] = static_cast<double>(i) / static_cast<double>(maxPoints - 1);
            }
            
            resetColorIndex();
            for (const auto& address : addresses) {
                if (!address.values.empty()) {
                    ImVec4 color = getNextColor();
                    ImPlot::SetNextLineStyle(color, lineThickness);
                    
                    // Normalize values to 0-1 range for comparison
                    std::vector<double> normalizedValues = address.values;
                    if (normalizedValues.size() > 1) {
                        double minVal = *std::min_element(normalizedValues.begin(), normalizedValues.end());
                        double maxVal = *std::max_element(normalizedValues.begin(), normalizedValues.end());
                        double range = maxVal - minVal;
                        
                        if (range > 0) {
                            for (auto& val : normalizedValues) {
                                val = (val - minVal) / range;
                            }
                        }
                    }
                    
                    ImPlot::PlotLine(address.name.c_str(), 
                                   timeAxis.data(), 
                                   normalizedValues.data(), 
                                   static_cast<int>(normalizedValues.size()));
                }
            }
        }
        
        ImPlot::EndPlot();
    }
}

void ChartRenderer::setChartHeight(float height) {
    chartHeight = std::max(100.0f, height);
}

void ChartRenderer::setShowGrid(bool show) {
    showGrid = show;
}

void ChartRenderer::setShowLegend(bool show) {
    showLegend = show;
}

void ChartRenderer::setAutoScale(bool autoScale) {
    this->autoScale = autoScale;
}

void ChartRenderer::setLineThickness(float thickness) {
    lineThickness = std::max(0.5f, std::min(10.0f, thickness));
}

void ChartRenderer::setMaxDataPoints(size_t maxPoints) {
    maxDataPoints = std::max(100UL, maxPoints);
}

ImVec4 ChartRenderer::getNextColor() {
    if (colors.empty()) {
        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    ImVec4 color = colors[colorIndex % colors.size()];
    colorIndex++;
    return color;
}

void ChartRenderer::resetColorIndex() {
    colorIndex = 0;
}

void ChartRenderer::updateTimeAxis(size_t dataPoints) {
    timeAxis.resize(dataPoints);
    std::iota(timeAxis.begin(), timeAxis.end(), 0.0);
}

std::vector<double> ChartRenderer::getTimeAxis() const {
    return timeAxis;
}

ChartRenderer::ChartStats ChartRenderer::calculateStats(const std::vector<double>& values) {
    ChartStats stats = {};
    
    if (values.empty()) {
        return stats;
    }
    
    stats.dataPoints = values.size();
    stats.currentValue = values.back();
    
    auto minMax = std::minmax_element(values.begin(), values.end());
    stats.minValue = *minMax.first;
    stats.maxValue = *minMax.second;
    
    stats.avgValue = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    
    return stats;
}

void ChartRenderer::setupChartStyle() {
    // Configure ImPlot style
    ImPlotStyle& style = ImPlot::GetStyle();
    style.LineWeight = lineThickness;
    style.MarkerSize = 4.0f;
    style.PlotPadding = ImVec2(10, 10);
    style.LabelPadding = ImVec2(5, 5);
    style.LegendPadding = ImVec2(10, 10);
    style.FitPadding = ImVec2(0.1f, 0.1f);
    style.DefaultErrorBarSize = 5.0f;
    style.DefaultErrorBarWeight = 1.5f;
    style.MinorAlpha = 0.25f;
    style.MajorTickLen = ImVec2(10, 10);
    style.MinorTickLen = ImVec2(5, 5);
    style.MajorTickSize = ImVec2(1, 1);
    style.MinorTickSize = ImVec2(1, 1);
    style.MajorGridSize = ImVec2(1, 1);
    style.MinorGridSize = ImVec2(1, 1);
    style.PlotDefaultSize = ImVec2(400, 300);
    style.PlotMinSize = ImVec2(200, 150);
}

void ChartRenderer::renderChartBackground() {
    // Custom background rendering if needed
}

void ChartRenderer::renderChartGrid() {
    // Custom grid rendering if needed
}

void ChartRenderer::renderChartLegend(const std::vector<MemoryAddress>& addresses) {
    // Custom legend rendering if needed
}

ImVec4 ChartRenderer::generateColor(int index) {
    float hue = (index * 137.5f) / 360.0f; // Golden angle approximation
    return hsvToRgb(hue, 0.8f, 1.0f);
}

ImVec4 ChartRenderer::hsvToRgb(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = v - c;
    
    float r, g, b;
    
    if (h < 1.0f / 6.0f) {
        r = c; g = x; b = 0;
    } else if (h < 2.0f / 6.0f) {
        r = x; g = c; b = 0;
    } else if (h < 3.0f / 6.0f) {
        r = 0; g = c; b = x;
    } else if (h < 4.0f / 6.0f) {
        r = 0; g = x; b = c;
    } else if (h < 5.0f / 6.0f) {
        r = x; g = 0; b = c;
    } else {
        r = c; g = 0; b = x;
    }
    
    return ImVec4(r + m, g + m, b + m, 1.0f);
}

std::vector<double> ChartRenderer::smoothData(const std::vector<double>& data, int windowSize) {
    if (data.size() < windowSize) {
        return data;
    }
    
    std::vector<double> smoothed(data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        double sum = 0.0;
        int count = 0;
        
        for (int j = -windowSize / 2; j <= windowSize / 2; ++j) {
            int index = static_cast<int>(i) + j;
            if (index >= 0 && index < static_cast<int>(data.size())) {
                sum += data[index];
                count++;
            }
        }
        
        smoothed[i] = sum / count;
    }
    
    return smoothed;
}

std::vector<double> ChartRenderer::downsampleData(const std::vector<double>& data, int factor) {
    if (factor <= 1 || data.empty()) {
        return data;
    }
    
    std::vector<double> downsampled;
    downsampled.reserve(data.size() / factor);
    
    for (size_t i = 0; i < data.size(); i += factor) {
        downsampled.push_back(data[i]);
    }
    
    return downsampled;
}
