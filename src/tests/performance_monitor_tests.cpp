#include "../test_framework.h"
#include "../performance_monitor.h"
#include <thread>
#include <chrono>

namespace PerformanceMonitorTests {
    
    TestFramework::TestResult testSingletonInstance() {
        auto& instance1 = PerformanceMonitor::getInstance();
        auto& instance2 = PerformanceMonitor::getInstance();
        
        ASSERT_NOT_NULL(&instance1, "Instance should not be null");
        ASSERT_TRUE(&instance1 == &instance2, "Should return same singleton instance");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testBasicTiming() {
        auto& monitor = PerformanceMonitor::getInstance();
        monitor.reset(); // Clear any previous data
        
        monitor.startTimer("test_operation");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        monitor.endTimer("test_operation");
        
        auto metric = monitor.getMetric("test_operation");
        ASSERT_TRUE(metric.totalCalls == 1, "Should have 1 call recorded");
        ASSERT_TRUE(metric.averageTime >= 9.0, "Average time should be at least 9ms");
        ASSERT_TRUE(metric.averageTime <= 20.0, "Average time should be reasonable");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testMultipleTimings() {
        auto& monitor = PerformanceMonitor::getInstance();
        monitor.reset();
        
        // Run the same operation multiple times
        for (int i = 0; i < 5; ++i) {
            monitor.startTimer("multi_test");
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            monitor.endTimer("multi_test");
        }
        
        auto metric = monitor.getMetric("multi_test");
        ASSERT_TRUE(metric.totalCalls == 5, "Should have 5 calls recorded");
        ASSERT_TRUE(metric.averageTime >= 4.0, "Average time should be reasonable");
        ASSERT_TRUE(metric.minTime <= metric.maxTime, "Min should be <= Max");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testPerformanceTargets() {
        auto& monitor = PerformanceMonitor::getInstance();
        monitor.reset();
        
        // Test fast operation (should meet target)
        monitor.startTimer("fast_op");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        monitor.endTimer("fast_op");
        
        // Test slow operation (should exceed target)
        monitor.startTimer("slow_op");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        monitor.endTimer("slow_op");
        
        ASSERT_TRUE(monitor.meetsTarget("fast_op", 50.0), "Fast operation should meet target");
        ASSERT_FALSE(monitor.meetsTarget("slow_op", 50.0), "Slow operation should exceed target");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testScopedTimer() {
        auto& monitor = PerformanceMonitor::getInstance();
        monitor.reset();
        
        {
            ScopedTimer timer("scoped_test");
            std::this_thread::sleep_for(std::chrono::milliseconds(15));
        }
        
        auto metric = monitor.getMetric("scoped_test");
        ASSERT_TRUE(metric.totalCalls == 1, "Scoped timer should record 1 call");
        ASSERT_TRUE(metric.averageTime >= 14.0, "Scoped timer should record correct time");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testResetFunctionality() {
        auto& monitor = PerformanceMonitor::getInstance();
        
        // Add some data
        monitor.startTimer("reset_test");
        monitor.endTimer("reset_test");
        
        auto metric = monitor.getMetric("reset_test");
        ASSERT_TRUE(metric.totalCalls == 1, "Should have data before reset");
        
        monitor.reset();
        
        auto emptyMetric = monitor.getMetric("reset_test");
        ASSERT_TRUE(emptyMetric.totalCalls == 0, "Should have no data after reset");
        
        TEST_PASS();
    }
    
    void registerTests() {
        auto& framework = TestFramework::getInstance();
        
        framework.addTest("PerformanceMonitor", "Singleton Instance", testSingletonInstance);
        framework.addTest("PerformanceMonitor", "Basic Timing", testBasicTiming);
        framework.addTest("PerformanceMonitor", "Multiple Timings", testMultipleTimings);
        framework.addTest("PerformanceMonitor", "Performance Targets", testPerformanceTargets);
        framework.addTest("PerformanceMonitor", "Scoped Timer", testScopedTimer);
        framework.addTest("PerformanceMonitor", "Reset Functionality", testResetFunctionality);
    }
}
