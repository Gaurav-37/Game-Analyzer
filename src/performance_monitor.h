#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

class PerformanceMonitor {
public:
    struct PerformanceMetric {
        std::string name;
        double averageTime;
        double minTime;
        double maxTime;
        uint64_t totalCalls;
        double totalTime;
        
        PerformanceMetric() : averageTime(0), minTime(0), maxTime(0), totalCalls(0), totalTime(0) {}
    };
    
    static PerformanceMonitor& getInstance();
    
    // Start timing an operation
    void startTimer(const std::string& operation);
    
    // End timing and record the result
    void endTimer(const std::string& operation);
    
    // Get performance statistics
    PerformanceMetric getMetric(const std::string& operation) const;
    
    // Get all metrics
    std::map<std::string, PerformanceMetric> getAllMetrics() const;
    
    // Reset all metrics
    void reset();
    
    // Check if operation meets performance target
    bool meetsTarget(const std::string& operation, double targetMs) const;
    
    // Performance targets (in milliseconds)
    static constexpr double TARGET_CAPTURE_TIME = 16.67; // 60 FPS
    static constexpr double TARGET_OCR_TIME = 50.0;      // <50ms target
    static constexpr double TARGET_PROCESSING_TIME = 50.0;
    static constexpr double TARGET_ANALYSIS_TIME = 100.0;

private:
    PerformanceMonitor() = default;
    
    mutable std::mutex metricsMutex;
    std::map<std::string, PerformanceMetric> metrics;
    std::map<std::string, std::chrono::high_resolution_clock::time_point> activeTimers;
    
    void updateMetric(const std::string& operation, double duration);
};

// RAII Timer class for automatic timing
class ScopedTimer {
public:
    ScopedTimer(const std::string& operation);
    ~ScopedTimer();
    
private:
    std::string operation;
    std::chrono::high_resolution_clock::time_point startTime;
};

// Macro for easy timing
#define TIMED_OPERATION(name) ScopedTimer _timer(name)
