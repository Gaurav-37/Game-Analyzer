#include "validation_suite.h"
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <sstream>

ValidationSuite& ValidationSuite::getInstance() {
    static ValidationSuite instance;
    return instance;
}

void ValidationSuite::addValidation(const std::string& component, const std::string& testName, 
                                   std::function<ValidationResult()> test, const std::string& severity) {
    componentTests[component].push_back([testName, test, severity]() {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = test();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0;
        
        result.testName = testName;
        result.executionTimeMs = duration;
        if (result.severity == "INFO") {
            result.severity = severity;
        }
        return result;
    });
    
    testSeverities[component + ":" + testName] = severity;
}

std::vector<ValidationSuite::ValidationResult> ValidationSuite::runAllValidations() {
    validationResults.clear();
    
    for (auto& [componentName, tests] : componentTests) {
        std::cout << "🔍 Validating Component: " << componentName << std::endl;
        
        ComponentValidation componentValidation(componentName);
        
        for (auto& test : tests) {
            auto result = test();
            result.component = componentName;
            validationResults.push_back(result);
            componentValidation.results.push_back(result);
            
            std::string status = result.passed ? "✅" : "❌";
            std::string severityIcon = "";
            if (result.severity == "CRITICAL") severityIcon = "🔴";
            else if (result.severity == "WARNING") severityIcon = "🟡";
            else severityIcon = "🔵";
            
            std::cout << "  " << status << " " << severityIcon << " " << result.testName;
            if (!result.passed) {
                std::cout << " - " << result.errorMessage;
            }
            std::cout << " (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)" << std::endl;
            
            if (!result.passed && result.severity == "CRITICAL") {
                componentValidation.overallPassed = false;
            }
            
            componentValidation.totalTime += result.executionTimeMs;
        }
        
        stats.componentResults[componentName] = componentValidation;
        std::cout << "  Component " << componentName << ": " << (componentValidation.overallPassed ? "✅ PASSED" : "❌ FAILED") << std::endl << std::endl;
    }
    
    updateStats();
    return validationResults;
}

std::vector<ValidationSuite::ValidationResult> ValidationSuite::runComponentValidations(const std::string& componentName) {
    validationResults.clear();
    
    auto it = componentTests.find(componentName);
    if (it != componentTests.end()) {
        std::cout << "🔍 Validating Component: " << componentName << std::endl;
        
        ComponentValidation componentValidation(componentName);
        
        for (auto& test : it->second) {
            auto result = test();
            result.component = componentName;
            validationResults.push_back(result);
            componentValidation.results.push_back(result);
            
            std::string status = result.passed ? "✅" : "❌";
            std::string severityIcon = "";
            if (result.severity == "CRITICAL") severityIcon = "🔴";
            else if (result.severity == "WARNING") severityIcon = "🟡";
            else severityIcon = "🔵";
            
            std::cout << "  " << status << " " << severityIcon << " " << result.testName;
            if (!result.passed) {
                std::cout << " - " << result.errorMessage;
            }
            std::cout << " (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)" << std::endl;
            
            if (!result.passed && result.severity == "CRITICAL") {
                componentValidation.overallPassed = false;
            }
            
            componentValidation.totalTime += result.executionTimeMs;
        }
        
        stats.componentResults[componentName] = componentValidation;
        std::cout << "  Component " << componentName << ": " << (componentValidation.overallPassed ? "✅ PASSED" : "❌ FAILED") << std::endl << std::endl;
    }
    
    updateStats();
    return validationResults;
}

ValidationSuite::ValidationStats ValidationSuite::getStats() const {
    return stats;
}

