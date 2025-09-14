#include "game_analytics.h"
#include "cuda_support.h"
#include "performance_monitor.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/dnn.hpp>
#include <numeric>
#include <cmath>

// GameEventDetector Implementation
GameEventDetector::GameEventDetector() 
    : totalEventsDetected(0), falsePositives(0), averageDetectionTime(0.0), isDetecting(false) {
    
    // Initialize CUDA support
    useCuda = CudaSupport::isAvailable();
    
    // Initialize optical flow with CUDA detection
    if (useCuda) {
        // CUDA optical flow would be initialized here if available
        opticalFlow = cv::FarnebackOpticalFlow::create(); // CPU fallback for now
    } else {
        opticalFlow = cv::FarnebackOpticalFlow::create();
    }
    
    // Initialize game colors
    gameColors["red"] = cv::Scalar(0, 0, 255);
    gameColors["green"] = cv::Scalar(0, 255, 0);
    gameColors["blue"] = cv::Scalar(255, 0, 0);
    gameColors["yellow"] = cv::Scalar(0, 255, 255);
    gameColors["white"] = cv::Scalar(255, 255, 255);
    gameColors["black"] = cv::Scalar(0, 0, 0);
}

GameEventDetector::~GameEventDetector() {
    cleanup();
}

bool GameEventDetector::initialize() {
    // Initialize event detection components
    loadDefaultVisualCues();
    return true;
}

void GameEventDetector::cleanup() {
    isDetecting = false;
    
    if (detectionThread.joinable()) {
        detectionThread.join();
    }
    
    opticalFlow.release();
}

std::vector<GameEventDetector::GameEvent> GameEventDetector::detectEvents(const cv::Mat& frame) {
    TIMED_OPERATION("Game Event Detection");
    std::lock_guard<std::mutex> lock(detectionMutex);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<GameEvent> events;
    
    // Detect different types of events
    auto deathEvents = detectDeaths(frame);
    auto levelUpEvents = detectLevelUps(frame);
    auto damageEvents = detectDamage(frame);
    auto shakeEvents = detectScreenShake(frame);
    auto colorEvents = detectColorChanges(frame);
    
    // Combine all events
    events.insert(events.end(), deathEvents.begin(), deathEvents.end());
    events.insert(events.end(), levelUpEvents.begin(), levelUpEvents.end());
    events.insert(events.end(), damageEvents.begin(), damageEvents.end());
    events.insert(events.end(), shakeEvents.begin(), shakeEvents.end());
    events.insert(events.end(), colorEvents.begin(), colorEvents.end());
    
    // Update statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    double detectionTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    totalEventsDetected += events.size();
    averageDetectionTime = (averageDetectionTime * (totalEventsDetected - events.size()) + detectionTime) / totalEventsDetected;
    
    return events;
}

std::vector<GameEventDetector::GameEvent> GameEventDetector::detectDeaths(const cv::Mat& frame) {
    std::vector<GameEvent> events;
    
    // Detect red screen flash (common death indicator)
    if (detectRedScreenFlash(frame)) {
        GameEvent event(EventType::DEATH, "Player death detected", 0.8f);
        events.push_back(event);
    }
    
    // Detect death text patterns
    // This would use OCR to detect "DEATH", "GAME OVER", etc.
    
    return events;
}

std::vector<GameEventDetector::GameEvent> GameEventDetector::detectLevelUps(const cv::Mat& frame) {
    std::vector<GameEvent> events;
    
    // Detect golden effects (common level up indicator)
    if (detectGoldenEffects(frame)) {
        GameEvent event(EventType::LEVEL_UP, "Level up detected", 0.7f);
        events.push_back(event);
    }
    
    return events;
}

