#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>
#include <memory>

class TestFramework {
public:
    struct TestResult {
        std::string testName;
        bool passed;
        std::string errorMessage;
        double executionTimeMs;
        
        TestResult() : passed(false), executionTimeMs(0.0) {}
        TestResult(const std::string& name, bool pass, const std::string& error = "", double time = 0.0)
            : testName(name), passed(pass), errorMessage(error), executionTimeMs(time) {}
    };
    
    struct TestSuite {
        std::string suiteName;
        std::vector<std::function<TestResult()>> tests;
        
        TestSuite(const std::string& name) : suiteName(name) {}
    };
    
    static TestFramework& getInstance();
    
    // Register a test
    void addTest(const std::string& suiteName, const std::string& testName, std::function<TestResult()> test);
    
    // Run all tests
    std::vector<TestResult> runAllTests();
    
    // Run specific suite
    std::vector<TestResult> runSuite(const std::string& suiteName);
    
    // Get test statistics
    struct TestStats {
        int totalTests;
        int passedTests;
        int failedTests;
        double totalExecutionTime;
        double averageExecutionTime;
    };
    
    TestStats getStats() const;
    
    // Generate test report
    std::string generateReport() const;

private:
    TestFramework() = default;
    
    std::map<std::string, TestSuite> testSuites;
    std::vector<TestResult> testResults;
    TestStats stats;
    
    void updateStats();
};

// Test macros for easy test creation
#define TEST_SUITE(name) TestFramework::getInstance().addTest
#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        return TestFramework::TestResult(__FUNCTION__, false, message); \
    }
#define ASSERT_FALSE(condition, message) \
    if (condition) { \
        return TestFramework::TestResult(__FUNCTION__, false, message); \
    }
#define ASSERT_EQUALS(expected, actual, message) \
    if ((expected) != (actual)) { \
        return TestFramework::TestResult(__FUNCTION__, false, \
            message + " Expected: " + std::to_string(expected) + ", Got: " + std::to_string(actual)); \
    }
#define ASSERT_NOT_NULL(ptr, message) \
    if (!(ptr)) { \
        return TestFramework::TestResult(__FUNCTION__, false, message); \
    }
#define TEST_PASS() \
    return TestFramework::TestResult(__FUNCTION__, true, "", 0.0);

// Performance test macros
#define PERFORMANCE_TEST(name, targetMs) \
    auto start = std::chrono::high_resolution_clock::now(); \
    auto result = [&]() -> TestFramework::TestResult { \
        name; \
        return TestFramework::TestResult(__FUNCTION__, true); \
    }(); \
    auto end = std::chrono::high_resolution_clock::now(); \
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0; \
    result.executionTimeMs = duration; \
    if (duration > targetMs) { \
        result.passed = false; \
        result.errorMessage = "Performance test failed: " + std::to_string(duration) + "ms > " + std::to_string(targetMs) + "ms target"; \
    } \
    return result;
