#include "advanced_ocr.h"
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <iostream>
#include <algorithm>
#include <regex>

// AdvancedOCR Implementation
AdvancedOCR::AdvancedOCR()
    : currentBackend(OCRBackend::TESSERACT), initialized(false),
      useCaching(true), confidenceThreshold(0.5f) {
    lastProcessTime = std::chrono::milliseconds(0);
}

AdvancedOCR::~AdvancedOCR() {
    cleanup();
}

bool AdvancedOCR::initialize(OCRBackend backend) {
    currentBackend = backend;
    
    switch (backend) {
        case OCRBackend::TESSERACT:
            return initializeTesseract();
        case OCRBackend::OPENCV_EAST:
            return initializeOpenCV();
        default:
            return false;
    }
}

bool AdvancedOCR::initializeTesseract() {
    try {
        tesseractAPI = std::make_unique<tesseract::TessBaseAPI>();
        
        // Initialize Tesseract with English language
        if (tesseractAPI->Init(nullptr, "eng")) {
            std::cerr << "Failed to initialize Tesseract" << std::endl;
            return false;
        }
        
        // Set OCR mode to single text block
        tesseractAPI->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
        
        initialized = true;
        loadDefaultGameTemplates();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Tesseract initialization error: " << e.what() << std::endl;
        return false;
    }
}

bool AdvancedOCR::initializeOpenCV() {
    try {
        // Load EAST text detector model (if available)
        // For now, we'll use basic OpenCV text detection
        initialized = true;
        loadDefaultGameTemplates();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "OpenCV initialization error: " << e.what() << std::endl;
        return false;
    }
}

void AdvancedOCR::cleanup() {
    std::lock_guard<std::mutex> lock(processingMutex);
    
    if (tesseractAPI) {
        tesseractAPI->End();
        tesseractAPI.reset();
    }
    
    initialized = false;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectText(const cv::Mat& frame, const std::string& gameName) {
    std::lock_guard<std::mutex> lock(processingMutex);
    
    if (!initialized) return {};
    
    // Check cache first
    if (useCaching && !cachedResults.empty()) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
        if (now - lastProcessTime < std::chrono::milliseconds(100)) { // 100ms cache
            return cachedResults;
        }
    }
    
    std::vector<TextRegion> results;
    
    // Process based on current backend
    switch (currentBackend) {
        case OCRBackend::TESSERACT:
            results = processWithTesseract(frame);
            break;
        case OCRBackend::OPENCV_EAST:
            results = processWithOpenCV(frame);
            break;
        default:
            return {};
    }
    
    // Apply game-specific detection if game name provided
    if (!gameName.empty()) {
        results = detectGameUI(frame, gameName);
    }
    
    // Cache results
    if (useCaching) {
        cachedResults = results;
        lastProcessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    }
    
    return results;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::processWithTesseract(const cv::Mat& frame) {
    std::vector<TextRegion> results;
    
    if (!tesseractAPI) return results;
    
    try {
        // Preprocess frame for better OCR
        cv::Mat processed = preprocessFrame(frame);
        
        // Set image for Tesseract
        tesseractAPI->SetImage(processed.data, processed.cols, processed.rows,
                              processed.channels(), processed.step);
        
        // Get text regions
        std::vector<cv::Rect> textRegions = detectTextRegions(processed);
        
        // Process each region
        for (const auto& region : textRegions) {
            // Extract region
            cv::Mat roi = processed(region);
            
            // Set ROI for Tesseract
            tesseractAPI->SetRectangle(region.x, region.y, region.width, region.height);
            
            // Get text
            char* text = tesseractAPI->GetUTF8Text();
            if (text) {
                std::string textStr(text);
                float confidence = tesseractAPI->MeanTextConf() / 100.0f;
                
                if (confidence >= confidenceThreshold) {
                    TextRegion textRegion(region, textStr, confidence);
                    textRegion.detectedType = classifyTextType(textStr);
                    textRegion.color = detectTextColor(frame, region);
                    results.push_back(textRegion);
                }
                
                delete[] text;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Tesseract processing error: " << e.what() << std::endl;
    }
    
    return results;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::processWithOpenCV(const cv::Mat& frame) {
    std::vector<TextRegion> results;
    
    try {
        // Preprocess frame
        cv::Mat processed = preprocessFrame(frame);
        
        // Detect text regions using OpenCV
        std::vector<cv::Rect> textRegions = detectTextRegions(processed);
        
        // Process each region
        for (const auto& region : textRegions) {
            // For now, just mark as detected text
            TextRegion textRegion(region, "TEXT", 0.5f);
            textRegion.detectedType = "unknown";
            textRegion.color = detectTextColor(frame, region);
            results.push_back(textRegion);
        }
    } catch (const std::exception& e) {
        std::cerr << "OpenCV processing error: " << e.what() << std::endl;
    }
    
    return results;
}

cv::Mat AdvancedOCR::preprocessFrame(const cv::Mat& frame) {
    cv::Mat processed;
    
    // Convert to grayscale if needed
    if (frame.channels() > 1) {
        cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = frame.clone();
    }
    
    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    
    // Apply adaptive threshold
    cv::adaptiveThreshold(processed, processed, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 11, 2);
    
    // Morphological operations to clean up
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
    
    return processed;
}

std::vector<cv::Rect> AdvancedOCR::detectTextRegions(const cv::Mat& frame) {
    std::vector<cv::Rect> regions;
    
    // Simple text region detection using contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(frame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : contours) {
        cv::Rect rect = cv::boundingRect(contour);
        
        // Filter by size (text regions should be reasonable size)
        if (rect.width > 20 && rect.height > 10 && rect.width < 500 && rect.height < 100) {
            regions.push_back(rect);
        }
    }
    
    return regions;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectGameUI(const cv::Mat& frame, const std::string& gameName) {
    std::vector<TextRegion> results;
    
    // Load game-specific templates
    loadGameTemplates(gameName);
    
    // Match templates
    results = matchGameTemplates(frame);
    
    // Also do general text detection
    std::vector<TextRegion> generalResults = detectText(frame, "");
    
    // Filter for game UI text
    for (const auto& result : generalResults) {
        if (isGameUIText(result.text)) {
            results.push_back(result);
        }
    }
    
    return results;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectHealthValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame, "");
    std::vector<TextRegion> healthResults;
    
    for (const auto& result : allResults) {
        if (isHealthValue(result.text)) {
            healthResults.push_back(result);
        }
    }
    
    return healthResults;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectScoreValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame, "");
    std::vector<TextRegion> scoreResults;
    
    for (const auto& result : allResults) {
        if (isScoreValue(result.text)) {
            scoreResults.push_back(result);
        }
    }
    
    return scoreResults;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectAmmoValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame, "");
    std::vector<TextRegion> ammoResults;
    
    for (const auto& result : allResults) {
        if (isAmmoValue(result.text)) {
            ammoResults.push_back(result);
        }
    }
    
    return ammoResults;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectTimeValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame, "");
    std::vector<TextRegion> timeResults;
    
    for (const auto& result : allResults) {
        if (isTimeValue(result.text)) {
            timeResults.push_back(result);
        }
    }
    
    return timeResults;
}