std::vector<GameEventDetector::GameEvent> GameEventDetector::detectDamage(const cv::Mat& frame) {
    std::vector<GameEvent> events;
    
    // Detect screen shake (damage indicator)
    auto shakeEvents = detectScreenShake(frame);
    for (auto& event : shakeEvents) {
        event.type = EventType::DAMAGE_TAKEN;
        event.description = "Damage taken detected";
        events.push_back(event);
    }
    
    // Detect red damage indicators
    if (detectColorFlash(frame, gameColors["red"], 0.3f)) {
        GameEvent event(EventType::DAMAGE_TAKEN, "Damage indicator detected", 0.6f);
        events.push_back(event);
    }
    
    return events;
}

std::vector<GameEventDetector::GameEvent> GameEventDetector::detectScreenShake(const cv::Mat& frame) {
    std::vector<GameEvent> events;
    
    if (prevFrame.empty()) {
        currentFrame = frame.clone();
        prevFrame = currentFrame.clone();
        return events;
    }
    
    currentFrame = frame.clone();
    
    // Calculate optical flow
    cv::Mat flow = calculateOpticalFlow(prevFrame, currentFrame);
    float motionMagnitude = calculateMotionMagnitude(flow);
    
    // If motion magnitude exceeds threshold, it's likely screen shake
    if (motionMagnitude > 5.0f) {
        GameEvent event(EventType::DAMAGE_TAKEN, "Screen shake detected", motionMagnitude / 10.0f);
        events.push_back(event);
    }
    
    // Update previous frame
    prevFrame = currentFrame.clone();
    
    return events;
}

std::vector<GameEventDetector::GameEvent> GameEventDetector::detectColorChanges(const cv::Mat& frame) {
    std::vector<GameEvent> events;
    
    // Detect various color flashes that might indicate game events
    for (const auto& colorPair : gameColors) {
        if (detectColorFlash(frame, colorPair.second, 0.2f)) {
            GameEvent event(EventType::UNKNOWN, "Color flash detected: " + colorPair.first, 0.5f);
            events.push_back(event);
        }
    }
    
    return events;
}

bool GameEventDetector::detectRedScreenFlash(const cv::Mat& frame) {
    // Convert to HSV for better color detection
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    
    // Define red color range
    cv::Scalar lowerRed(0, 50, 50);
    cv::Scalar upperRed(10, 255, 255);
    cv::Scalar lowerRed2(170, 50, 50);
    cv::Scalar upperRed2(180, 255, 255);
    
    cv::Mat mask1, mask2, mask;
    cv::inRange(hsv, lowerRed, upperRed, mask1);
    cv::inRange(hsv, lowerRed2, upperRed2, mask2);
    cv::bitwise_or(mask1, mask2, mask);
    
    // Count red pixels
    int redPixels = cv::countNonZero(mask);
    int totalPixels = frame.rows * frame.cols;
    
    // If more than 30% of screen is red, it's likely a death screen
    return (static_cast<double>(redPixels) / totalPixels) > 0.3;
}

bool GameEventDetector::detectGoldenEffects(const cv::Mat& frame) {
    // Convert to HSV for better color detection
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    
    // Define golden color range
    cv::Scalar lowerGold(20, 100, 100);
    cv::Scalar upperGold(30, 255, 255);
    
    cv::Mat mask;
    cv::inRange(hsv, lowerGold, upperGold, mask);
    
    // Count golden pixels
    int goldenPixels = cv::countNonZero(mask);
    int totalPixels = frame.rows * frame.cols;
    
    // If more than 10% of screen is golden, it's likely a level up effect
    return (static_cast<double>(goldenPixels) / totalPixels) > 0.1;
}

bool GameEventDetector::detectColorFlash(const cv::Mat& frame, const cv::Scalar& targetColor, float threshold) {
    // Convert to HSV for better color detection
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    
    // Create color range around target color
    cv::Scalar lower = targetColor - cv::Scalar(10, 50, 50);
    cv::Scalar upper = targetColor + cv::Scalar(10, 50, 50);
    
    cv::Mat mask;
    cv::inRange(hsv, lower, upper, mask);
    
    // Count pixels of target color
    int colorPixels = cv::countNonZero(mask);
    int totalPixels = frame.rows * frame.cols;
    
    return (static_cast<double>(colorPixels) / totalPixels) > threshold;
}

