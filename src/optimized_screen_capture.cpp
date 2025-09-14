#include "optimized_screen_capture.h"
#include <algorithm>
#include <chrono>
#include <thread>

// OptimizedScreenCapture Implementation
OptimizedScreenCapture::OptimizedScreenCapture() 
    : d3dDevice(nullptr), d3dContext(nullptr), outputDuplication(nullptr),
      dxgiOutput(nullptr), swapChain(nullptr), sharedTexture(nullptr),
      stagingTexture(nullptr), sharedHandle(nullptr),
      captureMode(CaptureMode::FULL_DESKTOP), useDifferentialCapture(true),
      changeThreshold(0.1f), useGPUAcceleration(true), useDownsampling(false),
      downsamplingFactor(0.5f), maxFPS(60), isCapturing(false),
      totalFrames(0), droppedFrames(0), averageCaptureTime(0.0),
      averageProcessingTime(0.0) {
    
    // Initialize GPU matrices (CPU fallback)
    gpuFrame = cv::cuda::GpuMat();
    gpuPreviousFrame = cv::cuda::GpuMat();
    gpuDiff = cv::cuda::GpuMat();
}

OptimizedScreenCapture::~OptimizedScreenCapture() {
    cleanup();
}

bool OptimizedScreenCapture::initialize(bool enableGPU, bool enableDifferential) {
    useGPUAcceleration = enableGPU;
    useDifferentialCapture = enableDifferential;
    
    if (useGPUAcceleration) {
        if (cv::cuda::getCudaEnabledDeviceCount() == 0) {
            useGPUAcceleration = false;
        }
    }
    
    if (!initializeDirectX()) {
        return false;
    }
    
    if (useGPUAcceleration && !initializeGPUSharing()) {
        useGPUAcceleration = false;
    }
    
    return true;
}

void OptimizedScreenCapture::cleanup() {
    isCapturing = false;
    
    if (captureThread.joinable()) {
        captureThread.join();
    }
    
    if (sharedTexture) {
        sharedTexture->Release();
        sharedTexture = nullptr;
    }
    
    if (stagingTexture) {
        stagingTexture->Release();
        stagingTexture = nullptr;
    }
    
    if (outputDuplication) {
        outputDuplication->Release();
        outputDuplication = nullptr;
    }
    
    if (dxgiOutput) {
        dxgiOutput->Release();
        dxgiOutput = nullptr;
    }
    
    if (swapChain) {
        swapChain->Release();
        swapChain = nullptr;
    }
    
    if (d3dContext) {
        d3dContext->Release();
        d3dContext = nullptr;
    }
    
    if (d3dDevice) {
        d3dDevice->Release();
        d3dDevice = nullptr;
    }
    
    if (sharedHandle) {
        CloseHandle(sharedHandle);
        sharedHandle = nullptr;
    }
}

bool OptimizedScreenCapture::initializeDirectX() {
    // Create D3D11 device
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION,
        &d3dDevice, &featureLevel, &d3dContext
    );
    
    if (FAILED(hr)) {
        handleDirectXError(hr, "D3D11CreateDevice");
        return false;
    }
    
    // Get DXGI device
    IDXGIDevice* dxgiDevice = nullptr;
    hr = d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) {
        handleDirectXError(hr, "QueryInterface IDXGIDevice");
        return false;
    }
    
    // Get DXGI adapter
    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) {
        handleDirectXError(hr, "GetAdapter");
        return false;
    }
    
    // Get DXGI output
    IDXGIOutput* tempOutput;
    hr = dxgiAdapter->EnumOutputs(0, &tempOutput);
    dxgiOutput = static_cast<IDXGIOutput1*>(tempOutput);
    dxgiAdapter->Release();
    if (FAILED(hr)) {
        handleDirectXError(hr, "EnumOutputs");
        return false;
    }
    
    // Create output duplication
    hr = dxgiOutput->DuplicateOutput(d3dDevice, &outputDuplication);
    if (FAILED(hr)) {
        handleDirectXError(hr, "DuplicateOutput");
        return false;
    }
    
    return true;
}

