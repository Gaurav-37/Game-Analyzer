#pragma once

#include <windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/cuda.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>

// Game Event Detection System
class GameEventDetector {
public:
    enum class EventType {
        DEATH,              // Player death
        LEVEL_UP,           // Level up
        DAMAGE_TAKEN,       // Damage received
        DAMAGE_DEALT,       // Damage dealt
        KILL,               // Enemy kill
        ITEM_PICKUP,        // Item collected
        ACHIEVEMENT,        // Achievement unlocked
        SCORE_CHANGE,       // Score changed
        HEALTH_CHANGE,      // Health changed
        AMMO_CHANGE,        // Ammo changed
        TIME_CHANGE,        // Time changed
        UNKNOWN             // Unknown event
    };

    struct GameEvent {
        EventType type;
        std::string description;
        int64_t timestamp;
        float confidence;
        cv::Rect region;
        std::map<std::string, float> parameters;
        
        GameEvent() : type(EventType::UNKNOWN), timestamp(0), confidence(0.0f) {}
        GameEvent(EventType t, const std::string& desc, float conf = 1.0f)
            : type(t), description(desc), confidence(conf) {
            timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }
    };

    struct VisualCue {
        std::string name;
        cv::Scalar color;
        cv::Rect region;
        float threshold;
        int duration;       // Expected duration in ms
        bool isActive;
        
        VisualCue() : threshold(0.8f), duration(1000), isActive(false) {}
    };

private:
    // Event detection components
    std::vector<VisualCue> visualCues;
    std::vector<GameEvent> detectedEvents;
    std::map<EventType, std::vector<VisualCue>> eventCues;
    
    // Optical flow for screen shake detection
    cv::Ptr<cv::cuda::GpuMat> gpuPrevFrame;
    cv::Ptr<cv::cuda::GpuMat> gpuCurrentFrame;
    cv::Ptr<cv::cuda::GpuMat> gpuFlow;
    cv::Ptr<cv::cuda::OpticalFlowDual_TVL1> opticalFlow;
    
    // Color detection for UI elements
    std::map<std::string, cv::Scalar> gameColors;
    
    // Performance tracking
    std::atomic<int> totalEventsDetected;
    std::atomic<int> falsePositives;
    double averageDetectionTime;
    
    // Threading
    std::mutex detectionMutex;
    std::thread detectionThread;
    std::atomic<bool> isDetecting;
    
public:
    GameEventDetector();
    ~GameEventDetector();
    
    // Initialization
    bool initialize();
    void cleanup();
    
    // Event detection
    std::vector<GameEvent> detectEvents(const cv::Mat& frame);
    std::vector<GameEvent> detectEventsGPU(const cv::cuda::GpuMat& gpuFrame);
    
    // Specific event detection
    std::vector<GameEvent> detectDeaths(const cv::Mat& frame);
    std::vector<GameEvent> detectLevelUps(const cv::Mat& frame);
    std::vector<GameEvent> detectDamage(const cv::Mat& frame);
    std::vector<GameEvent> detectScreenShake(const cv::Mat& frame);
    std::vector<GameEvent> detectColorChanges(const cv::Mat& frame);
    
    // Visual cue management
    void addVisualCue(const VisualCue& cue, EventType eventType);
    void removeVisualCue(const std::string& name);
    void updateVisualCue(const std::string& name, const VisualCue& newCue);
    
    // Game-specific configuration
    void loadGameProfile(const std::string& gameName);
    void saveGameProfile(const std::string& gameName);
    
    // Statistics
    int getTotalEventsDetected() const { return totalEventsDetected.load(); }
    int getFalsePositives() const { return falsePositives.load(); }
    double getDetectionAccuracy() const;
    double getAverageDetectionTime() const { return averageDetectionTime; }
    
private:
    // Detection algorithms
    bool detectRedScreenFlash(const cv::Mat& frame);
    bool detectGoldenEffects(const cv::Mat& frame);
    bool detectScreenShake(const cv::Mat& currentFrame, const cv::Mat& previousFrame);
    bool detectColorFlash(const cv::Mat& frame, const cv::Scalar& targetColor, float threshold);
    
    // Optical flow analysis
    cv::Mat calculateOpticalFlow(const cv::Mat& prevFrame, const cv::Mat& currFrame);
    float calculateMotionMagnitude(const cv::Mat& flow);
    
    // Color analysis
    cv::Scalar detectDominantColor(const cv::Mat& frame, const cv::Rect& region);
    bool isColorSimilar(const cv::Scalar& color1, const cv::Scalar& color2, float threshold);
    