void AdvancedOCR::loadGameTemplates(const std::string& gameName) {
    // Load game-specific templates
    // For now, just load default templates
    loadDefaultGameTemplates();
}

void AdvancedOCR::loadDefaultGameTemplates() {
    gameTemplates.clear();
    
    // Add some default game UI templates
    // This would normally load from files or database
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::matchGameTemplates(const cv::Mat& frame) {
    std::vector<TextRegion> results;
    
    for (const auto& template_ : gameTemplates) {
        float matchScore = matchTemplate(frame, template_);
        if (matchScore > template_.threshold) {
            TextRegion result(template_.region, template_.expectedTextPattern, matchScore, template_.elementType);
            results.push_back(result);
        }
    }
    
    return results;
}

float AdvancedOCR::matchTemplate(const cv::Mat& frame, const GameUITemplate& template_) {
    if (template_.templateImage.empty()) return 0.0f;
    
    cv::Mat result;
    cv::matchTemplate(frame, template_.templateImage, result, cv::TM_CCOEFF_NORMED);
    
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
    
    return static_cast<float>(maxVal);
}

bool AdvancedOCR::isGameUIText(const std::string& text) {
    // Check if text looks like game UI elements
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    // Common game UI keywords
    std::vector<std::string> gameKeywords = {
        "health", "hp", "score", "ammo", "time", "level", "xp", "gold", "money",
        "kills", "deaths", "assists", "round", "match", "player", "team"
    };
    
    for (const auto& keyword : gameKeywords) {
        if (lowerText.find(keyword) != std::string::npos) {
            return true;
        }
    }
    
    // Check for numeric patterns
    std::regex numberPattern(R"(\d+)");
    if (std::regex_search(text, numberPattern)) {
        return true;
    }
    
    return false;
}

std::string AdvancedOCR::classifyTextType(const std::string& text) {
    if (isHealthValue(text)) return "health";
    if (isScoreValue(text)) return "score";
    if (isAmmoValue(text)) return "ammo";
    if (isTimeValue(text)) return "time";
    return "unknown";
}

cv::Scalar AdvancedOCR::detectTextColor(const cv::Mat& frame, const cv::Rect& region) {
    // Extract region and calculate average color
    cv::Mat roi = frame(region);
    cv::Scalar avgColor = cv::mean(roi);
    return avgColor;
}

bool AdvancedOCR::isHealthValue(const std::string& text) {
    // Check for health-related patterns
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    if (lowerText.find("health") != std::string::npos || lowerText.find("hp") != std::string::npos) {
        return true;
    }
    
    // Check for health bar patterns (e.g., "100/100", "75%")
    std::regex healthPattern(R"(\d+/\d+|\d+%)");
    return std::regex_search(text, healthPattern);
}

bool AdvancedOCR::isScoreValue(const std::string& text) {
    // Check for score-related patterns
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    if (lowerText.find("score") != std::string::npos || lowerText.find("points") != std::string::npos) {
        return true;
    }
    
    // Check for numeric patterns that could be scores
    std::regex scorePattern(R"(\d{3,})"); // 3+ digits
    return std::regex_search(text, scorePattern);
}

bool AdvancedOCR::isAmmoValue(const std::string& text) {
    // Check for ammo-related patterns
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    if (lowerText.find("ammo") != std::string::npos || lowerText.find("bullets") != std::string::npos) {
        return true;
    }
    
    // Check for ammo patterns (e.g., "30/30", "15")
    std::regex ammoPattern(R"(\d+/\d+|\d{1,2})");
    return std::regex_search(text, ammoPattern);
}

bool AdvancedOCR::isTimeValue(const std::string& text) {
    // Check for time-related patterns
    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    
    if (lowerText.find("time") != std::string::npos || lowerText.find("timer") != std::string::npos) {
        return true;
    }
    
    // Check for time patterns (e.g., "2:30", "01:45")
    std::regex timePattern(R"(\d{1,2}:\d{2})");
    return std::regex_search(text, timePattern);
}

void AdvancedOCR::setBackend(OCRBackend backend) {
    currentBackend = backend;
    if (initialized) {
        cleanup();
        initialize(backend);
    }
}

// FrameCache Implementation
FrameCache::FrameCache(size_t maxSize, int64_t timeout)
    : maxCacheSize(maxSize), timeoutMs(timeout) {
    cacheHits = 0;
    cacheMisses = 0;
}

FrameCache::~FrameCache() {
    clearCache();
}

bool FrameCache::findCachedResult(const cv::Mat& frame, const std::string& gameName,
                                 std::vector<AdvancedOCR::TextRegion>& results) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cleanupExpiredEntries();
    
    for (const auto& cached : cache) {
        if (cached.gameName == gameName && framesAreSimilar(frame, cached.frame)) {
            results = cached.results;
            cacheHits++;
            return true;
        }
    }
    
    cacheMisses++;
    return false;
}