cv::Mat GameEventDetector::calculateOpticalFlow(const cv::Mat& prevFrame, const cv::Mat& currFrame) {
    cv::Mat flow;
    
    if (opticalFlow && !prevFrame.empty() && !currFrame.empty()) {
        cv::Mat prevGray, currGray;
        cv::cvtColor(prevFrame, prevGray, cv::COLOR_BGR2GRAY);
        cv::cvtColor(currFrame, currGray, cv::COLOR_BGR2GRAY);
        
        opticalFlow->calc(prevGray, currGray, flow);
    }
    
    return flow;
}

float GameEventDetector::calculateMotionMagnitude(const cv::Mat& flow) {
    if (flow.empty()) return 0.0f;
    if (flow.channels() != 2) return 0.0f;

    // Split 2-channel flow into x and y components and compute magnitude
    std::vector<cv::Mat> flowChannels(2);
    cv::split(flow, flowChannels);

    cv::Mat magnitude;
    cv::magnitude(flowChannels[0], flowChannels[1], magnitude);

    if (magnitude.empty()) return 0.0f;
    return static_cast<float>(cv::mean(magnitude)[0]);
}

void GameEventDetector::loadDefaultVisualCues() {
    // Load default visual cues for common game events
    VisualCue redFlash;
    redFlash.name = "Red Flash";
    redFlash.color = cv::Scalar(0, 0, 255);
    redFlash.threshold = 0.3f;
    redFlash.duration = 500;
    addVisualCue(redFlash, EventType::DEATH);
    
    VisualCue goldenFlash;
    goldenFlash.name = "Golden Flash";
    goldenFlash.color = cv::Scalar(0, 255, 255);
    goldenFlash.threshold = 0.1f;
    goldenFlash.duration = 1000;
    addVisualCue(goldenFlash, EventType::LEVEL_UP);
}

void GameEventDetector::addVisualCue(const VisualCue& cue, EventType eventType) {
    visualCues.push_back(cue);
    eventCues[eventType].push_back(cue);
}

double GameEventDetector::getDetectionAccuracy() const {
    if (totalEventsDetected == 0) return 0.0;
    return static_cast<double>(totalEventsDetected - falsePositives) / totalEventsDetected;
}

// GameFingerprinting Implementation
GameFingerprinting::GameFingerprinting() 
    : totalFingerprints(0), successfulMatches(0), averageFingerprintTime(0.0) {
    
    // Initialize template matcher (CPU fallback)
    templateMatcher = cv::Mat();
}

GameFingerprinting::~GameFingerprinting() {
    cleanup();
}

bool GameFingerprinting::initialize() {
    loadGameDatabase();
    return true;
}

void GameFingerprinting::cleanup() {
    templateMatcher.release();
}

GameFingerprinting::FingerprintMatch GameFingerprinting::identifyGame(HWND hwnd) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::string processName = getProcessNameFromHWND(hwnd);
    std::string windowTitle = getWindowTitleFromHWND(hwnd);
    
    // Try different identification methods
    float processMatch = matchByProcessName(processName);
    float titleMatch = matchByWindowTitle(windowTitle);
    
    FingerprintMatch bestMatch;
    
    if (processMatch > 0.8f) {
        bestMatch = FingerprintMatch(getProfileByProcessName(processName), processMatch, "Process Name");
    } else if (titleMatch > 0.7f) {
        bestMatch = FingerprintMatch(getProfileByWindowTitle(windowTitle), titleMatch, "Window Title");
    }
    
    // Update statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    double fingerprintTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    totalFingerprints++;
    if (bestMatch.confidence > 0.5f) {
        successfulMatches++;
    }
    
    averageFingerprintTime = (averageFingerprintTime * (totalFingerprints - 1) + fingerprintTime) / totalFingerprints;
    
    return bestMatch;
}

void GameFingerprinting::loadGameDatabase() {
    loadTop50GamesDatabase();
}

