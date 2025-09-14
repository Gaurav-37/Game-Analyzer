#pragma once

#include <windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <functional>

// Advanced OCR System with Multiple Backends
class AdvancedOCR {
public:
    enum class OCRBackend {
        TESSERACT,
        OPENCV_EAST
    };

    struct TextRegion {
        cv::Rect region;
        std::string text;
        float confidence;
        std::string detectedType;
        cv::Scalar color;
        
        TextRegion() : confidence(0.0f) {}
        TextRegion(const cv::Rect& r, const std::string& t, float conf, const std::string& type = "unknown")
            : region(r), text(t), confidence(conf), detectedType(type) {}
    };

    struct GameUITemplate {
        cv::Mat templateImage;
        cv::Rect region;
        std::string expectedTextPattern;
        std::string elementType;
        float threshold;
    };

    AdvancedOCR();
    ~AdvancedOCR();

    // Initialization
    bool initialize(OCRBackend backend = OCRBackend::TESSERACT);
    bool initializeTesseract();
    bool initializeOpenCV();
    void cleanup();

    // Main OCR functions
    std::vector<TextRegion> detectText(const cv::Mat& frame, const std::string& gameName = "");

    // Backend-specific processing
    std::vector<TextRegion> processWithTesseract(const cv::Mat& frame);
    std::vector<TextRegion> processWithOpenCV(const cv::Mat& frame);

    // Game-specific detection
    std::vector<TextRegion> detectGameUI(const cv::Mat& frame, const std::string& gameName);
    std::vector<TextRegion> detectHealthValues(const cv::Mat& frame);
    std::vector<TextRegion> detectScoreValues(const cv::Mat& frame);
    std::vector<TextRegion> detectAmmoValues(const cv::Mat& frame);
    std::vector<TextRegion> detectTimeValues(const cv::Mat& frame);

    // Configuration
    void setBackend(OCRBackend backend);
    void enableCaching(bool enable);
    void setConfidenceThreshold(float threshold);

    // Utility functions
    cv::Mat preprocessFrame(const cv::Mat& frame);
    std::vector<cv::Rect> detectTextRegions(const cv::Mat& frame);

private:
    // Backend management
    OCRBackend currentBackend;
    bool initialized;
    bool useCaching;
    float confidenceThreshold;

    // Windows components (removed for compatibility)

    // Tesseract components
    std::unique_ptr<tesseract::TessBaseAPI> tesseractAPI;

    // OpenCV components
    cv::dnn::Net eastNet;

    // Caching
    std::vector<TextRegion> cachedResults;
    std::chrono::milliseconds lastProcessTime;

    // Game templates
    std::vector<GameUITemplate> gameTemplates;

    // Thread safety
    std::mutex processingMutex;

    // Helper functions
    void loadGameTemplates(const std::string& gameName);
    void loadDefaultGameTemplates();
    std::vector<TextRegion> matchGameTemplates(const cv::Mat& frame);
    float matchTemplate(const cv::Mat& frame, const GameUITemplate& template_);
    bool isGameUIText(const std::string& text);
    std::string classifyTextType(const std::string& text);
    cv::Scalar detectTextColor(const cv::Mat& frame, const cv::Rect& region);
    bool isHealthValue(const std::string& text);
    bool isScoreValue(const std::string& text);
    bool isAmmoValue(const std::string& text);
    bool isTimeValue(const std::string& text);

    // Windows API helpers (removed for compatibility)
};

// Frame Cache for OCR Results
class FrameCache {
public:
    struct CachedFrame {
        cv::Mat frame;
        std::string gameName;
        std::vector<AdvancedOCR::TextRegion> results;
        std::chrono::milliseconds timestamp;
    };

    FrameCache(size_t maxSize = 100, int64_t timeout = 5000);
    FrameCache(const FrameCache&) = delete;
    FrameCache& operator=(const FrameCache&) = delete;
    FrameCache(FrameCache&& other) noexcept;
    FrameCache& operator=(FrameCache&& other) noexcept;
    ~FrameCache();

    bool findCachedResult(const cv::Mat& frame, const std::string& gameName,
                         std::vector<AdvancedOCR::TextRegion>& results);
    void cacheResult(const cv::Mat& frame, const std::string& gameName,
                    const std::vector<AdvancedOCR::TextRegion>& results);
    void clearCache();
    float getHitRate() const;

private:
    std::vector<CachedFrame> cache;
    size_t maxCacheSize;
    int64_t timeoutMs;
    mutable std::mutex cacheMutex;
    int cacheHits;
    int cacheMisses;

    void cleanupExpiredEntries();
    bool framesAreSimilar(const cv::Mat& frame1, const cv::Mat& frame2);
    double calculateFrameSimilarity(const cv::Mat& frame1, const cv::Mat& frame2);
};
