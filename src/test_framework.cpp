#include "test_framework.h"
#include <algorithm>
#include <numeric>

namespace BloombergTerminalTests {

void BloombergTestFramework::registerTest(const std::string& component, const std::string& testName,
                                         std::function<TestResult()> testFunction) {
    tests_.push_back({component, testName, testFunction});
}

void BloombergTestFramework::registerBenchmark(const std::string& component, const std::string& benchmarkName,
                                              std::function<BenchmarkResult()> benchmarkFunction) {
    benchmarks_.push_back({component, benchmarkName, benchmarkFunction});
}

void BloombergTestFramework::runAllTests() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  BLOOMBERG TERMINAL TEST SUITE" << std::endl;
    std::cout << "  Enterprise Gaming Analytics Platform" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    testResults_.clear();
    
    for (const auto& test : tests_) {
        std::cout << "Running: " << test.component << "::" << test.name << "... ";
        std::cout.flush();
        
        auto start = std::chrono::high_resolution_clock::now();
        TestResult result = test.function();
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        result.executionTimeMs = duration.count() / 1000.0;
        
        testResults_.push_back(result);
        
        if (result.passed) {
            std::cout << "✅ PASSED (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)" << std::endl;
            std::cout.flush();
        } else {
            std::cout << "❌ FAILED" << std::endl;
            std::cout << "   Error: " << result.message << std::endl;
            std::cout.flush();
        }
    }
    
    std::cout << std::endl;
    printSummary();
    std::cout.flush();
}

void BloombergTestFramework::runComponentTests(const std::string& component) {
    std::cout << "Running tests for component: " << component << std::endl << std::endl;
    
    testResults_.clear();
    
    for (const auto& test : tests_) {
        if (test.component == component) {
            std::cout << "Running: " << test.name << "... ";
            
            auto start = std::chrono::high_resolution_clock::now();
            TestResult result = test.function();
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            result.executionTimeMs = duration.count() / 1000.0;
            
            testResults_.push_back(result);
            
            if (result.passed) {
                std::cout << "✅ PASSED (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)" << std::endl;
            } else {
                std::cout << "❌ FAILED" << std::endl;
                std::cout << "   Error: " << result.message << std::endl;
            }
        }
    }
    
    std::cout << std::endl;
    printSummary();
}

void BloombergTestFramework::runBenchmarks() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  BLOOMBERG TERMINAL BENCHMARKS" << std::endl;
    std::cout << "  Performance Validation Suite" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    benchmarkResults_.clear();
    
    for (const auto& benchmark : benchmarks_) {
        std::cout << "Benchmarking: " << benchmark.component << "::" << benchmark.name << "... ";
        
        BenchmarkResult result = benchmark.function();
        benchmarkResults_.push_back(result);
        
        std::cout << "✅ COMPLETED" << std::endl;
        std::cout << "   Average Time: " << std::fixed << std::setprecision(2) << result.averageTimeMs << "ms" << std::endl;
        std::cout << "   Throughput: " << std::fixed << std::setprecision(1) << result.throughput << " items/sec" << std::endl;
        
        // Check against performance targets
        if (benchmark.component == "OptimizedScreenCapture" && result.averageTimeMs > TARGET_PROCESSING_LATENCY_MS) {
            std::cout << "   ⚠️  WARNING: Exceeds target processing latency (" << TARGET_PROCESSING_LATENCY_MS << "ms)" << std::endl;
        }
        std::cout << std::endl;
    }
}

TestStatistics BloombergTestFramework::getStatistics() const {
    TestStatistics stats;
    
    stats.totalTests = testResults_.size();
    stats.passedTests = std::count_if(testResults_.begin(), testResults_.end(),
                                    [](const TestResult& r) { return r.passed; });
    stats.failedTests = stats.totalTests - stats.passedTests;
    
    stats.totalExecutionTimeMs = std::accumulate(testResults_.begin(), testResults_.end(), 0.0,
                                               [](double sum, const TestResult& r) { return sum + r.executionTimeMs; });
    
    // Component statistics
    for (const auto& result : testResults_) {
        stats.componentTestCounts[result.component]++;
        if (result.passed) {
            stats.componentPassCounts[result.component]++;
        }
    }
    
    return stats;
}