    // Event validation
    bool validateEvent(const GameEvent& event, const cv::Mat& frame);
    void updateEventConfidence(GameEvent& event, const cv::Mat& frame);
};

// Game Fingerprinting System
class GameFingerprinting {
public:
    struct GameProfile {
        std::string name;
        std::string executableName;
        std::string windowTitle;
        std::string executableHash;
        cv::Mat uiTemplate;
        std::vector<cv::Scalar> colorPalette;
        std::vector<cv::Rect> uiRegions;
        std::map<std::string, std::string> metadata;
        
        GameProfile() {}
        GameProfile(const std::string& n) : name(n) {}
    };

    struct FingerprintMatch {
        GameProfile profile;
        float confidence;
        std::string matchMethod;
        
        FingerprintMatch() : confidence(0.0f) {}
        FingerprintMatch(const GameProfile& p, float c, const std::string& method)
            : profile(p), confidence(c), matchMethod(method) {}
    };

private:
    // Game database
    std::vector<GameProfile> gameDatabase;
    std::map<std::string, GameProfile> processNameMap;
    std::map<std::string, GameProfile> windowTitleMap;
    std::map<std::string, GameProfile> hashMap;
    
    // Template matching
    cv::Ptr<cv::cuda::TemplateMatching> templateMatcher;
    
    // Color analysis
    std::map<std::string, std::vector<cv::Scalar>> gameColorPalettes;
    
    // Performance tracking
    std::atomic<int> totalFingerprints;
    std::atomic<int> successfulMatches;
    double averageFingerprintTime;
    
public:
    GameFingerprinting();
    ~GameFingerprinting();
    
    // Initialization
    bool initialize();
    void cleanup();
    
    // Game identification
    FingerprintMatch identifyGame(HWND hwnd);
    FingerprintMatch identifyGame(const std::string& processName);
    FingerprintMatch identifyGame(const cv::Mat& uiScreenshot);
    
    // Database management
    void loadGameDatabase();
    void saveGameDatabase();
    void addGameProfile(const GameProfile& profile);
    void removeGameProfile(const std::string& gameName);
    void updateGameProfile(const std::string& gameName, const GameProfile& profile);
    
    // Fingerprinting methods
    std::string calculateExecutableHash(const std::string& executablePath);
    cv::Mat extractUITemplate(const cv::Mat& screenshot);
    std::vector<cv::Scalar> extractColorPalette(const cv::Mat& screenshot);
    std::vector<cv::Rect> detectUIRegions(const cv::Mat& screenshot);
    
    // Matching algorithms
    float matchByProcessName(const std::string& processName);
    float matchByWindowTitle(const std::string& windowTitle);
    float matchByExecutableHash(const std::string& hash);
    float matchByUITemplate(const cv::Mat& template_);
    float matchByColorPalette(const std::vector<cv::Scalar>& palette);
    
    // Statistics
    int getTotalFingerprints() const { return totalFingerprints.load(); }
    int getSuccessfulMatches() const { return successfulMatches.load(); }
    double getMatchAccuracy() const;
    double getAverageFingerprintTime() const { return averageFingerprintTime; }
    
    // Top 50 games database
    void loadTop50GamesDatabase();
    
private:
    // Database operations
    void loadDefaultGameProfiles();
    void saveDefaultGameProfiles();
    
    // Template matching
    float calculateTemplateSimilarity(const cv::Mat& template1, const cv::Mat& template2);
    
    // Color analysis
    float calculateColorPaletteSimilarity(const std::vector<cv::Scalar>& palette1, 
                                         const std::vector<cv::Scalar>& palette2);
    
    // Hash calculation
    std::string calculateMD5Hash(const std::string& data);
    std::string calculateFileHash(const std::string& filePath);
};

// Bloomberg-Style Analytics Engine
class BloombergAnalyticsEngine {
public:
    struct PerformanceMetrics {
        // Technical indicators
        double rsi;                 // Relative Strength Index for consistency
        double macd;                // MACD for trend analysis
        double volatility;          // Volatility measure
        double sharpeRatio;         // Risk-adjusted return
        
        // Performance indices
        double consistencyIndex;    // How consistent is performance
        double improvementIndex;    // Rate of improvement
        double stabilityIndex;      // Performance stability
        