std::string ValidationSuite::generateValidationReport() const {
    std::ostringstream report;
    
    report << "=== BLOOMBERG TERMINAL VALIDATION REPORT ===\n\n";
    
    // Executive Summary
    report << "📊 EXECUTIVE SUMMARY:\n";
    report << "  Total Tests: " << stats.totalTests << "\n";
    report << "  Passed: " << stats.passedTests << " (" << std::fixed << std::setprecision(1) 
           << (stats.totalTests > 0 ? (double)stats.passedTests / stats.totalTests * 100.0 : 0.0) << "%)\n";
    report << "  Failed: " << stats.failedTests << "\n";
    report << "  Critical Failures: " << stats.criticalFailures << "\n";
    report << "  Warnings: " << stats.warnings << "\n\n";
    
    // Production Readiness
    bool productionReady = isProductionReady();
    report << "🚀 PRODUCTION READINESS: ";
    if (productionReady) {
        report << "✅ READY FOR DEPLOYMENT\n";
    } else {
        report << "❌ NOT READY - CRITICAL ISSUES FOUND\n";
    }
    report << "\n";
    
    // Performance Summary
    report << "⚡ PERFORMANCE SUMMARY:\n";
    report << "  Total Execution Time: " << std::fixed << std::setprecision(2) << stats.totalExecutionTime << "ms\n";
    report << "  Average Test Time: " << std::fixed << std::setprecision(2) << stats.averageExecutionTime << "ms\n\n";
    
    // Component Results
    report << "🔧 COMPONENT VALIDATION RESULTS:\n";
    for (const auto& [componentName, componentResult] : stats.componentResults) {
        report << "  📦 " << componentName << ":\n";
        report << "    Status: " << (componentResult.overallPassed ? "✅ PASSED" : "❌ FAILED") << "\n";
        report << "    Tests: " << componentResult.results.size() << "\n";
        report << "    Total Time: " << std::fixed << std::setprecision(2) << componentResult.totalTime << "ms\n";
        
        // Show critical failures
        for (const auto& result : componentResult.results) {
            if (!result.passed && result.severity == "CRITICAL") {
                report << "    🔴 CRITICAL: " << result.testName << " - " << result.errorMessage << "\n";
            }
        }
        report << "\n";
    }
    
    // Detailed Results
    report << "📋 DETAILED VALIDATION RESULTS:\n";
    for (const auto& result : validationResults) {
        std::string status = result.passed ? "✅" : "❌";
        std::string severityIcon = "";
        if (result.severity == "CRITICAL") severityIcon = "🔴";
        else if (result.severity == "WARNING") severityIcon = "🟡";
        else severityIcon = "🔵";
        
        report << "  " << status << " " << severityIcon << " [" << result.component << "] " << result.testName;
        if (!result.passed) {
            report << " - " << result.errorMessage;
        }
        report << " (" << std::fixed << std::setprecision(2) << result.executionTimeMs << "ms)\n";
    }
    
    return report.str();
}

bool ValidationSuite::isProductionReady() const {
    return stats.criticalFailures == 0;
}

void ValidationSuite::updateStats() {
    stats.totalTests = validationResults.size();
    stats.passedTests = std::count_if(validationResults.begin(), validationResults.end(), 
                                    [](const ValidationResult& r) { return r.passed; });
    stats.failedTests = stats.totalTests - stats.passedTests;
    stats.criticalFailures = std::count_if(validationResults.begin(), validationResults.end(), 
                                         [](const ValidationResult& r) { return !r.passed && r.severity == "CRITICAL"; });
    stats.warnings = std::count_if(validationResults.begin(), validationResults.end(), 
                                 [](const ValidationResult& r) { return !r.passed && r.severity == "WARNING"; });
    
    if (!validationResults.empty()) {
        stats.totalExecutionTime = std::accumulate(validationResults.begin(), validationResults.end(), 0.0,
                                                  [](double sum, const ValidationResult& r) { return sum + r.executionTimeMs; });
        stats.averageExecutionTime = stats.totalExecutionTime / validationResults.size();
    } else {
        stats.totalExecutionTime = 0.0;
        stats.averageExecutionTime = 0.0;
    }
}
