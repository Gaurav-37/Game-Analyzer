#pragma once
#include <chrono>
#include <future>
#include <atomic>
#include <memory>
#include <string>
#include <functional>
#include <iostream>
#include <iomanip>
#include <opencv2/opencv.hpp>
#include "advanced_ocr.h"
#include "optimized_screen_capture.h"
#include "game_analytics.h"
#include "thread_manager.h"

namespace BloombergTerminal {

// Safe initialization result
struct SafeInitResult {
    bool success;
    std::string errorMessage;
    double initializationTimeMs;
    
    SafeInitResult(bool s = false, const std::string& msg = "", double time = 0.0) 
        : success(s), errorMessage(msg), initializationTimeMs(time) {}
};

// Safe component initializer with timeout protection
template<typename ComponentType>
class SafeComponentInitializer {
private:
    std::unique_ptr<ComponentType> component_;
    SafeInitResult lastResult_;
    std::atomic<bool> isInitialized_;
    
public:
    SafeComponentInitializer() : isInitialized_(false) {}
    
    // Safe initialization with timeout
    SafeInitResult initializeSafely(std::function<bool(ComponentType&)> initFunc, 
                                   double timeoutMs = 5000.0) {
        if (isInitialized_) {
            return lastResult_;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // Create component
            component_ = std::make_unique<ComponentType>();
            
            // Run initialization with timeout
            auto future = std::async(std::launch::async, [this, initFunc]() -> bool {
                try {
                    return initFunc(*component_);
                } catch (...) {
                    return false;
                }
            });
            
            // Wait with timeout
            auto status = future.wait_for(std::chrono::milliseconds(static_cast<int>(timeoutMs)));
            
            auto end = std::chrono::high_resolution_clock::now();
            double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
            
            if (status == std::future_status::timeout) {
                lastResult_ = SafeInitResult(false, "Initialization timeout after " + std::to_string(timeoutMs) + "ms", elapsedMs);
                isInitialized_ = false;
                component_.reset();
                return lastResult_;
            }
            
            bool result = future.get();
            
            if (result) {
                lastResult_ = SafeInitResult(true, "Initialization successful", elapsedMs);
                isInitialized_ = true;
            } else {
                lastResult_ = SafeInitResult(false, "Initialization failed", elapsedMs);
                isInitialized_ = false;
                component_.reset();
            }
            
            return lastResult_;
            
        } catch (const std::exception& e) {
            auto end = std::chrono::high_resolution_clock::now();
            double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
            
            lastResult_ = SafeInitResult(false, std::string("Exception: ") + e.what(), elapsedMs);
            isInitialized_ = false;
            component_.reset();
            return lastResult_;
        } catch (...) {
            auto end = std::chrono::high_resolution_clock::now();
            double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
            
            lastResult_ = SafeInitResult(false, "Unknown exception during initialization", elapsedMs);
            isInitialized_ = false;
            component_.reset();
            return lastResult_;
        }
    }
    
    // Get component (only if initialized successfully)
    ComponentType* getComponent() {
        return isInitialized_ ? component_.get() : nullptr;
    }
    
    // Check if initialized
    bool isInitialized() const {
        return isInitialized_;
    }
    
    // Get last result
    const SafeInitResult& getLastResult() const {
        return lastResult_;
    }
    
    // Reset for re-initialization
    void reset() {
        isInitialized_ = false;
        component_.reset();
        lastResult_ = SafeInitResult();
    }
};

// Mock components for testing when initialization fails
class MockOCR {
public:
    bool initialize() { return true; }
    std::vector<std::string> detectText(const cv::Mat& frame) {
        return {"Mock OCR Result"};
    }
};

class MockScreenCapture {
public:
    struct FrameData {
        cv::Mat frame;
        int64_t timestamp;
        bool isGPU;
        
        FrameData() : timestamp(0), isGPU(false) {
            frame = cv::Mat::zeros(480, 640, CV_8UC3);
        }
    };
    