        // Pattern detection
        std::vector<int> deathClusters;     // Death cluster analysis
        std::vector<double> performanceCycles; // Performance cycle detection
        double trendDirection;      // Overall trend direction
        
        // Predictive modeling
        double predictedPerformance; // Next performance prediction
        double confidenceInterval;   // Prediction confidence
        std::vector<double> forecast; // Future performance forecast
        
        PerformanceMetrics() : rsi(0.0), macd(0.0), volatility(0.0), sharpeRatio(0.0),
                              consistencyIndex(0.0), improvementIndex(0.0), stabilityIndex(0.0),
                              trendDirection(0.0), predictedPerformance(0.0), confidenceInterval(0.0) {}
    };

    struct TradingSignal {
        enum class SignalType {
            BUY,        // Strong performance expected
            SELL,       // Poor performance expected
            HOLD,       // Maintain current position
            STRONG_BUY, // Very strong performance expected
            STRONG_SELL // Very poor performance expected
        };
        
        SignalType type;
        double strength;        // Signal strength (0-1)
        std::string reasoning;  // Why this signal was generated
        int64_t timestamp;
        
        TradingSignal() : type(SignalType::HOLD), strength(0.0), timestamp(0) {}
    };

private:
    // Data storage
    std::vector<double> performanceHistory;
    std::vector<double> timeSeries;
    std::vector<GameEventDetector::GameEvent> eventHistory;
    
    // Technical indicators
    std::vector<double> rsiHistory;
    std::vector<double> macdHistory;
    std::vector<double> volatilityHistory;
    
    // Pattern detection
    std::vector<std::vector<int>> deathPatterns;
    std::vector<std::vector<double>> performancePatterns;
    
    // Predictive models
    std::vector<double> predictionHistory;
    std::vector<double> actualHistory;
    
    // Configuration
    int lookbackPeriod;         // Period for technical analysis
    double smoothingFactor;     // EMA smoothing factor
    double volatilityThreshold; // Volatility threshold
    
    // Performance tracking
    std::atomic<int> totalCalculations;
    double averageCalculationTime;
    
public:
    BloombergAnalyticsEngine();
    ~BloombergAnalyticsEngine();
    
    // Initialization
    bool initialize(int lookback = 20, double smoothing = 0.1);
    void cleanup();
    
    // Data input
    void addPerformanceData(double performance, int64_t timestamp);
    void addEventData(const GameEventDetector::GameEvent& event);
    void addTimeSeriesData(const std::vector<double>& data);
    
    // Technical analysis
    PerformanceMetrics calculateMetrics();
    double calculateRSI(int period = 14);
    double calculateMACD(int fastPeriod = 12, int slowPeriod = 26, int signalPeriod = 9);
    double calculateVolatility(int period = 20);
    double calculateSharpeRatio(double riskFreeRate = 0.02);
    
    // Pattern detection
    std::vector<int> detectDeathClusters();
    std::vector<double> detectPerformanceCycles();
    double calculateTrendDirection();
    
    // Predictive modeling
    double predictNextPerformance();
    std::vector<double> generateForecast(int periods = 10);
    double calculatePredictionConfidence();
    
    // Trading signals
    TradingSignal generateTradingSignal();
    std::vector<TradingSignal> generateSignalHistory(int periods = 10);
    
    // Export functionality
    std::string exportToCSV();
    std::string exportAnalyticsReport();
    std::string exportBloombergFormat();
    
    // Statistics
    int getTotalCalculations() const { return totalCalculations.load(); }
    double getAverageCalculationTime() const { return averageCalculationTime; }
    double getPredictionAccuracy() const;
    
private:
    // Technical indicator calculations
    double calculateEMA(const std::vector<double>& data, int period, double smoothing);
    double calculateSMA(const std::vector<double>& data, int period);
    double calculateStandardDeviation(const std::vector<double>& data);
    
    // Pattern analysis
    std::vector<int> findClusters(const std::vector<int>& data, int minClusterSize = 3);
    std::vector<double> findCycles(const std::vector<double>& data, int minCycleLength = 5);
    
    // Predictive algorithms
    double linearRegression(const std::vector<double>& x, const std::vector<double>& y);
    double exponentialSmoothing(const std::vector<double>& data, double alpha);
    
    // Signal generation
    TradingSignal generateSignalFromMetrics(const PerformanceMetrics& metrics);
    std::string generateSignalReasoning(const PerformanceMetrics& metrics);
    
    // Utility functions
    std::vector<double> getRecentData(int count);
    void updatePerformanceTracking(double calculationTime);
};