void BloombergTestFramework::generateReport(const std::string& filename) {
    std::ofstream report(filename);
    if (!report.is_open()) {
        std::cerr << "Failed to create report file: " << filename << std::endl;
        return;
    }
    
    auto stats = getStatistics();
    
    report << "==========================================" << std::endl;
    report << "  BLOOMBERG TERMINAL TEST REPORT" << std::endl;
    report << "  Enterprise Gaming Analytics Platform" << std::endl;
    report << "==========================================" << std::endl << std::endl;
    
    // Executive Summary
    report << "📊 EXECUTIVE SUMMARY:" << std::endl;
    report << "  Total Tests Executed: " << stats.totalTests << std::endl;
    report << "  Tests Passed: " << stats.passedTests << " (" << std::fixed << std::setprecision(1) 
           << stats.getPassRate() << "%)" << std::endl;
    report << "  Tests Failed: " << stats.failedTests << std::endl;
    report << "  Total Execution Time: " << std::fixed << std::setprecision(2) 
           << stats.totalExecutionTimeMs << "ms" << std::endl;
    report << "  Average Test Time: " << std::fixed << std::setprecision(2) 
           << stats.getAverageExecutionTime() << "ms" << std::endl;
    report << std::endl;
    
    // Component Breakdown
    report << "🔧 COMPONENT BREAKDOWN:" << std::endl;
    for (const auto& [component, total] : stats.componentTestCounts) {
        auto passed = stats.componentPassCounts.find(component);
        size_t passedCount = (passed != stats.componentPassCounts.end()) ? passed->second : 0;
        double passRate = total > 0 ? (static_cast<double>(passedCount) / total) * 100.0 : 0.0;
        
        report << "  " << component << ": " << passedCount << "/" << total 
               << " (" << std::fixed << std::setprecision(1) << passRate << "%)" << std::endl;
    }
    report << std::endl;
    
    // Detailed Results
    report << "📋 DETAILED TEST RESULTS:" << std::endl;
    for (const auto& result : testResults_) {
        report << "  [" << (result.passed ? "PASS" : "FAIL") << "] " 
               << result.component << "::" << result.testName << std::endl;
        report << "      Time: " << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms" << std::endl;
        if (!result.passed) {
            report << "      Error: " << result.message << std::endl;
        }
        report << std::endl;
    }
    
    // Performance Targets Validation
    report << "🎯 PERFORMANCE TARGETS VALIDATION:" << std::endl;
    report << "  Target Processing Latency: " << TARGET_PROCESSING_LATENCY_MS << "ms" << std::endl;
    report << "  Target Memory Usage: " << TARGET_MEMORY_USAGE_MB << "MB" << std::endl;
    report << "  Target CPU Usage: " << TARGET_CPU_USAGE_PERCENT << "%" << std::endl;
    report << "  Target OCR Accuracy: " << TARGET_OCR_ACCURACY_PERCENT << "%" << std::endl;
    report << "  Target Capture Rate: " << TARGET_CAPTURE_RATE_FPS << " FPS" << std::endl;
    report << "  Target Startup Time: " << TARGET_STARTUP_TIME_MS << "ms" << std::endl;
    report << std::endl;
    
    // Benchmark Results
    if (!benchmarkResults_.empty()) {
        report << "📈 BENCHMARK RESULTS:" << std::endl;
        for (const auto& benchmark : benchmarkResults_) {
            report << "  " << benchmark.component << "::" << benchmark.benchmarkName << std::endl;
            report << "      Average Time: " << std::fixed << std::setprecision(2) << benchmark.averageTimeMs << "ms" << std::endl;
            report << "      Min Time: " << std::fixed << std::setprecision(2) << benchmark.minTimeMs << "ms" << std::endl;
            report << "      Max Time: " << std::fixed << std::setprecision(2) << benchmark.maxTimeMs << "ms" << std::endl;
            report << "      Throughput: " << std::fixed << std::setprecision(1) << benchmark.throughput << " items/sec" << std::endl;
            report << std::endl;
        }
    }
    
    // Recommendations
    report << "💡 RECOMMENDATIONS:" << std::endl;
    if (stats.getPassRate() >= 95.0) {
        report << "  ✅ Excellent test coverage and reliability" << std::endl;
    } else if (stats.getPassRate() >= 80.0) {
        report << "  ⚠️  Good test coverage, consider addressing failed tests" << std::endl;
    } else {
        report << "  ❌ Significant test failures detected, review implementation" << std::endl;
    }
    
    if (stats.getAverageExecutionTime() > 100.0) {
        report << "  ⚠️  Consider optimizing slow tests" << std::endl;
    }
    
    report << std::endl;
    report << "Report generated: " << std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) << std::endl;
    
    report.close();
    std::cout << "📄 Test report generated: " << filename << std::endl;
}

void BloombergTestFramework::printSummary() {
    auto stats = getStatistics();
    
    std::cout << "==========================================" << std::endl;
    std::cout << "        TEST EXECUTION SUMMARY" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    if (stats.failedTests == 0) {
        std::cout << "🎉 ALL TESTS PASSED!" << std::endl;
        std::cout << "🚀 Bloomberg Terminal is PRODUCTION READY" << std::endl;
        std::cout << "✅ " << stats.passedTests << "/" << stats.totalTests << " tests passed" << std::endl;
        std::cout << "📊 Pass Rate: " << std::fixed << std::setprecision(1) << stats.getPassRate() << "%" << std::endl;
        std::cout << "⏱️  Average Test Time: " << std::fixed << std::setprecision(2) << stats.getAverageExecutionTime() << "ms" << std::endl;
    } else {
        std::cout << "❌ TEST FAILURES DETECTED!" << std::endl;
        std::cout << "🔴 " << stats.failedTests << " tests failed" << std::endl;
        std::cout << "📊 Pass Rate: " << std::fixed << std::setprecision(1) << stats.getPassRate() << "%" << std::endl;
        std::cout << "⚠️  System requires fixes before production deployment" << std::endl;
    }
    
    std::cout << std::endl;
}

} // namespace BloombergTerminalTests