void GameFingerprinting::loadTop50GamesDatabase() {
    // Load top 50 games database
    std::vector<std::string> topGames = {
        "Counter-Strike 2", "Valorant", "League of Legends", "Dota 2", "Fortnite",
        "Apex Legends", "Call of Duty", "Overwatch 2", "Rocket League", "Minecraft",
        "World of Warcraft", "Final Fantasy XIV", "Genshin Impact", "PUBG", "Rainbow Six Siege",
        "Destiny 2", "Warframe", "Path of Exile", "Diablo IV", "Elden Ring",
        "Cyberpunk 2077", "The Witcher 3", "Grand Theft Auto V", "Red Dead Redemption 2", "Assassin's Creed",
        "FIFA", "NBA 2K", "Madden NFL", "Call of Duty: Warzone", "Fall Guys",
        "Among Us", "Phasmophobia", "Dead by Daylight", "Left 4 Dead 2", "Team Fortress 2",
        "Half-Life 2", "Portal 2", "Terraria", "Stardew Valley", "Hollow Knight",
        "Celeste", "Cuphead", "Ori and the Blind Forest", "Hades", "Bastion",
        "Transistor", "Pyre", "Supergiant Games", "Indie Games", "Steam Games"
    };
    
    for (const auto& gameName : topGames) {
        GameProfile profile(gameName);
        profile.executableName = gameName + ".exe";
        profile.windowTitle = gameName;
        
        // Add to maps
        processNameMap[profile.executableName] = profile;
        windowTitleMap[profile.windowTitle] = profile;
        
        gameDatabase.push_back(profile);
    }
}

float GameFingerprinting::matchByProcessName(const std::string& processName) {
    auto it = processNameMap.find(processName);
    if (it != processNameMap.end()) {
        return 1.0f; // Exact match
    }
    
    // Fuzzy matching
    for (const auto& pair : processNameMap) {
        if (pair.first.find(processName) != std::string::npos ||
            processName.find(pair.first) != std::string::npos) {
            return 0.8f;
        }
    }
    
    return 0.0f;
}

float GameFingerprinting::matchByWindowTitle(const std::string& windowTitle) {
    auto it = windowTitleMap.find(windowTitle);
    if (it != windowTitleMap.end()) {
        return 1.0f; // Exact match
    }
    
    // Fuzzy matching
    for (const auto& pair : windowTitleMap) {
        if (pair.first.find(windowTitle) != std::string::npos ||
            windowTitle.find(pair.first) != std::string::npos) {
            return 0.7f;
        }
    }
    
    return 0.0f;
}

GameFingerprinting::GameProfile GameFingerprinting::getProfileByProcessName(const std::string& processName) {
    auto it = processNameMap.find(processName);
    if (it != processNameMap.end()) {
        return it->second;
    }
    return GameProfile("Unknown");
}

GameFingerprinting::GameProfile GameFingerprinting::getProfileByWindowTitle(const std::string& windowTitle) {
    auto it = windowTitleMap.find(windowTitle);
    if (it != windowTitleMap.end()) {
        return it->second;
    }
    return GameProfile("Unknown");
}

std::string GameFingerprinting::getProcessNameFromHWND(HWND hwnd) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return "";
    
    char processName[MAX_PATH];
    DWORD size = sizeof(processName);
    
    if (QueryFullProcessImageNameA(hProcess, 0, processName, &size)) {
        CloseHandle(hProcess);
        std::string fullPath(processName);
        size_t lastSlash = fullPath.find_last_of("\\/");
        return (lastSlash != std::string::npos) ? fullPath.substr(lastSlash + 1) : fullPath;
    }
    
    CloseHandle(hProcess);
    return "";
}

std::string GameFingerprinting::getWindowTitleFromHWND(HWND hwnd) {
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    return std::string(title);
}

double GameFingerprinting::getMatchAccuracy() const {
    if (totalFingerprints == 0) return 0.0;
    return static_cast<double>(successfulMatches) / totalFingerprints;
}

// BloombergAnalyticsEngine Implementation
BloombergAnalyticsEngine::BloombergAnalyticsEngine() 
    : lookbackPeriod(20), smoothingFactor(0.1), volatilityThreshold(0.2),
      totalCalculations(0), averageCalculationTime(0.0) {
}

