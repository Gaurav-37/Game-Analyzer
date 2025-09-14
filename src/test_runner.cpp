#include "test_framework.h"
#include "unit_tests.cpp"
#include <iostream>

// Forward declarations
namespace BloombergTerminalTests {
    void registerAllBenchmarks();
}

int main(int argc, char* argv[]) {
    using namespace BloombergTerminalTests;
    
    std::cout << "==========================================" << std::endl;
    std::cout << "  BLOOMBERG TERMINAL TEST SUITE" << std::endl;
    std::cout << "  Enterprise Gaming Analytics Platform" << std::endl;
    std::cout << "  MinGW64 Build System" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    // Register all tests and benchmarks
    registerAllTests();
    registerAllBenchmarks();
    
    auto& testFramework = BloombergTestFramework::getInstance();
    
    if (argc > 1) {
        std::string command = argv[1];
        
        if (command == "--component" && argc > 2) {
            std::string componentName = argv[2];
            std::cout << "🔍 Running tests for component: " << componentName << std::endl << std::endl;
            
            testFramework.runComponentTests(componentName);
        } else if (command == "--benchmarks") {
            std::cout << "📊 Running performance benchmarks..." << std::endl << std::endl;
            
            testFramework.runBenchmarks();
        } else if (command == "--help") {
            std::cout << "Bloomberg Terminal Test Suite Usage:" << std::endl;
            std::cout << "  BloombergTestSuite.exe                    - Run all tests" << std::endl;
            std::cout << "  BloombergTestSuite.exe --component <name> - Run component tests" << std::endl;
            std::cout << "  BloombergTestSuite.exe --benchmarks      - Run performance benchmarks" << std::endl;
            std::cout << "  BloombergTestSuite.exe --help            - Show this help" << std::endl;
            std::cout << std::endl;
            std::cout << "Available components:" << std::endl;
            std::cout << "  • AdvancedOCR - Text recognition system" << std::endl;
            std::cout << "  • OptimizedScreenCapture - Frame capture system" << std::endl;
            std::cout << "  • GameAnalytics - Event detection system" << std::endl;
            std::cout << "  • ThreadManager - Concurrency management" << std::endl;
            std::cout << "  • PerformanceMonitor - Performance tracking" << std::endl;
            std::cout << "  • CudaSupport - GPU acceleration support" << std::endl;
            std::cout << "  • SystemIntegration - Cross-component testing" << std::endl;
            std::cout << std::endl;
            std::cout << "Performance Targets (from prompt.md):" << std::endl;
            std::cout << "  • Processing latency: <" << BloombergTestFramework::TARGET_PROCESSING_LATENCY_MS << "ms" << std::endl;
            std::cout << "  • Memory usage: <" << BloombergTestFramework::TARGET_MEMORY_USAGE_MB << "MB" << std::endl;
            std::cout << "  • CPU usage: <" << BloombergTestFramework::TARGET_CPU_USAGE_PERCENT << "%" << std::endl;
            std::cout << "  • OCR accuracy: >" << BloombergTestFramework::TARGET_OCR_ACCURACY_PERCENT << "%" << std::endl;
            std::cout << "  • Capture rate: " << BloombergTestFramework::TARGET_CAPTURE_RATE_FPS << " FPS" << std::endl;
            std::cout << "  • Startup time: <" << BloombergTestFramework::TARGET_STARTUP_TIME_MS << "ms" << std::endl;
            return 0;
        } else {
            std::cout << "❌ Invalid arguments. Use --help for usage information." << std::endl;
            return 1;
        }
    } else {
        std::cout << "🔍 Running comprehensive test suite..." << std::endl << std::endl;
        
        testFramework.runAllTests();
    }
    
    // Generate comprehensive report
    testFramework.generateReport();
    
    auto stats = testFramework.getStatistics();
    
    std::cout << "==========================================" << std::endl;
    std::cout << "        FINAL TEST SUMMARY" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    if (stats.failedTests == 0) {
        std::cout << "🎉 ALL TESTS PASSED!" << std::endl;
        std::cout << "🚀 Bloomberg Terminal is PRODUCTION READY" << std::endl;
        std::cout << "✅ Enterprise validation completed successfully" << std::endl;
        std::cout << "📊 " << stats.passedTests << "/" << stats.totalTests << " tests passed" << std::endl;
        std::cout << "📈 Pass Rate: " << std::fixed << std::setprecision(1) << stats.getPassRate() << "%" << std::endl;
        std::cout << "⏱️  Average Test Time: " << std::fixed << std::setprecision(2) << stats.getAverageExecutionTime() << "ms" << std::endl;
        std::cout << "🎯 System approved for enterprise deployment" << std::endl;
        std::cout << "📄 Detailed report saved to: bloomberg_test_report.txt" << std::endl;
        
        return 0;
    } else {
        std::cout << "❌ TEST FAILURES DETECTED!" << std::endl;
        std::cout << "🔴 " << stats.failedTests << " tests failed" << std::endl;
        std::cout << "📊 Pass Rate: " << std::fixed << std::setprecision(1) << stats.getPassRate() << "%" << std::endl;
        std::cout << "⚠️  System is NOT production ready" << std::endl;
        
        std::cout << "\n🔧 REQUIRED ACTIONS:" << std::endl;
        std::cout << "  1. Fix all test failures before deployment" << std::endl;
        std::cout << "  2. Review test report for detailed issues" << std::endl;
        std::cout << "  3. Re-run test suite after fixes" << std::endl;
        std::cout << "  4. Ensure 100% test pass rate" << std::endl;
        
        std::cout << "\n📄 Detailed report saved to: bloomberg_test_report.txt" << std::endl;
        
        return 1;
    }
}
