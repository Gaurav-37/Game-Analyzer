#include "test_framework.h"
#include <algorithm>
#include <numeric>
#include <iomanip>

TestFramework& TestFramework::getInstance() {
    static TestFramework instance;
    return instance;
}

void TestFramework::addTest(const std::string& suiteName, const std::string& testName, std::function<TestResult()> test) {
    if (testSuites.find(suiteName) == testSuites.end()) {
        testSuites[suiteName] = TestSuite(suiteName);
    }
    
    testSuites[suiteName].tests.push_back([testName, test]() {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        
        result.testName = testName;
        result.executionTimeMs = duration;
        return result;
    });
}

std::vector<TestFramework::TestResult> TestFramework::runAllTests() {
    testResults.clear();
    
    for (auto& [suiteName, suite] : testSuites) {
        std::cout << "Running test suite: " << suiteName << std::endl;
        
        for (auto& test : suite.tests) {
            auto result = test();
            testResults.push_back(result);
            
            std::cout << "  " << (result.passed ? "✅" : "❌") << " " << result.testName;
            if (!result.passed) {
                std::cout << " - " << result.errorMessage;
            }
            std::cout << " (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)" << std::endl;
        }
    }
    
    updateStats();
    return testResults;
}

std::vector<TestFramework::TestResult> TestFramework::runSuite(const std::string& suiteName) {
    testResults.clear();
    
    auto it = testSuites.find(suiteName);
    if (it != testSuites.end()) {
        std::cout << "Running test suite: " << suiteName << std::endl;
        
        for (auto& test : it->second.tests) {
            auto result = test();
            testResults.push_back(result);
            
            std::cout << "  " << (result.passed ? "✅" : "❌") << " " << result.testName;
            if (!result.passed) {
                std::cout << " - " << result.errorMessage;
            }
            std::cout << " (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)" << std::endl;
        }
    }
    
    updateStats();
    return testResults;
}

TestFramework::TestStats TestFramework::getStats() const {
    return stats;
}

std::string TestFramework::generateReport() const {
    std::ostringstream report;
    
    report << "=== BLOOMBERG TERMINAL TEST REPORT ===\n\n";
    report << "Test Summary:\n";
    report << "  Total Tests: " << stats.totalTests << "\n";
    report << "  Passed: " << stats.passedTests << "\n";
    report << "  Failed: " << stats.failedTests << "\n";
    report << "  Success Rate: " << std::fixed << std::setprecision(1) 
           << (stats.totalTests > 0 ? (double)stats.passedTests / stats.totalTests * 100.0 : 0.0) << "%\n\n";
    
    report << "Performance Summary:\n";
    report << "  Total Execution Time: " << std::fixed << std::setprecision(2) << stats.totalExecutionTime << "ms\n";
    report << "  Average Test Time: " << std::fixed << std::setprecision(2) << stats.averageExecutionTime << "ms\n\n";
    
    report << "Detailed Results:\n";
    for (const auto& result : testResults) {
        report << "  " << (result.passed ? "✅" : "❌") << " " << result.testName;
        if (!result.passed) {
            report << " - " << result.errorMessage;
        }
        report << " (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)\n";
    }
    
    return report.str();
}

void TestFramework::updateStats() {
    stats.totalTests = testResults.size();
    stats.passedTests = std::count_if(testResults.begin(), testResults.end(), 
                                    [](const TestResult& r) { return r.passed; });
    stats.failedTests = stats.totalTests - stats.passedTests;
    
    if (!testResults.empty()) {
        stats.totalExecutionTime = std::accumulate(testResults.begin(), testResults.end(), 0.0,
                                                  [](double sum, const TestResult& r) { return sum + r.executionTimeMs; });
        stats.averageExecutionTime = stats.totalExecutionTime / testResults.size();
    } else {
        stats.totalExecutionTime = 0.0;
        stats.averageExecutionTime = 0.0;
    }
}
