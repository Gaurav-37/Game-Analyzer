#include "performance_monitor.h"
#include <iostream>
#include <algorithm>

PerformanceMonitor& PerformanceMonitor::getInstance() {
    static PerformanceMonitor instance;
    return instance;
}

void PerformanceMonitor::startTimer(const std::string& operation) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    activeTimers[operation] = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::endTimer(const std::string& operation) {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    auto it = activeTimers.find(operation);
    if (it != activeTimers.end()) {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - it->second).count() / 1000.0; // Convert to milliseconds
        
        updateMetric(operation, duration);
        activeTimers.erase(it);
    }
}

void PerformanceMonitor::updateMetric(const std::string& operation, double duration) {
    auto& metric = metrics[operation];
    
    metric.totalCalls++;
    metric.totalTime += duration;
    metric.averageTime = metric.totalTime / metric.totalCalls;
    
    if (metric.totalCalls == 1) {
        metric.minTime = duration;
        metric.maxTime = duration;
    } else {
        metric.minTime = std::min(metric.minTime, duration);
        metric.maxTime = std::max(metric.maxTime, duration);
    }
}

PerformanceMonitor::PerformanceMetric PerformanceMonitor::getMetric(const std::string& operation) const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    
    auto it = metrics.find(operation);
    return (it != metrics.end()) ? it->second : PerformanceMetric();
}

std::map<std::string, PerformanceMonitor::PerformanceMetric> PerformanceMonitor::getAllMetrics() const {
    std::lock_guard<std::mutex> lock(metricsMutex);
    return metrics;
}

void PerformanceMonitor::reset() {
    std::lock_guard<std::mutex> lock(metricsMutex);
    metrics.clear();
    activeTimers.clear();
}

bool PerformanceMonitor::meetsTarget(const std::string& operation, double targetMs) const {
    auto metric = getMetric(operation);
    return metric.totalCalls > 0 && metric.averageTime <= targetMs;
}

// ScopedTimer Implementation
ScopedTimer::ScopedTimer(const std::string& op) : operation(op) {
    PerformanceMonitor::getInstance().startTimer(operation);
}

ScopedTimer::~ScopedTimer() {
    PerformanceMonitor::getInstance().endTimer(operation);
}