BloombergAnalyticsEngine::~BloombergAnalyticsEngine() {
    cleanup();
}

bool BloombergAnalyticsEngine::initialize(int lookback, double smoothing) {
    lookbackPeriod = lookback;
    smoothingFactor = smoothing;
    return true;
}

void BloombergAnalyticsEngine::cleanup() {
    // Cleanup resources
}

void BloombergAnalyticsEngine::addPerformanceData(double performance, int64_t timestamp) {
    performanceHistory.push_back(performance);
    timeSeries.push_back(timestamp);
    
    // Keep only recent data
    if (performanceHistory.size() > lookbackPeriod * 2) {
        performanceHistory.erase(performanceHistory.begin());
        timeSeries.erase(timeSeries.begin());
    }
}

BloombergAnalyticsEngine::PerformanceMetrics BloombergAnalyticsEngine::calculateMetrics() {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    PerformanceMetrics metrics;
    
    if (performanceHistory.size() < 2) {
        return metrics;
    }
    
    // Calculate technical indicators
    metrics.rsi = calculateRSI();
    metrics.macd = calculateMACD();
    metrics.volatility = calculateVolatility();
    metrics.sharpeRatio = calculateSharpeRatio();
    
    // Calculate performance indices
    metrics.consistencyIndex = calculateConsistencyIndex();
    metrics.improvementIndex = calculateImprovementIndex();
    metrics.stabilityIndex = calculateStabilityIndex();
    
    // Pattern detection
    metrics.deathClusters = detectDeathClusters();
    metrics.performanceCycles = detectPerformanceCycles();
    metrics.trendDirection = calculateTrendDirection();
    
    // Predictive modeling
    metrics.predictedPerformance = predictNextPerformance();
    metrics.confidenceInterval = calculatePredictionConfidence();
    metrics.forecast = generateForecast();
    
    // Update statistics
    auto endTime = std::chrono::high_resolution_clock::now();
    double calculationTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    totalCalculations++;
    averageCalculationTime = (averageCalculationTime * (totalCalculations - 1) + calculationTime) / totalCalculations;
    
    return metrics;
}

double BloombergAnalyticsEngine::calculateRSI(int period) {
    if (performanceHistory.size() < period + 1) return 50.0;
    
    std::vector<double> gains, losses;
    
    for (size_t i = 1; i < performanceHistory.size(); ++i) {
        double change = performanceHistory[i] - performanceHistory[i-1];
        if (change > 0) {
            gains.push_back(change);
            losses.push_back(0);
        } else {
            gains.push_back(0);
            losses.push_back(-change);
        }
    }
    
    double avgGain = calculateEMA(gains, period, smoothingFactor);
    double avgLoss = calculateEMA(losses, period, smoothingFactor);
    
    if (avgLoss == 0) return 100.0;
    
    double rs = avgGain / avgLoss;
    return 100.0 - (100.0 / (1.0 + rs));
}

double BloombergAnalyticsEngine::calculateMACD(int fastPeriod, int slowPeriod, int signalPeriod) {
    if (performanceHistory.size() < slowPeriod) return 0.0;
    
    double emaFast = calculateEMA(performanceHistory, fastPeriod, smoothingFactor);
    double emaSlow = calculateEMA(performanceHistory, slowPeriod, smoothingFactor);
    
    return emaFast - emaSlow;
}

double BloombergAnalyticsEngine::calculateVolatility(int period) {
    if (performanceHistory.size() < period) return 0.0;
    
    std::vector<double> recentData = getRecentData(period);
    return calculateStandardDeviation(recentData);
}

double BloombergAnalyticsEngine::calculateSharpeRatio(double riskFreeRate) {
    if (performanceHistory.size() < 2) return 0.0;
    
    double meanReturn = calculateSimpleAverage(performanceHistory);
    double volatility = calculateVolatility();
    
    if (volatility == 0) return 0.0;
    
    return (meanReturn - riskFreeRate) / volatility;
}

