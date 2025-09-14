#include "validation_suite.h"
#include "performance_monitor.h"
#include <iostream>
#include <string>

// Include component validations
#include "component_validations.cpp"

int main(int argc, char* argv[]) {
    std::cout << "==========================================" << std::endl;
    std::cout << "  BLOOMBERG TERMINAL VALIDATION SUITE" << std::endl;
    std::cout << "  Professional Gaming Analytics Platform" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    // Register all validation tests
    ComponentValidations::registerAllValidations();
    
    auto& validationSuite = ValidationSuite::getInstance();
    
    if (argc > 1) {
        std::string componentName = argv[1];
        std::cout << "🔍 Running validations for component: " << componentName << std::endl << std::endl;
        
        auto results = validationSuite.runComponentValidations(componentName);
        
        if (results.empty()) {
            std::cout << "❌ No validation suite found for component: " << componentName << std::endl;
            std::cout << "Available components:" << std::endl;
            std::cout << "  • PerformanceMonitor" << std::endl;
            std::cout << "  • CudaSupport" << std::endl;
            std::cout << "  • AdvancedOCR" << std::endl;
            std::cout << "  • GameAnalytics" << std::endl;
            std::cout << "  • ScreenCapture" << std::endl;
            std::cout << "  • ThreadManager" << std::endl;
            std::cout << "  • SystemIntegration" << std::endl;
            return 1;
        }
    } else {
        std::cout << "🔍 Running comprehensive validation suite..." << std::endl << std::endl;
        
        auto results = validationSuite.runAllValidations();
    }
    
    // Generate and display comprehensive report
    std::cout << validationSuite.generateValidationReport() << std::endl;
    
    auto stats = validationSuite.getStats();
    
    std::cout << "==========================================" << std::endl;
    std::cout << "           VALIDATION SUMMARY" << std::endl;
    std::cout << "==========================================" << std::endl;
    
    if (stats.criticalFailures == 0) {
        std::cout << "🎉 ALL CRITICAL VALIDATIONS PASSED!" << std::endl;
        std::cout << "🚀 System is PRODUCTION READY" << std::endl;
        std::cout << "📊 " << stats.passedTests << "/" << stats.totalTests << " tests passed" << std::endl;
        
        if (stats.warnings > 0) {
            std::cout << "⚠️  " << stats.warnings << " warnings detected (non-critical)" << std::endl;
        }
        
        std::cout << "✅ Bloomberg Terminal validated successfully!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ CRITICAL VALIDATION FAILURES DETECTED!" << std::endl;
        std::cout << "🔴 " << stats.criticalFailures << " critical failures found" << std::endl;
        std::cout << "⚠️  System is NOT production ready" << std::endl;
        std::cout << "📊 " << stats.passedTests << "/" << stats.totalTests << " tests passed" << std::endl;
        
        std::cout << "\n🔧 REQUIRED ACTIONS:" << std::endl;
        std::cout << "  1. Fix all critical failures before deployment" << std::endl;
        std::cout << "  2. Review validation report for detailed issues" << std::endl;
        std::cout << "  3. Re-run validation suite after fixes" << std::endl;
        
        return 1;
    }
}
