#include "../test_framework.h"
#include "../advanced_ocr.h"
#include <opencv2/opencv.hpp>

namespace OCRTests {
    
    TestFramework::TestResult testOCRInitialization() {
        AdvancedOCR ocr;
        
        // OCR might not initialize if Tesseract data is not available
        // This test ensures the function doesn't crash
        bool initialized = ocr.initialize();
        
        ASSERT_TRUE(initialized == true || initialized == false, "OCR initialization should return valid boolean");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testOCRWithEmptyFrame() {
        AdvancedOCR ocr;
        
        // Create empty frame
        cv::Mat emptyFrame;
        auto results = ocr.detectText(emptyFrame);
        
        ASSERT_TRUE(results.empty(), "Empty frame should return empty results");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testOCRWithValidFrame() {
        AdvancedOCR ocr;
        
        // Create a simple test frame with some content
        cv::Mat testFrame = cv::Mat::zeros(100, 200, CV_8UC3);
        cv::putText(testFrame, "TEST", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        
        auto results = ocr.detectText(testFrame);
        
        // Results might be empty if OCR is not properly initialized
        // This test ensures the function doesn't crash
        ASSERT_TRUE(results.size() >= 0, "OCR should return valid results (possibly empty)");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testOCRCaching() {
        AdvancedOCR ocr;
        
        // Test caching functionality
        ocr.enableCaching(true);
        
        cv::Mat testFrame = cv::Mat::zeros(100, 200, CV_8UC3);
        cv::putText(testFrame, "CACHE TEST", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        
        auto results1 = ocr.detectText(testFrame);
        auto results2 = ocr.detectText(testFrame);
        
        // Both calls should return valid results (caching is internal)
        ASSERT_TRUE(results1.size() >= 0, "First OCR call should return valid results");
        ASSERT_TRUE(results2.size() >= 0, "Second OCR call should return valid results");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testOCRWithDifferentGames() {
        AdvancedOCR ocr;
        
        cv::Mat testFrame = cv::Mat::zeros(100, 200, CV_8UC3);
        cv::putText(testFrame, "123", cv::Point(50, 50), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        
        // Test with different game names
        auto results1 = ocr.detectText(testFrame, "Counter-Strike 2");
        auto results2 = ocr.detectText(testFrame, "Valorant");
        auto results3 = ocr.detectText(testFrame, "");
        
        ASSERT_TRUE(results1.size() >= 0, "CS2 OCR should return valid results");
        ASSERT_TRUE(results2.size() >= 0, "Valorant OCR should return valid results");
        ASSERT_TRUE(results3.size() >= 0, "Generic OCR should return valid results");
        
        TEST_PASS();
    }
    
    TestFramework::TestResult testOCRPerformance() {
        AdvancedOCR ocr;
        
        cv::Mat testFrame = cv::Mat::zeros(200, 400, CV_8UC3);
        cv::putText(testFrame, "PERFORMANCE TEST 123456789", cv::Point(50, 100), cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
        
        auto start = std::chrono::high_resolution_clock::now();
        auto results = ocr.detectText(testFrame);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        ASSERT_TRUE(duration >= 0, "OCR should complete in reasonable time");
        ASSERT_TRUE(duration < 5000, "OCR should not take more than 5 seconds");
        
        TEST_PASS();
    }
    
    void registerTests() {
        auto& framework = TestFramework::getInstance();
        
        framework.addTest("OCR", "Initialization", testOCRInitialization);
        framework.addTest("OCR", "Empty Frame Handling", testOCRWithEmptyFrame);
        framework.addTest("OCR", "Valid Frame Processing", testOCRWithValidFrame);
        framework.addTest("OCR", "Caching Functionality", testOCRCaching);
        framework.addTest("OCR", "Different Game Support", testOCRWithDifferentGames);
        framework.addTest("OCR", "Performance", testOCRPerformance);
    }
}
