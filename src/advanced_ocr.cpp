#include "advanced_ocr.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>

// AdvancedOCR Implementation
AdvancedOCR::AdvancedOCR() 
    : currentBackend(OCRBackend::WINDOWS_MEDIA_OCR), initialized(false), 
      lastProcessTime(0), useCaching(true) {
    gpuFrame = cv::makePtr<cv::cuda::GpuMat>();
    gpuGray = cv::makePtr<cv::cuda::GpuMat>();
    gpuProcessed = cv::makePtr<cv::cuda::GpuMat>();
}

AdvancedOCR::~AdvancedOCR() {
    cleanup();
}

bool AdvancedOCR::initialize(OCRBackend backend) {
    std::lock_guard<std::mutex> lock(processingMutex);
    
    if (initialized) {
        cleanup();
    }
    
    currentBackend = backend;
    
    switch (backend) {
        case OCRBackend::WINDOWS_MEDIA_OCR:
            return initializeWindowsOCR();
        case OCRBackend::TESSERACT:
            return initializeTesseract();
        case OCRBackend::OPENCV_EAST:
            return initializeOpenCV();
        default:
            return false;
    }
}

bool AdvancedOCR::initializeWindowsOCR() {
    try {
        // Initialize Windows.Media.Ocr
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr)) return false;
        
        // Create OCR engine
        hr = CoCreateInstance(CLSID_OcrEngine, nullptr, CLSCTX_INPROC_SERVER, 
                             IID_PPV_ARGS(&windowsOcrEngine));
        if (FAILED(hr)) return false;
        
        // Create bitmap factory
        hr = CoCreateInstance(CLSID_SoftwareBitmapFactory, nullptr, CLSCTX_INPROC_SERVER,
                             IID_PPV_ARGS(&bitmapFactory));
        if (FAILED(hr)) return false;
        
        initialized = true;
        loadDefaultGameTemplates();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool AdvancedOCR::initializeTesseract() {
    try {
        tesseractAPI = std::make_unique<tesseract::TessBaseAPI>();
        
        // Initialize Tesseract with English language
        if (tesseractAPI->Init(nullptr, "eng")) {
            return false;
        }
        
        // Set OCR parameters for game UI
        tesseractAPI->SetVariable("tessedit_char_whitelist", "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz:/");
        tesseractAPI->SetVariable("tessedit_pageseg_mode", "8"); // Single word
        tesseractAPI->SetVariable("classify_bln_numeric_mode", "1");
        
        initialized = true;
        loadDefaultGameTemplates();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool AdvancedOCR::initializeOpenCV() {
    try {
        // Initialize OpenCV CUDA
        if (cv::cuda::getCudaEnabledDeviceCount() == 0) {
            return false; // No CUDA support
        }
        
        initialized = true;
        loadDefaultGameTemplates();
        return true;
    }
    catch (...) {
        return false;
    }
}

void AdvancedOCR::cleanup() {
    std::lock_guard<std::mutex> lock(processingMutex);
    
    if (tesseractAPI) {
        tesseractAPI->End();
        tesseractAPI.reset();
    }
    
    windowsOcrEngine.Reset();
    bitmapFactory.Reset();
    
    CoUninitialize();
    
    initialized = false;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectText(const cv::Mat& frame, const std::string& gameName) {
    std::lock_guard<std::mutex> lock(processingMutex);
    
    if (!initialized) return {};
    
    // Check cache first
    if (useCaching && !cachedResults.empty()) {
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
        if (now - lastProcessTime < 100) { // 100ms cache
            return cachedResults;
        }
    }
    
    std::vector<TextRegion> results;
    
    switch (currentBackend) {
        case OCRBackend::WINDOWS_MEDIA_OCR:
            results = processWithWindowsOCR(frame);
            break;
        case OCRBackend::TESSERACT:
            results = processWithTesseract(frame);
            break;
        case OCRBackend::OPENCV_EAST:
            results = processWithOpenCV(frame);
            break;
    }
    
    // Filter results for game-specific UI elements
    if (!gameName.empty()) {
        results = detectGameUI(frame, gameName);
    }
    
    // Cache results
    if (useCaching) {
        cachedResults = results;
        lastProcessTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    return results;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectTextGPU(const cv::cuda::GpuMat& gpuFrame, const std::string& gameName) {
    std::lock_guard<std::mutex> lock(processingMutex);
    
    if (!initialized) return {};
    
    // Download to CPU for processing (most OCR libraries don't support GPU directly)
    cv::Mat frame;
    gpuFrame.download(frame);
    
    return detectText(frame, gameName);
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::processWithWindowsOCR(const cv::Mat& frame) {
    std::vector<TextRegion> results;
    
    try {
        // Convert OpenCV Mat to Windows SoftwareBitmap
        ComPtr<ISoftwareBitmap> bitmap;
        HRESULT hr = createSoftwareBitmapFromMat(frame, &bitmap);
        if (FAILED(hr)) return results;
        
        // Perform OCR
        ComPtr<IAsyncOperation<OcrResult*>> asyncOp;
        hr = windowsOcrEngine->RecognizeAsync(bitmap.Get(), &asyncOp);
        if (FAILED(hr)) return results;
        
        // Wait for completion (simplified - in real implementation use proper async handling)
        // This is a placeholder for the actual async implementation
        
        // For now, return empty results - full implementation would handle async properly
        return results;
    }
    catch (...) {
        return results;
    }
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::processWithTesseract(const cv::Mat& frame) {
    std::vector<TextRegion> results;
    
    if (!tesseractAPI) return results;
    
    try {
        // Preprocess frame
        cv::Mat processed = preprocessFrame(frame);
        
        // Set image for Tesseract
        tesseractAPI->SetImage(processed.data, processed.cols, processed.rows, 
                              processed.channels(), processed.step);
        
        // Get text regions
        std::vector<cv::Rect> textRegions = detectTextRegions(processed);
        
        for (const auto& region : textRegions) {
            // Extract region
            cv::Mat roi = processed(region);
            
            // Set ROI for Tesseract
            tesseractAPI->SetRectangle(region.x, region.y, region.width, region.height);
            
            // Get text
            char* text = tesseractAPI->GetUTF8Text();
            if (text) {
                std::string textStr(text);
                delete[] text;
                
                // Get confidence
                float confidence = tesseractAPI->MeanTextConf() / 100.0f;
                
                if (confidence > 0.3f && !textStr.empty()) {
                    TextRegion textRegion(region, textStr, confidence);
                    textRegion.detectedType = classifyTextType(textStr);
                    textRegion.color = detectTextColor(frame, region);
                    results.push_back(textRegion);
                }
            }
        }
        
        return results;
    }
    catch (...) {
        return results;
    }
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::processWithOpenCV(const cv::Mat& frame) {
    std::vector<TextRegion> results;
    
    try {
        // Use OpenCV EAST text detector
        cv::Mat processed = preprocessFrame(frame);
        std::vector<cv::Rect> textRegions = detectTextRegions(processed);
        
        for (const auto& region : textRegions) {
            // Simple text extraction using OpenCV
            cv::Mat roi = processed(region);
            
            // Convert to grayscale if needed
            if (roi.channels() > 1) {
                cv::cvtColor(roi, roi, cv::COLOR_BGR2GRAY);
            }
            
            // Apply threshold
            cv::threshold(roi, roi, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
            
            // For now, return region without actual text recognition
            // In a full implementation, you'd use additional OpenCV text recognition
            TextRegion textRegion(region, "TEXT", 0.5f);
            textRegion.detectedType = "unknown";
            textRegion.color = detectTextColor(frame, region);
            results.push_back(textRegion);
        }
        
        return results;
    }
    catch (...) {
        return results;
    }
}

cv::Mat AdvancedOCR::preprocessFrame(const cv::Mat& frame) {
    cv::Mat processed;
    
    // Convert to grayscale
    if (frame.channels() > 1) {
        cv::cvtColor(frame, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = frame.clone();
    }
    
    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
    
    // Apply adaptive threshold
    cv::adaptiveThreshold(processed, processed, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                         cv::THRESH_BINARY, 11, 2);
    
    // Morphological operations to clean up
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(processed, processed, cv::MORPH_CLOSE, kernel);
    
    return processed;
}

std::vector<cv::Rect> AdvancedOCR::detectTextRegions(const cv::Mat& frame) {
    std::vector<cv::Rect> regions;
    
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(frame, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : contours) {
        cv::Rect boundingRect = cv::boundingRect(contour);
        
        // Filter regions by size (typical text regions)
        if (boundingRect.width > 20 && boundingRect.height > 10 && 
            boundingRect.width < 200 && boundingRect.height < 50) {
            regions.push_back(boundingRect);
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
    std::vector<TextRegion> generalResults = detectText(frame);
    
    // Combine and filter results
    for (const auto& result : generalResults) {
        if (isGameUIText(result.text)) {
            results.push_back(result);
        }
    }
    
    return results;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectHealthValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame);
    std::vector<TextRegion> healthResults;
    
    for (const auto& result : allResults) {
        if (isHealthValue(result.text)) {
            healthResults.push_back(result);
        }
    }
    
    return healthResults;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectScoreValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame);
    std::vector<TextRegion> scoreResults;
    
    for (const auto& result : allResults) {
        if (isScoreValue(result.text)) {
            scoreResults.push_back(result);
        }
    }
    
    return scoreResults;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectAmmoValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame);
    std::vector<TextRegion> ammoResults;
    
    for (const auto& result : allResults) {
        if (isAmmoValue(result.text)) {
            ammoResults.push_back(result);
        }
    }
    
    return ammoResults;
}

std::vector<AdvancedOCR::TextRegion> AdvancedOCR::detectTimeValues(const cv::Mat& frame) {
    std::vector<TextRegion> allResults = detectText(frame);
    std::vector<TextRegion> timeResults;
    
    for (const auto& result : allResults) {
        if (isTimeValue(result.text)) {
            timeResults.push_back(result);
        }
    }
    
    return timeResults;
}

void AdvancedOCR::loadGameTemplates(const std::string& gameName) {
    // Load game-specific templates from database
    // This would typically load from a file or database
    loadDefaultGameTemplates();
}

void AdvancedOCR::loadDefaultGameTemplates() {
    gameTemplates.clear();
    
    // Add default templates for common games
    // These would be loaded from actual template files in a real implementation
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
    if (text.empty()) return false;
    
    // Check for numeric values (health, score, ammo)
    if (std::all_of(text.begin(), text.end(), ::isdigit)) {
        return true;
    }
    
    // Check for time format (MM:SS)
    if (text.find(':') != std::string::npos && text.length() <= 8) {
        return true;
    }
    
    // Check for common game UI words
    std::vector<std::string> gameWords = {"HEALTH", "SCORE", "AMMO", "TIME", "LEVEL", "XP", "KILLS", "DEATHS"};
    std::string upperText = text;
    std::transform(upperText.begin(), upperText.end(), upperText.begin(), ::toupper);
    
    for (const auto& word : gameWords) {
        if (upperText.find(word) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

std::string AdvancedOCR::classifyTextType(const std::string& text) {
    if (text.empty()) return "unknown";
    
    // Classify based on content
    if (isHealthValue(text)) return "health";
    if (isScoreValue(text)) return "score";
    if (isAmmoValue(text)) return "ammo";
    if (isTimeValue(text)) return "time";
    
    return "unknown";
}

cv::Scalar AdvancedOCR::detectTextColor(const cv::Mat& frame, const cv::Rect& region) {
    if (region.x < 0 || region.y < 0 || 
        region.x + region.width > frame.cols || 
        region.y + region.height > frame.rows) {
        return cv::Scalar(0, 0, 0);
    }
    
    cv::Mat roi = frame(region);
    cv::Scalar meanColor = cv::mean(roi);
    
    return meanColor;
}

bool AdvancedOCR::isHealthValue(const std::string& text) {
    // Check if text represents health values
    if (text.empty()) return false;
    
    // Numeric health values
    if (std::all_of(text.begin(), text.end(), ::isdigit)) {
        int value = std::stoi(text);
        return value >= 0 && value <= 1000; // Typical health range
    }
    
    // Health percentage
    if (text.back() == '%') {
        std::string numPart = text.substr(0, text.length() - 1);
        if (std::all_of(numPart.begin(), numPart.end(), ::isdigit)) {
            return true;
        }
    }
    
    return false;
}

bool AdvancedOCR::isScoreValue(const std::string& text) {
    // Check if text represents score values
    if (text.empty()) return false;
    
    // Numeric scores
    if (std::all_of(text.begin(), text.end(), ::isdigit)) {
        return true;
    }
    
    // Scores with separators (e.g., "1,234,567")
    if (text.find(',') != std::string::npos) {
        return true;
    }
    
    return false;
}

bool AdvancedOCR::isAmmoValue(const std::string& text) {
    // Check if text represents ammo values
    if (text.empty()) return false;
    
    // Numeric ammo counts
    if (std::all_of(text.begin(), text.end(), ::isdigit)) {
        int value = std::stoi(text);
        return value >= 0 && value <= 999; // Typical ammo range
    }
    
    // Ammo with slash (e.g., "30/120")
    if (text.find('/') != std::string::npos) {
        return true;
    }
    
    return false;
}

bool AdvancedOCR::isTimeValue(const std::string& text) {
    // Check if text represents time values
    if (text.empty()) return false;
    
    // Time format MM:SS or HH:MM:SS
    if (text.find(':') != std::string::npos) {
        return true;
    }
    
    return false;
}

void AdvancedOCR::setBackend(OCRBackend backend) {
    if (backend != currentBackend) {
        initialize(backend);
    }
}

HRESULT AdvancedOCR::createSoftwareBitmapFromMat(const cv::Mat& frame, ISoftwareBitmap** bitmap) {
    // Convert OpenCV Mat to Windows SoftwareBitmap
    // This is a simplified implementation
    return E_NOTIMPL; // Placeholder
}

HRESULT AdvancedOCR::convertMatToBitmapStream(const cv::Mat& frame, IRandomAccessStream** stream) {
    // Convert OpenCV Mat to Windows bitmap stream
    // This is a simplified implementation
    return E_NOTIMPL; // Placeholder
}

// FrameCache Implementation
FrameCache::FrameCache(size_t maxSize, int64_t timeout) 
    : maxCacheSize(maxSize), cacheTimeout(timeout) {
}

bool FrameCache::findCachedResult(const cv::Mat& frame, const std::string& gameName, 
                                 std::vector<AdvancedOCR::TextRegion>& results) {
    cleanupExpiredEntries();
    
    for (const auto& cached : cache) {
        if (cached.gameName == gameName && framesAreSimilar(frame, cached.frame)) {
            results = cached.results;
            return true;
        }
    }
    
    return false;
}

void FrameCache::cacheResult(const cv::Mat& frame, const std::string& gameName, 
                            const std::vector<AdvancedOCR::TextRegion>& results) {
    cleanupExpiredEntries();
    
    // Remove oldest entries if cache is full
    while (cache.size() >= maxCacheSize) {
        cache.erase(cache.begin());
    }
    
    CachedFrame cached;
    cached.frame = frame.clone();
    cached.results = results;
    cached.gameName = gameName;
    cached.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    cache.push_back(cached);
}

void FrameCache::clearCache() {
    cache.clear();
}

float FrameCache::getHitRate() const {
    // Calculate cache hit rate
    // This would need to track hits/misses in a real implementation
    return 0.0f; // Placeholder
}

void FrameCache::cleanupExpiredEntries() {
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    cache.erase(
        std::remove_if(cache.begin(), cache.end(),
            [now, this](const CachedFrame& cached) {
                return (now - cached.timestamp) > cacheTimeout;
            }),
        cache.end()
    );
}

bool FrameCache::framesAreSimilar(const cv::Mat& frame1, const cv::Mat& frame2) {
    if (frame1.size() != frame2.size() || frame1.type() != frame2.type()) {
        return false;
    }
    
    double similarity = calculateFrameSimilarity(frame1, frame2);
    return similarity > 0.95; // 95% similarity threshold
}

double FrameCache::calculateFrameSimilarity(const cv::Mat& frame1, const cv::Mat& frame2) {
    cv::Mat diff;
    cv::absdiff(frame1, frame2, diff);
    
    cv::Scalar sum = cv::sum(diff);
    double totalDiff = sum[0] + sum[1] + sum[2];
    
    double maxDiff = frame1.rows * frame1.cols * frame1.channels() * 255.0;
    return 1.0 - (totalDiff / maxDiff);
}