double BloombergAnalyticsEngine::calculateConsistencyIndex() {
    if (performanceHistory.size() < 2) return 0.0;
    
    double mean = calculateSimpleAverage(performanceHistory);
    double variance = 0.0;
    
    for (double value : performanceHistory) {
        variance += (value - mean) * (value - mean);
    }
    variance /= performanceHistory.size();
    
    double standardDeviation = std::sqrt(variance);
    
    // Consistency is inverse of coefficient of variation
    return mean > 0 ? 1.0 / (1.0 + standardDeviation / mean) : 0.0;
}

double BloombergAnalyticsEngine::calculateImprovementIndex() {
    if (performanceHistory.size() < 2) return 0.0;
    
    // Calculate trend using linear regression
    return calculateTrendDirection();
}

double BloombergAnalyticsEngine::calculateStabilityIndex() {
    if (performanceHistory.size() < 3) return 0.0;
    
    // Calculate stability as inverse of volatility
    double volatility = calculateVolatility();
    return volatility > 0 ? 1.0 / (1.0 + volatility) : 1.0;
}

std::vector<int> BloombergAnalyticsEngine::detectDeathClusters() {
    std::vector<int> clusters;
    
    // This would analyze event history for death patterns
    // For now, return empty vector
    return clusters;
}

std::vector<double> BloombergAnalyticsEngine::detectPerformanceCycles() {
    std::vector<double> cycles;
    
    // This would analyze performance history for cyclical patterns
    // For now, return empty vector
    return cycles;
}

double BloombergAnalyticsEngine::calculateTrendDirection() {
    if (performanceHistory.size() < 2) return 0.0;
    
    // Simple linear regression
    double n = performanceHistory.size();
    double sumX = 0, sumY = 0, sumXY = 0, sumXX = 0;
    
    for (size_t i = 0; i < performanceHistory.size(); ++i) {
        double x = static_cast<double>(i);
        double y = performanceHistory[i];
        
        sumX += x;
        sumY += y;
        sumXY += x * y;
        sumXX += x * x;
    }
    
    double slope = (n * sumXY - sumX * sumY) / (n * sumXX - sumX * sumX);
    return slope;
}

double BloombergAnalyticsEngine::predictNextPerformance() {
    if (performanceHistory.empty()) return 0.0;
    
    // Use exponential smoothing for prediction
    return exponentialSmoothing(performanceHistory, smoothingFactor);
}

std::vector<double> BloombergAnalyticsEngine::generateForecast(int periods) {
    std::vector<double> forecast;
    
    if (performanceHistory.empty()) return forecast;
    
    double currentValue = performanceHistory.back();
    double trend = calculateTrendDirection();
    
    for (int i = 1; i <= periods; ++i) {
        double predictedValue = currentValue + (trend * i);
        forecast.push_back(predictedValue);
    }
    
    return forecast;
}

double BloombergAnalyticsEngine::calculatePredictionConfidence() {
    if (predictionHistory.empty() || actualHistory.empty()) return 0.0;
    
    // Calculate correlation between predictions and actual values
    return calculateCorrelation(predictionHistory, actualHistory);
}

BloombergAnalyticsEngine::TradingSignal BloombergAnalyticsEngine::generateTradingSignal() {
    auto metrics = calculateMetrics();
    
    TradingSignal signal;
    signal.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Determine signal based on technical indicators
    if (metrics.rsi < 30 && metrics.trendDirection > 0) {
        signal.type = TradingSignal::SignalType::BUY;
        signal.strength = 0.8;
        signal.reasoning = "Oversold with positive trend";
    } else if (metrics.rsi > 70 && metrics.trendDirection < 0) {
        signal.type = TradingSignal::SignalType::SELL;
        signal.strength = 0.8;
        signal.reasoning = "Overbought with negative trend";
    } else if (metrics.macd > 0 && metrics.improvementIndex > 0.1) {
        signal.type = TradingSignal::SignalType::BUY;
        signal.strength = 0.6;
        signal.reasoning = "Positive MACD with improvement";
    } else if (metrics.macd < 0 && metrics.improvementIndex < -0.1) {
        signal.type = TradingSignal::SignalType::SELL;
        signal.strength = 0.6;
        signal.reasoning = "Negative MACD with decline";
    } else {
        signal.type = TradingSignal::SignalType::HOLD;
        signal.strength = 0.5;
        signal.reasoning = "No clear signal";
    }
    
    return signal;
}

