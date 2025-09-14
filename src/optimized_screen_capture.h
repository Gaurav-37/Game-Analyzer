#pragma once

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <d3d11_1.h>
#include <wincodec.h>
#include <opencv2/opencv.hpp>
// CUDA support disabled for compatibility
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>

// Optimized Screen Capture with GPU acceleration and differential processing
class OptimizedScreenCapture {
public:
    enum class CaptureMode {
        FULL_DESKTOP,       // Capture entire desktop
        GAME_WINDOW,        // Capture specific game window
        REGION_BASED,       // Capture specific regions only
        DIFFERENTIAL        // Only capture changed regions
    };

    struct CaptureRegion {
        RECT rect;
        std::string name;
        int priority;       // Higher priority = more frequent capture
        bool enabled;
        std::chrono::steady_clock::time_point lastCapture;
        
        CaptureRegion() : priority(1), enabled(true) {}
        CaptureRegion(const RECT& r, const std::string& n, int p = 1) 
            : rect(r), name(n), priority(p), enabled(true) {}
    };

    struct GameWindow {
        HWND hwnd;
        std::string title;
        std::string processName;
        RECT clientRect;
        bool isValid;
        
        GameWindow() : hwnd(nullptr), isValid(false) {}
    };

    struct FrameData {
        cv::Mat frame;
        cv::cuda::GpuMat gpuFrame;
        std::vector<CaptureRegion> regions;
        int64_t timestamp;
        bool isGPU;
        
        FrameData() : timestamp(0), isGPU(false) {}
    };

private:
    // DirectX components
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dContext;
    IDXGIOutputDuplication* outputDuplication;
    IDXGIOutput1* dxgiOutput;
    IDXGISwapChain* swapChain;
    
    // GPU memory sharing
    ID3D11Texture2D* sharedTexture;
    ID3D11Texture2D* stagingTexture;
    HANDLE sharedHandle;
    
    // OpenCV GPU components
    cv::cuda::GpuMat gpuFrame;
    cv::cuda::GpuMat gpuPreviousFrame;
    cv::cuda::GpuMat gpuDiff;
    
    // Capture configuration
    CaptureMode captureMode;
    GameWindow targetWindow;
    std::vector<CaptureRegion> captureRegionsList;
    
    // Differential processing
    cv::Mat previousFrame;
    std::vector<cv::Rect> changedRegions;
    bool useDifferentialCapture;
    float changeThreshold;
    
    // Performance optimization
    bool useGPUAcceleration;
    bool useDownsampling;
    float downsamplingFactor;
    int maxFPS;
    std::chrono::steady_clock::time_point lastCaptureTime;
    
    // Threading
    std::mutex captureMutex;
    std::atomic<bool> isCapturing;
    std::thread captureThread;
    
    // Statistics
    int64_t totalFrames;
    int64_t droppedFrames;
    double averageCaptureTime;
    double averageProcessingTime;
    
public:
    OptimizedScreenCapture();
    ~OptimizedScreenCapture();
    
    // Initialization
    bool initialize(bool enableGPU = true, bool enableDifferential = true);
    void cleanup();
    
    // Capture methods
    bool captureFrame(FrameData& frameData);
    bool captureGameWindow(const std::string& windowTitle, FrameData& frameData);
    bool captureRegions(const std::vector<CaptureRegion>& regions, FrameData& frameData);
    bool captureDifferential(FrameData& frameData);
    
    // GPU-accelerated capture
    bool captureFrameGPU(FrameData& frameData);
    bool copyToGPU(const cv::Mat& cpuFrame, cv::cuda::GpuMat& gpuFrame);
    bool copyFromGPU(const cv::cuda::GpuMat& gpuFrame, cv::Mat& cpuFrame);
    
    // Window management
    bool findGameWindow(const std::string& windowTitle);
    bool setTargetWindow(HWND hwnd);
    std::vector<GameWindow> enumerateGameWindows();
    
    // Region management
    void addCaptureRegion(const CaptureRegion& region);
    void removeCaptureRegion(const std::string& name);
    void updateCaptureRegion(const std::string& name, const RECT& newRect);
    void setRegionPriority(const std::string& name, int priority);
    void enableRegion(const std::string& name, bool enable);
    
    // Configuration
    void setCaptureMode(CaptureMode mode) { captureMode = mode; }
    void setMaxFPS(int fps) { maxFPS = fps; }
    void setDownsampling(bool enable, float factor = 0.5f);
    void setChangeThreshold(float threshold) { changeThreshold = threshold; }
    