bool OptimizedScreenCapture::initializeGPUSharing() {
    if (!d3dDevice) return false;
    
    // Create shared texture for GPU memory sharing
    DXGI_OUTPUT_DESC outputDesc;
    dxgiOutput->GetDesc(&outputDesc);
    
    return createSharedTexture(outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left,
                              outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top);
}

bool OptimizedScreenCapture::createSharedTexture(int width, int height) {
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    textureDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    
    HRESULT hr = d3dDevice->CreateTexture2D(&textureDesc, nullptr, &sharedTexture);
    if (FAILED(hr)) {
        handleDirectXError(hr, "CreateTexture2D");
        return false;
    }
    
    // Get shared handle
    IDXGIResource* dxgiResource = nullptr;
    hr = sharedTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&dxgiResource);
    if (FAILED(hr)) {
        handleDirectXError(hr, "QueryInterface IDXGIResource");
        return false;
    }
    
    hr = dxgiResource->GetSharedHandle(&sharedHandle);
    dxgiResource->Release();
    if (FAILED(hr)) {
        handleDirectXError(hr, "GetSharedHandle");
        return false;
    }
    
    return true;
}

bool OptimizedScreenCapture::captureFrame(FrameData& frameData) {
    if (!shouldCaptureFrame()) {
        droppedFrames++;
        return false;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    bool success = false;
    switch (captureMode) {
        case CaptureMode::FULL_DESKTOP:
            success = captureDesktop(frameData);
            break;
        case CaptureMode::GAME_WINDOW:
            success = captureWindow(targetWindow.hwnd, frameData);
            break;
        case CaptureMode::REGION_BASED:
            success = captureRegions(captureRegionsList, frameData);
            break;
        case CaptureMode::DIFFERENTIAL:
            success = captureDifferential(frameData);
            break;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    double captureTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    if (success) {
        totalFrames++;
        updateStatistics(captureTime, 0.0);
        limitFPS();
    } else {
        droppedFrames++;
    }
    
    return success;
}

bool OptimizedScreenCapture::captureDesktop(FrameData& frameData) {
    if (!outputDuplication) return false;
    
    IDXGIResource* desktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    
    HRESULT hr = outputDuplication->AcquireNextFrame(0, &frameInfo, &desktopResource);
    if (FAILED(hr)) {
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return false; // No new frame available
        }
        handleDirectXError(hr, "AcquireNextFrame");
        return false;
    }
    
    ID3D11Texture2D* desktopTexture = nullptr;
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktopTexture);
    desktopResource->Release();
    
    if (FAILED(hr)) {
        handleDirectXError(hr, "QueryInterface ID3D11Texture2D");
        return false;
    }
    
    // Copy to staging texture for CPU access
    D3D11_TEXTURE2D_DESC desc;
    desktopTexture->GetDesc(&desc);
    
    if (!stagingTexture) {
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        
        hr = d3dDevice->CreateTexture2D(&desc, nullptr, &stagingTexture);
        if (FAILED(hr)) {
            desktopTexture->Release();
            handleDirectXError(hr, "CreateTexture2D staging");
            return false;
        }
    }
    
    d3dContext->CopyResource(stagingTexture, desktopTexture);
    desktopTexture->Release();
    
    // Map staging texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        handleDirectXError(hr, "Map staging texture");
        return false;
    }
    
    // Create OpenCV Mat from mapped data
    cv::Mat frame(desc.Height, desc.Width, CV_8UC4, mappedResource.pData, mappedResource.RowPitch);
    frameData.frame = frame.clone();
    
    d3dContext->Unmap(stagingTexture, 0);
    outputDuplication->ReleaseFrame();
    
    // Convert BGRA to BGR
    cv::cvtColor(frameData.frame, frameData.frame, cv::COLOR_BGRA2BGR);
    
    // Apply downsampling if enabled
    if (useDownsampling) {
        cv::resize(frameData.frame, frameData.frame, cv::Size(), downsamplingFactor, downsamplingFactor);
    }
    
    frameData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    return true;
}

