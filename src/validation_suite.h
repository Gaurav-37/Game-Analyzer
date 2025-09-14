#pragma once

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <iostream>
#include <memory>
#include <map>
#include <atomic>

class ValidationSuite {
public:
    struct ValidationResult {
        std::string testName;
        std::string component;
        bool passed;
        std::string errorMessage;
        double executionTimeMs;
        std::string severity; // "CRITICAL", "WARNING", "INFO"
        
        ValidationResult() : passed(false), executionTimeMs(0.0), severity("INFO") {}
        ValidationResult(const std::string& name, const std::string& comp, bool pass, 
                        const std::string& error = "", double time = 0.0, const std::string& sev = "INFO")
            : testName(name), component(comp), passed(pass), errorMessage(error), executionTimeMs(time), severity(sev) {}
    };
    
    struct ComponentValidation {
        std::string componentName;
        std::vector<ValidationResult> results;
        bool overallPassed;
        double totalTime;
        
        ComponentValidation(const std::string& name) : componentName(name), overallPassed(true), totalTime(0.0) {}
    };
    
    static ValidationSuite& getInstance();
    
    // Register a validation test
    void addValidation(const std::string& component, const std::string& testName, 
                      std::function<ValidationResult()> test, const std::string& severity = "INFO");
    
    // Run all validations
    std::vector<ValidationResult> runAllValidations();
    
    // Run specific component validations
    std::vector<ValidationResult> runComponentValidations(const std::string& componentName);
    
    // Get validation statistics
    struct ValidationStats {
        int totalTests;
        int passedTests;
        int failedTests;
        int criticalFailures;
        int warnings;
        double totalExecutionTime;
        double averageExecutionTime;
        std::map<std::string, ComponentValidation> componentResults;
    };
    
    ValidationStats getStats() const;
    
    // Generate comprehensive validation report
    std::string generateValidationReport() const;
    
    // Check if system is production ready
    bool isProductionReady() const;

private:
    ValidationSuite() = default;
    
    std::map<std::string, std::vector<std::function<ValidationResult()>>> componentTests;
    std::map<std::string, std::string> testSeverities;
    std::vector<ValidationResult> validationResults;
    ValidationStats stats;
    
    void updateStats();
};

// Validation macros for easy test creation
#define VALIDATE_COMPONENT(component, name, severity) ValidationSuite::getInstance().addValidation(component, name, [&]() -> ValidationSuite::ValidationResult
#define VALIDATE_CRITICAL(component, name) VALIDATE_COMPONENT(component, name, "CRITICAL")
#define VALIDATE_WARNING(component, name) VALIDATE_COMPONENT(component, name, "WARNING")
#define VALIDATE_INFO(component, name) VALIDATE_COMPONENT(component, name, "INFO")

#define ASSERT_VALIDATION(condition, message, severity) \
    if (!(condition)) { \
        return ValidationSuite::ValidationResult(__FUNCTION__, "", false, message, 0.0, severity); \
    }
#define ASSERT_CRITICAL(condition, message) ASSERT_VALIDATION(condition, message, "CRITICAL")
#define ASSERT_WARNING(condition, message) ASSERT_VALIDATION(condition, message, "WARNING")
#define ASSERT_INFO(condition, message) ASSERT_VALIDATION(condition, message, "INFO")

#define VALIDATION_PASS(severity) \
    return ValidationSuite::ValidationResult(__FUNCTION__, "", true, "", 0.0, severity);

// Performance validation macros
#define PERFORMANCE_VALIDATE(name, targetMs, severity) \
    auto start = std::chrono::high_resolution_clock::now(); \
    auto result = [&]() -> ValidationSuite::ValidationResult { \
        name; \
        return ValidationSuite::ValidationResult(__FUNCTION__, "", true, "", 0.0, severity); \
    }(); \
    auto end = std::chrono::high_resolution_clock::now(); \
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000.0; \
    result.executionTimeMs = duration; \
    if (duration > targetMs) { \
        result.passed = false; \
        result.errorMessage = "Performance validation failed: " + std::to_string(duration) + "ms > " + std::to_string(targetMs) + "ms target"; \
    } \
    return result;