    // Performance monitoring
    double getAverageCaptureTime() const { return averageCaptureTime; }
    double getAverageProcessingTime() const { return averageProcessingTime; }
    int64_t getTotalFrames() const { return totalFrames; }
    int64_t getDroppedFrames() const { return droppedFrames; }
    float getFPS() const;
    
    // Utility functions
    static bool isGameWindow(HWND hwnd);
    static RECT getWindowClientRect(HWND hwnd);
    static std::string getWindowTitle(HWND hwnd);
    static std::string getProcessName(HWND hwnd);
    
private:
    // DirectX initialization
    bool initializeDirectX();
    bool initializeGPUSharing();
    bool createSharedTexture(int width, int height);
    
    // Capture implementations
    bool captureDesktop(FrameData& frameData);
    bool captureWindow(HWND hwnd, FrameData& frameData);
    bool captureRegion(const RECT& rect, cv::Mat& frame);
    
    // Differential processing
    std::vector<cv::Rect> detectChangedRegions(const cv::Mat& currentFrame, const cv::Mat& previousFrame);
    bool hasSignificantChange(const cv::Mat& currentFrame, const cv::Mat& previousFrame);
    
    // GPU processing
    bool processFrameGPU(const cv::cuda::GpuMat& input, cv::cuda::GpuMat& output);
    bool applyDownsamplingGPU(const cv::cuda::GpuMat& input, cv::cuda::GpuMat& output);
    
    // Performance optimization
    void updateStatistics(double captureTime, double processingTime);
    bool shouldCaptureFrame();
    void limitFPS();
    
    // Error handling
    void handleDirectXError(HRESULT hr, const std::string& operation);
    bool isDirectXDeviceLost();
    bool recoverDirectXDevice();
};

// Intelligent Region Processor
class IntelligentRegionProcessor {
public:
    enum class GameState {
        MENU,           // Main menu, settings, etc.
        GAMEPLAY,       // Active gameplay
        LOADING,        // Loading screens
        CUTSCENE,       // Cutscenes, cinematics
        PAUSED,         // Game paused
        UNKNOWN         // Unknown state
    };

    struct ProcessingRegion {
        std::string name;
        cv::Rect rect;
        int fps;                    // Target FPS for this region
        GameState requiredState;    // Only process in this state
        bool enabled;
        std::chrono::steady_clock::time_point lastProcess;
        
        ProcessingRegion() : fps(30), requiredState(GameState::GAMEPLAY), enabled(true) {}
    };

private:
    GameState currentState;
    std::vector<ProcessingRegion> regions;
    std::map<GameState, std::vector<std::string>> stateRegions;
    
    // State detection
    cv::Mat menuTemplate;
    cv::Mat loadingTemplate;
    cv::Mat gameplayTemplate;
    
    // Performance tracking
    std::map<std::string, double> regionProcessingTimes;
    std::map<std::string, int> regionFrameCounts;
    
public:
    IntelligentRegionProcessor();
    
    // State management
    GameState detectGameState(const cv::Mat& frame);
    void setGameState(GameState state) { currentState = state; }
    GameState getCurrentState() const { return currentState; }
    
    // Region management
    void addRegion(const ProcessingRegion& region);
    void removeRegion(const std::string& name);
    void updateRegionFPS(const std::string& name, int fps);
    void enableRegion(const std::string& name, bool enable);
    
    // Processing
    std::vector<ProcessingRegion> getRegionsToProcess();
    bool shouldProcessRegion(const std::string& name);
    void updateRegionLastProcess(const std::string& name);
    
    // Templates
    void loadStateTemplates();
    void saveStateTemplates();
    
    // Statistics
    double getRegionProcessingTime(const std::string& name) const;
    int getRegionFrameCount(const std::string& name) const;
    float getRegionFPS(const std::string& name) const;
    
private:
    // State detection helpers
    bool isMenuState(const cv::Mat& frame);
    bool isLoadingState(const cv::Mat& frame);
    bool isGameplayState(const cv::Mat& frame);
    bool isCutsceneState(const cv::Mat& frame);
    
    // Template matching
    float matchTemplate(const cv::Mat& frame, const cv::Mat& template_);
    void updateTemplates(const cv::Mat& frame, GameState state);
};