bool OptimizedScreenCapture::captureWindow(HWND hwnd, FrameData& frameData) {
    if (!hwnd || !IsWindow(hwnd)) return false;
    
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    
    int width = windowRect.right - windowRect.left;
    int height = windowRect.bottom - windowRect.top;
    
    if (width <= 0 || height <= 0) return false;
    
    HDC hdcScreen = GetDC(nullptr);
    HDC hdcWindow = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcWindow, hBitmap);
    
    // Capture window
    PrintWindow(hwnd, hdcWindow, PW_CLIENTONLY);
    
    // Convert to OpenCV Mat
    BITMAPINFOHEADER bih = {};
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height; // Negative for top-down bitmap
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    
    frameData.frame = cv::Mat(height, width, CV_8UC3);
    GetDIBits(hdcScreen, hBitmap, 0, height, frameData.frame.data, (BITMAPINFO*)&bih, DIB_RGB_COLORS);
    
    // Cleanup
    SelectObject(hdcWindow, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcWindow);
    ReleaseDC(nullptr, hdcScreen);
    
    frameData.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    return true;
}

bool OptimizedScreenCapture::captureRegions(const std::vector<CaptureRegion>& regions, FrameData& frameData) {
    if (regions.empty()) return false;
    
    // Capture full desktop first
    FrameData desktopFrame;
    if (!captureDesktop(desktopFrame)) {
        return false;
    }
    
    frameData.frame = cv::Mat::zeros(desktopFrame.frame.size(), desktopFrame.frame.type());
    frameData.regions = regions;
    
    for (const auto& region : regions) {
        if (!region.enabled) continue;
        
        cv::Rect cvRect(region.rect.left, region.rect.top, 
                       region.rect.right - region.rect.left,
                       region.rect.bottom - region.rect.top);
        
        if (cvRect.x >= 0 && cvRect.y >= 0 && 
            cvRect.x + cvRect.width <= desktopFrame.frame.cols &&
            cvRect.y + cvRect.height <= desktopFrame.frame.rows) {
            
            cv::Mat roi = desktopFrame.frame(cvRect);
            roi.copyTo(frameData.frame(cvRect));
        }
    }
    
    frameData.timestamp = desktopFrame.timestamp;
    return true;
}

bool OptimizedScreenCapture::captureDifferential(FrameData& frameData) {
    if (!useDifferentialCapture) {
        return captureDesktop(frameData);
    }
    
    // Capture current frame
    FrameData currentFrame;
    if (!captureDesktop(currentFrame)) {
        return false;
    }
    
    if (previousFrame.empty()) {
        // First frame - no differential processing
        previousFrame = currentFrame.frame.clone();
        frameData = currentFrame;
        return true;
    }
    
    // Detect changed regions
    changedRegions = detectChangedRegions(currentFrame.frame, previousFrame);
    
    if (changedRegions.empty()) {
        // No changes detected
        frameData = currentFrame;
        frameData.frame = previousFrame.clone();
        return true;
    }
    
    // Create frame with only changed regions
    frameData.frame = previousFrame.clone();
    for (const auto& region : changedRegions) {
        if (region.x >= 0 && region.y >= 0 && 
            region.x + region.width <= currentFrame.frame.cols &&
            region.y + region.height <= currentFrame.frame.rows) {
            
            cv::Mat roi = currentFrame.frame(region);
            roi.copyTo(frameData.frame(region));
        }
    }
    
    // Update previous frame
    previousFrame = currentFrame.frame.clone();
    frameData.timestamp = currentFrame.timestamp;
    
    return true;
}