    bool initialize() { return true; }
    void captureFrame(FrameData& frameData) {
        frameData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

class MockGameAnalytics {
public:
    bool initialize() { return true; }
    std::vector<std::string> detectEvents(const cv::Mat& frame) {
        return {"Mock Event Detected"};
    }
};

class MockThreadManager {
public:
    bool initialize() { return true; }
    void submitTask(std::function<void()> task) {
        // Execute immediately in mock
        if (task) task();
    }
};

// Global safe initializers
class SafeComponentManager {
private:
    static SafeComponentManager* instance_;
    
    SafeComponentInitializer<AdvancedOCR> ocrInitializer_;
    SafeComponentInitializer<OptimizedScreenCapture> captureInitializer_;
    SafeComponentInitializer<GameEventDetector> analyticsInitializer_;
    SafeComponentInitializer<ThreadManager> threadInitializer_;
    
    // Mock components as fallbacks
    std::unique_ptr<MockOCR> mockOCR_;
    std::unique_ptr<MockScreenCapture> mockScreenCapture_;
    std::unique_ptr<MockGameAnalytics> mockGameAnalytics_;
    std::unique_ptr<MockThreadManager> mockThreadManager_;
    
    SafeComponentManager() {
        // Initialize mock components
        mockOCR_ = std::make_unique<MockOCR>();
        mockScreenCapture_ = std::make_unique<MockScreenCapture>();
        mockGameAnalytics_ = std::make_unique<MockGameAnalytics>();
        mockThreadManager_ = std::make_unique<MockThreadManager>();
    }
    
public:
    static SafeComponentManager& getInstance() {
        if (!instance_) {
            instance_ = new SafeComponentManager();
        }
        return *instance_;
    }
    
    // Initialize all components safely
    void initializeAllComponents() {
        std::cout << "🔧 Initializing Bloomberg Terminal components safely..." << std::endl;
        
        // Initialize OCR (real) with timeout
        auto ocrResult = ocrInitializer_.initializeSafely([](AdvancedOCR& ocr) {
            return ocr.initialize(AdvancedOCR::OCRBackend::TESSERACT);
        }, 5000.0);
        std::cout << "📝 OCR: " << (ocrResult.success ? "✅ Success" : "❌ Failed (" + ocrResult.errorMessage + ")")
                  << " (" << std::fixed << std::setprecision(1) << ocrResult.initializationTimeMs << "ms)" << std::endl;
        
        // Initialize Screen Capture (forced to mock to avoid DX issues in CI)
        auto captureResult = SafeInitResult(false, "Forced mock ScreenCapture for robust tests", 0.0);
        std::cout << "📷 Screen Capture: ❌ Failed (" << captureResult.errorMessage << ") (0.0ms)" << std::endl;
        
        // Initialize Game Analytics (forced to mock in robust tests)
        auto analyticsResult = SafeInitResult(false, "Forced mock GameAnalytics for robust tests", 0.0);
        std::cout << "🎮 Game Analytics: ❌ Failed (" << analyticsResult.errorMessage << ") (0.0ms)" << std::endl;
        
        // Initialize Thread Manager (forced to mock in robust tests)
        auto threadResult = SafeInitResult(false, "Forced mock ThreadManager for robust tests", 0.0);
        std::cout << "🧵 Thread Manager: ❌ Failed (" << threadResult.errorMessage << ") (0.0ms)" << std::endl;
        
        std::cout << std::endl;
    }
    
    // Get components (with fallback to mocks)
    AdvancedOCR* getOCR() {
        return ocrInitializer_.getComponent() ? ocrInitializer_.getComponent() : nullptr;
    }
    
    MockOCR* getMockOCR() {
        return mockOCR_.get();
    }
    
    OptimizedScreenCapture* getScreenCapture() {
        return captureInitializer_.getComponent() ? captureInitializer_.getComponent() : nullptr;
    }
    
    MockScreenCapture* getMockScreenCapture() {
        return mockScreenCapture_.get();
    }
    
    GameEventDetector* getGameAnalytics() {
        return analyticsInitializer_.getComponent() ? analyticsInitializer_.getComponent() : nullptr;
    }
    
    MockGameAnalytics* getMockGameAnalytics() {
        return mockGameAnalytics_.get();
    }
    
    ThreadManager* getThreadManager() {
        return threadInitializer_.getComponent() ? threadInitializer_.getComponent() : nullptr;
    }
    
    MockThreadManager* getMockThreadManager() {
        return mockThreadManager_.get();
    }
    
    // Check initialization status
    bool isOCRInitialized() const { return ocrInitializer_.isInitialized(); }
    bool isScreenCaptureInitialized() const { return captureInitializer_.isInitialized(); }
    bool isGameAnalyticsInitialized() const { return analyticsInitializer_.isInitialized(); }
    bool isThreadManagerInitialized() const { return threadInitializer_.isInitialized(); }
    
    // Get initialization results
    SafeInitResult getOCRResult() const { return ocrInitializer_.getLastResult(); }
    SafeInitResult getScreenCaptureResult() const { return captureInitializer_.getLastResult(); }
    SafeInitResult getGameAnalyticsResult() const { return analyticsInitializer_.getLastResult(); }
    SafeInitResult getThreadManagerResult() const { return threadInitializer_.getLastResult(); }
};

// Global instance
SafeComponentManager* SafeComponentManager::instance_ = nullptr;

} // namespace BloombergTerminal