std::string BloombergAnalyticsEngine::exportToCSV() {
    std::stringstream csv;
    csv << "Timestamp,Performance,RSI,MACD,Volatility,SharpeRatio\n";
    
    for (size_t i = 0; i < performanceHistory.size(); ++i) {
        csv << timeSeries[i] << "," << performanceHistory[i] << ",";
        
        if (i >= 13) { // RSI needs at least 14 periods
            csv << calculateRSI() << ",";
        } else {
            csv << "0,";
        }
        
        if (i >= 25) { // MACD needs at least 26 periods
            csv << calculateMACD() << ",";
        } else {
            csv << "0,";
        }
        
        csv << calculateVolatility() << "," << calculateSharpeRatio() << "\n";
    }
    
    return csv.str();
}

double BloombergAnalyticsEngine::getPredictionAccuracy() const {
    if (predictionHistory.empty() || actualHistory.empty()) return 0.0;
    
    double totalError = 0.0;
    for (size_t i = 0; i < predictionHistory.size(); ++i) {
        totalError += std::abs(predictionHistory[i] - actualHistory[i]);
    }
    
    return 1.0 - (totalError / predictionHistory.size());
}

// Helper functions
double BloombergAnalyticsEngine::calculateEMA(const std::vector<double>& data, int period, double smoothing) {
    if (data.empty()) return 0.0;
    if (data.size() < period) return calculateSimpleAverage(data);
    
    double ema = calculateSimpleAverage(std::vector<double>(data.begin(), data.begin() + period));
    double multiplier = smoothing > 0 ? smoothing : (2.0 / (period + 1.0));
    
    for (size_t i = period; i < data.size(); ++i) {
        ema = (data[i] - ema) * multiplier + ema;
    }
    
    return ema;
}

double BloombergAnalyticsEngine::calculateSimpleAverage(const std::vector<double>& data) {
    if (data.empty()) return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double BloombergAnalyticsEngine::calculateStandardDeviation(const std::vector<double>& data) {
    if (data.size() < 2) return 0.0;
    
    double mean = calculateSimpleAverage(data);
    double variance = 0.0;
    
    for (double value : data) {
        variance += (value - mean) * (value - mean);
    }
    variance /= (data.size() - 1);
    
    return std::sqrt(variance);
}

std::vector<double> BloombergAnalyticsEngine::getRecentData(int count) {
    if (performanceHistory.size() <= count) {
        return performanceHistory;
    }
    
    return std::vector<double>(
        performanceHistory.end() - count,
        performanceHistory.end()
    );
}

double BloombergAnalyticsEngine::exponentialSmoothing(const std::vector<double>& data, double alpha) {
    if (data.empty()) return 0.0;
    if (data.size() == 1) return data[0];
    
    double forecast = data[0];
    for (size_t i = 1; i < data.size(); ++i) {
        forecast = alpha * data[i] + (1 - alpha) * forecast;
    }
    
    return forecast;
}

// Static helper function
double BloombergAnalyticsEngine::calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y) {
    if (x.size() != y.size() || x.empty()) return 0.0;
    
    double meanX = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
    double meanY = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
    
    double covariance = 0.0;
    double varX = 0.0;
    double varY = 0.0;
    
    for (size_t i = 0; i < x.size(); ++i) {
        double dx = x[i] - meanX;
        double dy = y[i] - meanY;
        covariance += dx * dy;
        varX += dx * dx;
        varY += dy * dy;
    }
    
    if (varX == 0 || varY == 0) return 0.0;
    
    return covariance / (std::sqrt(varX) * std::sqrt(varY));
}