std::vector<cv::Rect> OptimizedScreenCapture::detectChangedRegions(const cv::Mat& currentFrame, const cv::Mat& previousFrame) {
    std::vector<cv::Rect> regions;
    
    if (currentFrame.size() != previousFrame.size() || currentFrame.type() != previousFrame.type()) {
        return regions;
    }
    
    cv::Mat diff;
    cv::absdiff(currentFrame, previousFrame, diff);
    
    cv::Mat gray;
    if (diff.channels() > 1) {
        cv::cvtColor(diff, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = diff;
    }
    
    cv::Mat thresh;
    cv::threshold(gray, thresh, 30, 255, cv::THRESH_BINARY);
    
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : contours) {
        cv::Rect boundingRect = cv::boundingRect(contour);
        
        // Filter small regions
        if (boundingRect.area() > 100) {
            regions.push_back(boundingRect);
        }
    }
    
    return regions;
}

bool OptimizedScreenCapture::findGameWindow(const std::string& windowTitle) {
    std::vector<GameWindow> windows = enumerateGameWindows();
    
    for (const auto& window : windows) {
        if (window.title.find(windowTitle) != std::string::npos) {
            targetWindow = window;
            return true;
        }
    }
    
    return false;
}

std::vector<OptimizedScreenCapture::GameWindow> OptimizedScreenCapture::enumerateGameWindows() {
    std::vector<GameWindow> windows;
    
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        std::vector<GameWindow>* windows = reinterpret_cast<std::vector<GameWindow>*>(lParam);
        
        if (isGameWindow(hwnd)) {
            GameWindow window;
            window.hwnd = hwnd;
            window.title = getWindowTitle(hwnd);
            window.processName = getProcessName(hwnd);
            window.clientRect = getWindowClientRect(hwnd);
            window.isValid = true;
            
            windows->push_back(window);
        }
        
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));
    
    return windows;
}

bool OptimizedScreenCapture::isGameWindow(HWND hwnd) {
    if (!IsWindowVisible(hwnd)) return false;
    
    char className[256];
    GetClassNameA(hwnd, className, sizeof(className));
    
    // Filter out system windows
    std::string classNameStr(className);
    if (classNameStr.find("Shell_") != std::string::npos ||
        classNameStr.find("Progman") != std::string::npos ||
        classNameStr.find("WorkerW") != std::string::npos) {
        return false;
    }
    
    // Check if window has a title
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    if (strlen(title) == 0) return false;
    
    return true;
}

RECT OptimizedScreenCapture::getWindowClientRect(HWND hwnd) {
    RECT rect = {};
    GetClientRect(hwnd, &rect);
    return rect;
}

std::string OptimizedScreenCapture::getWindowTitle(HWND hwnd) {
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    return std::string(title);
}

std::string OptimizedScreenCapture::getProcessName(HWND hwnd) {
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

void OptimizedScreenCapture::addCaptureRegion(const CaptureRegion& region) {
    captureRegionsList.push_back(region);
}

void OptimizedScreenCapture::removeCaptureRegion(const std::string& name) {
    captureRegionsList.erase(
        std::remove_if(captureRegionsList.begin(), captureRegionsList.end(),
            [&name](const CaptureRegion& region) {
                return region.name == name;
            }),
        captureRegionsList.end()
    );
}

void OptimizedScreenCapture::updateCaptureRegion(const std::string& name, const RECT& newRect) {
    for (auto& region : captureRegionsList) {
        if (region.name == name) {
            region.rect = newRect;
            break;
        }
    }
}

void OptimizedScreenCapture::setRegionPriority(const std::string& name, int priority) {
    for (auto& region : captureRegionsList) {
        if (region.name == name) {
            region.priority = priority;
            break;
        }
    }
}

void OptimizedScreenCapture::enableRegion(const std::string& name, bool enable) {
    for (auto& region : captureRegionsList) {
        if (region.name == name) {
            region.enabled = enable;
            break;
        }
    }
}

void OptimizedScreenCapture::setDownsampling(bool enable, float factor) {
    useDownsampling = enable;
    downsamplingFactor = factor;
}

float OptimizedScreenCapture::getFPS() const {
    if (totalFrames == 0) return 0.0f;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastCaptureTime).count();
    
    if (duration > 0) {
        return static_cast<float>(totalFrames) / duration;
    }
    
    return 0.0f;
}

