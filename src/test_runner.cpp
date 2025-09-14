#include "test_framework.h"
#include "performance_monitor.h"
#include "cuda_support.h"
#include "advanced_ocr.h"
#include "game_analytics.h"
#include "optimized_screen_capture.h"
#include "thread_manager.h"

// Include all test suites
#include "tests/performance_monitor_tests.cpp"
#include "tests/cuda_support_tests.cpp"
#include "tests/ocr_tests.cpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::cout << "=== BLOOMBERG TERMINAL TEST SUITE ===" << std::endl;
    std::cout << "Professional Gaming Analytics Platform" << std::endl;
    std::cout << "=====================================" << std::endl << std::endl;
    
    // Register all test suites
    PerformanceMonitorTests::registerTests();
    CudaSupportTests::registerTests();
    OCRTests::registerTests();
    
    auto& framework = TestFramework::getInstance();
    
    if (argc > 1) {
        std::string suiteName = argv[1];
        std::cout << "Running specific test suite: " << suiteName << std::endl << std::endl;
        
        auto results = framework.runSuite(suiteName);
        
        if (results.empty()) {
            std::cout << "No test suite found with name: " << suiteName << std::endl;
            std::cout << "Available suites:" << std::endl;
            std::cout << "  - PerformanceMonitor" << std::endl;
            std::cout << "  - CudaSupport" << std::endl;
            std::cout << "  - OCR" << std::endl;
            return 1;
        }
    } else {
        std::cout << "Running all test suites..." << std::endl << std::endl;
        
        auto results = framework.runAllTests();
    }
    
    // Generate and display report
    std::cout << std::endl << framework.generateReport() << std::endl;
    
    auto stats = framework.getStats();
    
    std::cout << "=== TEST EXECUTION COMPLETE ===" << std::endl;
    
    if (stats.failedTests == 0) {
        std::cout << "🎉 All tests passed! System is ready for production." << std::endl;
        return 0;
    } else {
        std::cout << "⚠️  " << stats.failedTests << " test(s) failed. Please review and fix issues." << std::endl;
        return 1;
    }
}