void FrameCache::cacheResult(const cv::Mat& frame, const std::string& gameName,
                            const std::vector<AdvancedOCR::TextRegion>& results) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cleanupExpiredEntries();
    
    // Remove oldest entries if cache is full
    while (cache.size() >= maxCacheSize) {
        cache.erase(cache.begin());
    }
    
    CachedFrame cached;
    cached.frame = frame.clone();
    cached.gameName = gameName;
    cached.results = results;
    cached.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch());
    
    cache.push_back(cached);
}

void FrameCache::clearCache() {
    std::lock_guard<std::mutex> lock(cacheMutex);
    cache.clear();
    cacheHits = 0;
    cacheMisses = 0;
}

float FrameCache::getHitRate() const {
    std::lock_guard<std::mutex> lock(cacheMutex);
    int total = cacheHits + cacheMisses;
    return total > 0 ? static_cast<float>(cacheHits) / total : 0.0f;
}

void FrameCache::cleanupExpiredEntries() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch());
    
    cache.erase(
        std::remove_if(cache.begin(), cache.end(),
            [now, this](const CachedFrame& cached) {
                return (now - cached.timestamp).count() > timeoutMs;
            }),
        cache.end());
}

bool FrameCache::framesAreSimilar(const cv::Mat& frame1, const cv::Mat& frame2) {
    if (frame1.size() != frame2.size()) return false;
    
    double similarity = calculateFrameSimilarity(frame1, frame2);
    return similarity > 0.95; // 95% similarity threshold
}

double FrameCache::calculateFrameSimilarity(const cv::Mat& frame1, const cv::Mat& frame2) {
    // Simple similarity calculation using mean squared error
    cv::Mat diff;
    cv::absdiff(frame1, frame2, diff);
    cv::Scalar meanDiff = cv::mean(diff);
    
    // Convert to similarity percentage
    double similarity = 1.0 - (meanDiff[0] / 255.0);
    return std::max(0.0, similarity);
}

void AdvancedOCR::enableCaching(bool enable) {
    useCaching = enable;
}