void OptimizedScreenCapture::updateStatistics(double captureTime, double processingTime) {
    averageCaptureTime = (averageCaptureTime * (totalFrames - 1) + captureTime) / totalFrames;
    averageProcessingTime = (averageProcessingTime * (totalFrames - 1) + processingTime) / totalFrames;
}

bool OptimizedScreenCapture::shouldCaptureFrame() {
    if (maxFPS <= 0) return true;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCaptureTime).count();
    
    return duration >= (1000 / maxFPS);
}

void OptimizedScreenCapture::limitFPS() {
    if (maxFPS > 0) {
        auto now = std::chrono::steady_clock::now();
        auto targetDuration = std::chrono::milliseconds(1000 / maxFPS);
        auto elapsed = now - lastCaptureTime;
        
        if (elapsed < targetDuration) {
            std::this_thread::sleep_for(targetDuration - elapsed);
        }
        
        lastCaptureTime = std::chrono::steady_clock::now();
    }
}

void OptimizedScreenCapture::handleDirectXError(HRESULT hr, const std::string& operation) {
    // Log DirectX errors (simplified implementation)
    // In a real implementation, you'd want proper logging
}

bool OptimizedScreenCapture::isDirectXDeviceLost() {
    // Check if DirectX device is lost
    return false; // Simplified implementation
}

bool OptimizedScreenCapture::recoverDirectXDevice() {
    // Attempt to recover from device loss
    return false; // Simplified implementation
}

// IntelligentRegionProcessor Implementation
IntelligentRegionProcessor::IntelligentRegionProcessor() 
    : currentState(GameState::UNKNOWN) {
    loadStateTemplates();
}

IntelligentRegionProcessor::GameState IntelligentRegionProcessor::detectGameState(const cv::Mat& frame) {
    if (isMenuState(frame)) {
        currentState = GameState::MENU;
    } else if (isLoadingState(frame)) {
        currentState = GameState::LOADING;
    } else if (isGameplayState(frame)) {
        currentState = GameState::GAMEPLAY;
    } else if (isCutsceneState(frame)) {
        currentState = GameState::CUTSCENE;
    } else {
        currentState = GameState::UNKNOWN;
    }
    
    return currentState;
}

bool IntelligentRegionProcessor::isMenuState(const cv::Mat& frame) {
    // Detect menu state by looking for UI elements
    // This is a simplified implementation
    return false;
}

bool IntelligentRegionProcessor::isLoadingState(const cv::Mat& frame) {
    // Detect loading state by looking for loading indicators
    // This is a simplified implementation
    return false;
}

bool IntelligentRegionProcessor::isGameplayState(const cv::Mat& frame) {
    // Detect gameplay state by looking for game UI elements
    // This is a simplified implementation
    return true; // Default to gameplay
}

bool IntelligentRegionProcessor::isCutsceneState(const cv::Mat& frame) {
    // Detect cutscene state by looking for cinematic elements
    // This is a simplified implementation
    return false;
}

void IntelligentRegionProcessor::addRegion(const ProcessingRegion& region) {
    regions.push_back(region);
}

std::vector<IntelligentRegionProcessor::ProcessingRegion> IntelligentRegionProcessor::getRegionsToProcess() {
    std::vector<ProcessingRegion> regionsToProcess;
    
    for (const auto& region : regions) {
        if (region.enabled && region.requiredState == currentState) {
            regionsToProcess.push_back(region);
        }
    }
    
    return regionsToProcess;
}

void IntelligentRegionProcessor::loadStateTemplates() {
    // Load templates for state detection
    // This would typically load from files
}

void IntelligentRegionProcessor::saveStateTemplates() {
    // Save templates for state detection
    // This would typically save to files
}
