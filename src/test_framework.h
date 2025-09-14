#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <map>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

namespace BloombergTerminalTests {

// Test result structure
struct TestResult {
    std::string testName;
    std::string component;
    bool passed;
    std::string message;
    double executionTimeMs;
    std::string timestamp;
    
    TestResult(const std::string& name, const std::string& comp, bool pass, 
               const std::string& msg, double time = 0.0)
        : testName(name), component(comp), passed(pass), message(msg), executionTimeMs(time) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        timestamp = std::ctime(&time_t);
        if (!timestamp.empty() && timestamp.back() == '\n') {
            timestamp.pop_back();
        }
    }
};

// Benchmark result structure
struct BenchmarkResult {
    std::string benchmarkName;
    std::string component;
    double averageTimeMs;
    double minTimeMs;
    double maxTimeMs;
    size_t iterations;
    size_t itemsProcessed;
    double throughput;
    
    BenchmarkResult(const std::string& name, const std::string& comp, double avg, 
                   double min, double max, size_t iter, size_t items)
        : benchmarkName(name), component(comp), averageTimeMs(avg), 
          minTimeMs(min), maxTimeMs(max), iterations(iter), itemsProcessed(items) {
        throughput = itemsProcessed / (averageTimeMs / 1000.0);
    }
};

// Test statistics
struct TestStatistics {
    size_t totalTests = 0;
    size_t passedTests = 0;
    size_t failedTests = 0;
    double totalExecutionTimeMs = 0.0;
    std::map<std::string, size_t> componentTestCounts;
    std::map<std::string, size_t> componentPassCounts;
    
    double getPassRate() const {
        return totalTests > 0 ? (static_cast<double>(passedTests) / totalTests) * 100.0 : 0.0;
    }
    
    double getAverageExecutionTime() const {
        return totalTests > 0 ? totalExecutionTimeMs / totalTests : 0.0;
    }
};

// Main test framework class
class BloombergTestFramework {
public:
    static BloombergTestFramework& getInstance() {
        static BloombergTestFramework instance;
        return instance;
    }
    
    // Test registration
    void registerTest(const std::string& component, const std::string& testName, 
                     std::function<TestResult()> testFunction);
    
    // Benchmark registration
    void registerBenchmark(const std::string& component, const std::string& benchmarkName,
                          std::function<BenchmarkResult()> benchmarkFunction);
    
    // Test execution
    void runAllTests();
    void runComponentTests(const std::string& component);
    void runBenchmarks();
    
    // Results
    const std::vector<TestResult>& getTestResults() const { return testResults_; }
    const std::vector<BenchmarkResult>& getBenchmarkResults() const { return benchmarkResults_; }
    TestStatistics getStatistics() const;
    
    // Reporting
    void generateReport(const std::string& filename = "bloomberg_test_report.txt");
    void printSummary();
    
    // Performance targets from prompt.md
    static constexpr double TARGET_PROCESSING_LATENCY_MS = 50.0;
    static constexpr double TARGET_MEMORY_USAGE_MB = 200.0;
    static constexpr double TARGET_CPU_USAGE_PERCENT = 15.0;
    static constexpr double TARGET_OCR_ACCURACY_PERCENT = 95.0;
    static constexpr double TARGET_CAPTURE_RATE_FPS = 60.0;
    static constexpr double TARGET_STARTUP_TIME_MS = 2000.0;

private:
    BloombergTestFramework() = default;
    
    struct TestInfo {
        std::string component;
        std::string name;
        std::function<TestResult()> function;
    };
    
    struct BenchmarkInfo {
        std::string component;
        std::string name;
        std::function<BenchmarkResult()> function;
    };
    
    std::vector<TestInfo> tests_;
    std::vector<BenchmarkInfo> benchmarks_;
    std::vector<TestResult> testResults_;
    std::vector<BenchmarkResult> benchmarkResults_;
};

// Test assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        return TestResult(__FUNCTION__, "", false, "Assertion failed: " #condition); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        return TestResult(__FUNCTION__, "", false, "Assertion failed: " #condition " should be false"); \
    }

#define ASSERT_EQUALS(expected, actual) \
    if ((expected) != (actual)) { \
        return TestResult(__FUNCTION__, "", false, \
            "Assertion failed: expected " + std::to_string(expected) + \
            ", got " + std::to_string(actual)); \
    }

#define ASSERT_NOT_NULL(ptr) \
    if ((ptr) == nullptr) { \
        return TestResult(__FUNCTION__, "", false, "Assertion failed: " #ptr " is null"); \
    }

#define ASSERT_GT(value1, value2) \
    if ((value1) <= (value2)) { \
        return TestResult(__FUNCTION__, "", false, \
            "Assertion failed: " + std::to_string(value1) + " should be greater than " + std::to_string(value2)); \
    }

#define ASSERT_LT(value1, value2) \
    if ((value1) >= (value2)) { \
        return TestResult(__FUNCTION__, "", false, \
            "Assertion failed: " + std::to_string(value1) + " should be less than " + std::to_string(value2)); \
    }

#define ASSERT_NEAR(value1, value2, tolerance) \
    if (std::abs((value1) - (value2)) > (tolerance)) { \
        return TestResult(__FUNCTION__, "", false, \
            "Assertion failed: " + std::to_string(value1) + " is not near " + std::to_string(value2) + \
            " (tolerance: " + std::to_string(tolerance) + ")"); \
    }

// Simple test registration - no complex macros
inline void registerTest(const std::string& component, const std::string& name, 
                        std::function<TestResult()> testFunction) {
    BloombergTestFramework::getInstance().registerTest(component, name, testFunction);
}

inline void registerBenchmark(const std::string& component, const std::string& name,
                             std::function<BenchmarkResult()> benchmarkFunction) {
    BloombergTestFramework::getInstance().registerBenchmark(component, name, benchmarkFunction);
}

// Performance target validation macros
#define VALIDATE_PERFORMANCE_TARGET(actual, target, metric) \
    if ((actual) > (target)) { \
        return TestResult(__FUNCTION__, "", false, \
            "Performance target not met: " metric " was " + std::to_string(actual) + \
            "ms, target is " + std::to_string(target) + "ms"); \
    }

#define VALIDATE_ACCURACY_TARGET(actual, target, metric) \
    if ((actual) < (target)) { \
        return TestResult(__FUNCTION__, "", false, \
            "Accuracy target not met: " metric " was " + std::to_string(actual) + \
            "%, target is " + std::to_string(target) + "%"); \
    }

// Timer utility for benchmarks
class BenchmarkTimer {
public:
    BenchmarkTimer() : start_(std::chrono::high_resolution_clock::now()) {}
    
    double elapsedMs() const {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
        return duration.count() / 1000.0;
    }
    
private:
    std::chrono::high_resolution_clock::time_point start_;
};

} // namespace BloombergTerminalTests
