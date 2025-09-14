#include <windows.h>
#include <commctrl.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
#include <ctime>
#include <unistd.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <map>
#include <cstdarg>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <dwmapi.h>
#include "ui_framework.h"
#include "popup_dialogs.h"
#include "advanced_ocr.h"
#include "optimized_screen_capture.h"
#include "game_analytics.h"
#include "thread_manager.h"


// Real process information structure
struct ProcessInfo {
    DWORD pid;
    std::string name;
    std::string windowTitle;
    
    ProcessInfo(DWORD p, const std::string& n, const std::string& w = "") 
        : pid(p), name(n), windowTitle(w) {}
};

// Real memory reading functionality
class MemoryReader {
public:
    static bool readMemory(DWORD pid, uintptr_t address, void* buffer, size_t size) {
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return false;
        }
        
        SIZE_T bytesRead;
        bool success = ReadProcessMemory(hProcess, (LPCVOID)address, buffer, size, &bytesRead);
        CloseHandle(hProcess);
        
        return success && (bytesRead == size);
    }
};

// Memory region information
struct MemoryRegion {
    uintptr_t baseAddress;
    size_t size;
    DWORD protection;
    DWORD state;
    DWORD type;
    std::string regionType;
    
    MemoryRegion(uintptr_t addr, size_t sz, DWORD prot, DWORD st, DWORD ty) 
        : baseAddress(addr), size(sz), protection(prot), state(st), type(ty) {
        // Determine region type based on protection flags
        if (protection & PAGE_EXECUTE_READWRITE) regionType = "Executable Read/Write";
        else if (protection & PAGE_EXECUTE_READ) regionType = "Executable Read";
        else if (protection & PAGE_READWRITE) regionType = "Read/Write";
        else if (protection & PAGE_READONLY) regionType = "Read Only";
        else if (protection & PAGE_NOACCESS) regionType = "No Access";
        else regionType = "Unknown";
    }
};

// Screen capture functionality using Windows Graphics Capture API
class ScreenCapture {
private:
    ID3D11Device* d3dDevice;
    ID3D11DeviceContext* d3dContext;
    IDXGIOutputDuplication* outputDuplication;
    DXGI_OUTPUT_DESC outputDesc;
    bool initialized;
    
public:
    ScreenCapture() : d3dDevice(nullptr), d3dContext(nullptr), outputDuplication(nullptr), initialized(false) {}
    
    ~ScreenCapture() {
        cleanup();
    }
    
    bool initialize() {
        if (initialized) return true;
        
        // Create D3D11 device
        D3D_FEATURE_LEVEL featureLevel;
        HRESULT hr = D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            nullptr, 0, D3D11_SDK_VERSION,
            &d3dDevice, &featureLevel, &d3dContext
        );
        
        if (FAILED(hr)) {
            return false;
        }
        
        // Get DXGI device
        IDXGIDevice* dxgiDevice;
        hr = d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        if (FAILED(hr)) {
            return false;
        }
        
        // Get DXGI adapter
        IDXGIAdapter* dxgiAdapter;
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        dxgiDevice->Release();
        if (FAILED(hr)) {
            return false;
        }
        
        // Get primary output
        IDXGIOutput* dxgiOutput;
        hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
        dxgiAdapter->Release();
        if (FAILED(hr)) {
            return false;
        }
        
        // Get output description
        dxgiOutput->GetDesc(&outputDesc);
        
        // Get DXGI Output1 interface for DuplicateOutput
        IDXGIOutput1* dxgiOutput1;
        hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);
        dxgiOutput->Release();
        if (FAILED(hr)) {
            return false;
        }
        
        // Create output duplication
        hr = dxgiOutput1->DuplicateOutput(d3dDevice, &outputDuplication);
        dxgiOutput1->Release();
        if (FAILED(hr)) {
            return false;
        }
        
        initialized = true;
        return true;
    }
    
    bool captureFrame(std::vector<uint8_t>& frameData, int& width, int& height) {
        if (!initialized || !outputDuplication) {
            return false;
        }
        
        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        IDXGIResource* desktopResource;
        
        // Get next frame
        HRESULT hr = outputDuplication->AcquireNextFrame(0, &frameInfo, &desktopResource);
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
            return false; // No new frame available
        }
        if (FAILED(hr)) {
            return false;
        }
        
        // Get texture from resource
        ID3D11Texture2D* desktopTexture;
        hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktopTexture);
        desktopResource->Release();
        if (FAILED(hr)) {
            outputDuplication->ReleaseFrame();
            return false;
        }
        
        // Get texture description
        D3D11_TEXTURE2D_DESC desc;
        desktopTexture->GetDesc(&desc);
        
        width = desc.Width;
        height = desc.Height;
        
        // Create staging texture for CPU access
        D3D11_TEXTURE2D_DESC stagingDesc = desc;
        stagingDesc.Usage = D3D11_USAGE_STAGING;
        stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        stagingDesc.MiscFlags = 0;
        stagingDesc.BindFlags = 0;
        
        ID3D11Texture2D* stagingTexture;
        hr = d3dDevice->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
        if (FAILED(hr)) {
            desktopTexture->Release();
            outputDuplication->ReleaseFrame();
            return false;
        }
        
        // Copy to staging texture
        d3dContext->CopyResource(stagingTexture, desktopTexture);
        desktopTexture->Release();
        
        // Map and read pixel data
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = d3dContext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            // Calculate frame size (assuming RGBA format)
            size_t frameSize = width * height * 4;
            frameData.resize(frameSize);
            
            // Copy pixel data
            uint8_t* src = static_cast<uint8_t*>(mappedResource.pData);
            uint8_t* dst = frameData.data();
            
            for (int y = 0; y < height; y++) {
                memcpy(dst + y * width * 4, src + y * mappedResource.RowPitch, width * 4);
            }
            
            d3dContext->Unmap(stagingTexture, 0);
        }
        
        stagingTexture->Release();
        outputDuplication->ReleaseFrame();
        
        return SUCCEEDED(hr);
    }
    
    void cleanup() {
        if (outputDuplication) {
            outputDuplication->Release();
            outputDuplication = nullptr;
        }
        if (d3dContext) {
            d3dContext->Release();
            d3dContext = nullptr;
        }
        if (d3dDevice) {
            d3dDevice->Release();
            d3dDevice = nullptr;
        }
        initialized = false;
    }
};

// Basic OCR functionality for game UI text recognition
class SimpleOCR {
private:
    struct TextRegion {
        int x, y, width, height;
        std::string text;
        float confidence;
    };
    
    std::vector<TextRegion> detectedText;
    
public:
    SimpleOCR() {}
    
    // Detect text regions in captured frame
    std::vector<TextRegion> detectText(const std::vector<uint8_t>& frameData, int width, int height) {
        detectedText.clear();
        
        // Simple text detection based on pixel patterns
        // This is a placeholder for more sophisticated OCR
        
        // Look for bright regions that might be text
        for (int y = 0; y < height - 20; y += 10) {
            for (int x = 0; x < width - 50; x += 10) {
                if (isTextRegion(frameData, width, height, x, y)) {
                    TextRegion region;
                    region.x = x;
                    region.y = y;
                    region.width = 40;
                    region.height = 20;
                    region.text = recognizeText(frameData, width, height, x, y);
                    region.confidence = 0.8f; // Placeholder confidence
                    
                    if (!region.text.empty()) {
                        detectedText.push_back(region);
                    }
                }
            }
        }
        
        return detectedText;
    }
    
private:
    bool isTextRegion(const std::vector<uint8_t>& frameData, int width, int height, int x, int y) {
        // Simple heuristic: look for regions with high contrast and bright pixels
        int brightPixels = 0;
        int totalPixels = 0;
        
        for (int dy = 0; dy < 20 && y + dy < height; dy++) {
            for (int dx = 0; dx < 40 && x + dx < width; dx++) {
                int pixelIndex = ((y + dy) * width + (x + dx)) * 4;
                if (pixelIndex + 2 < frameData.size()) {
                    uint8_t r = frameData[pixelIndex];
                    uint8_t g = frameData[pixelIndex + 1];
                    uint8_t b = frameData[pixelIndex + 2];
                    
                    // Check if pixel is bright (potential text)
                    if (r > 150 || g > 150 || b > 150) {
                        brightPixels++;
                    }
                    totalPixels++;
                }
            }
        }
        
        // Text regions typically have 20-60% bright pixels
        if (totalPixels > 0) {
            float brightRatio = (float)brightPixels / totalPixels;
            return brightRatio > 0.2f && brightRatio < 0.6f;
        }
        
        return false;
    }
    
    std::string recognizeText(const std::vector<uint8_t>& frameData, int width, int height, int x, int y) {
        // Placeholder text recognition
        // In a real implementation, this would use OCR libraries like Tesseract
        
        // Simple pattern matching for common game text
        std::string text;
        
        // Look for numeric patterns (common in games)
        if (containsNumericPattern(frameData, width, height, x, y)) {
            text = extractNumericValue(frameData, width, height, x, y);
        }
        // Look for common game UI labels
        else if (containsLabelPattern(frameData, width, height, x, y)) {
            text = extractLabel(frameData, width, height, x, y);
        }
        
        return text;
    }
    
    bool containsNumericPattern(const std::vector<uint8_t>& frameData, int width, int height, int x, int y) {
        // Simple check for numeric patterns (placeholder)
        // Real OCR would analyze character shapes
        return true; // Placeholder
    }
    
    std::string extractNumericValue(const std::vector<uint8_t>& frameData, int width, int height, int x, int y) {
        // Placeholder: return a sample numeric value
        return "100"; // Placeholder
    }
    
    bool containsLabelPattern(const std::vector<uint8_t>& frameData, int width, int height, int x, int y) {
        // Placeholder for label detection
        return false;
    }
    
    std::string extractLabel(const std::vector<uint8_t>& frameData, int width, int height, int x, int y) {
        // Placeholder for label extraction
        return "Health"; // Placeholder
    }
};

// Memory scanning functionality
class MemoryScanner {
public:
    static std::vector<MemoryRegion> scanProcessMemory(DWORD pid) {
        std::vector<MemoryRegion> regions;
        
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return regions;
        }
        
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t address = 0;
        
        while (VirtualQueryEx(hProcess, (LPCVOID)address, &mbi, sizeof(mbi))) {
            // Only include committed memory regions that are readable
            if (mbi.State == MEM_COMMIT && 
                (mbi.Protect & PAGE_READONLY || 
                 mbi.Protect & PAGE_READWRITE || 
                 mbi.Protect & PAGE_EXECUTE_READ || 
                 mbi.Protect & PAGE_EXECUTE_READWRITE)) {
                
                regions.emplace_back((uintptr_t)mbi.BaseAddress, mbi.RegionSize, 
                                   mbi.Protect, mbi.State, mbi.Type);
            }
            
            address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            
            // Prevent infinite loop
            if (address == 0) break;
        }
        
        CloseHandle(hProcess);
        return regions;
    }
    
    static std::string addressToString(uintptr_t address) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << address;
        return ss.str();
    }
    
    static std::vector<uintptr_t> findReadableAddresses(DWORD pid, const std::vector<MemoryRegion>& regions, int maxAddresses = 100) {
        std::vector<uintptr_t> addresses;
        
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return addresses;
        }
        
        for (const auto& region : regions) {
            if (addresses.size() >= maxAddresses) break;
            
            // Focus on larger regions that are more likely to contain game data
            if (region.size < 4096) continue; // Skip small regions
            
            // Sample addresses within the region more densely
            size_t step = std::max(region.size / 100, (size_t)4); // Sample every 4 bytes or region size / 100
            for (uintptr_t addr = region.baseAddress; 
                 addr < region.baseAddress + region.size && addresses.size() < maxAddresses; 
                 addr += step) {
                
                // Try to read a 4-byte value at this address
                int32_t testValue;
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, (LPCVOID)addr, &testValue, sizeof(testValue), &bytesRead) &&
                    bytesRead == sizeof(testValue)) {
                    
                    // Prefer addresses with non-zero values that look like game data
                    if (testValue != 0 && testValue != -1 && 
                        testValue >= -1000000 && testValue <= 1000000) {
                        addresses.push_back(addr);
                    }
                }
            }
        }
        
        CloseHandle(hProcess);
        return addresses;
    }
    
    static std::vector<uintptr_t> findGameDataAddresses(DWORD pid, const std::vector<MemoryRegion>& regions, int maxAddresses = 200) {
        std::vector<uintptr_t> addresses;
        
        HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return addresses;
        }
        
        // Focus on regions that are more likely to contain game data
        for (const auto& region : regions) {
            if (addresses.size() >= maxAddresses) break;
            
            // Skip very small regions and system regions
            if (region.size < 8192) continue;
            
            // Prefer regions with read/write access (more likely to be game data)
            if (!(region.protection & PAGE_READWRITE)) continue;
            
            // Sample more densely in promising regions
            size_t step = std::max(region.size / 300, (size_t)4); // Increased density
            for (uintptr_t addr = region.baseAddress; 
                 addr < region.baseAddress + region.size && addresses.size() < maxAddresses; 
                 addr += step) {
                
                // Try to read a 4-byte value at this address
                int32_t testValue;
                SIZE_T bytesRead;
                if (ReadProcessMemory(hProcess, (LPCVOID)addr, &testValue, sizeof(testValue), &bytesRead) &&
                    bytesRead == sizeof(testValue)) {
                    
                    // Enhanced game data detection with better heuristics
                    bool isGameData = false;
                    
                    // Health/Ammo/Score values (0-1000)
                    if (testValue >= 0 && testValue <= 1000) {
                        isGameData = true;
                    }
                    // Coordinates and offsets (-1000 to 1000)
                    else if (testValue >= -1000 && testValue <= 1000) {
                        isGameData = true;
                    }
                    // High scores and currency (1000-999999)
                    else if (testValue >= 1000 && testValue <= 999999) {
                        isGameData = true;
                    }
                    // Boolean flags (0, 1)
                    else if (testValue == 0 || testValue == 1) {
                        isGameData = true;
                    }
                    // Round numbers (common in games)
                    else if (testValue > 0 && (testValue % 10 == 0 || testValue % 100 == 0)) {
                        isGameData = true;
                    }
                    // Time values (seconds)
                    else if (testValue > 0 && testValue < 86400) {
                        isGameData = true;
                    }
                    
                    // Avoid obviously non-game values
                    if (isGameData && testValue != -1 && 
                        testValue != 0xFFFFFFFF && testValue != 0xCCCCCCCC) {
                        addresses.push_back(addr);
                    }
                }
            }
        }
        
        CloseHandle(hProcess);
        return addresses;
    }
    
    static std::string interpretValue(int32_t value) {
        std::stringstream ss;
        
        // Enhanced game value interpretation with better categorization
        if (value >= 0 && value <= 100) {
            ss << " [Health/Ammo/Percentage]";
        } else if (value >= 0 && value <= 1000) {
            ss << " [Score/Count/Points]";
        } else if (value >= 1000 && value <= 999999) {
            ss << " [High Score/Currency]";
        } else if (value >= -100 && value <= 100) {
            ss << " [Coordinate/Offset]";
        } else if (value >= -1000 && value <= 1000) {
            ss << " [Stat/Modifier]";
        } else if (value >= 1000000) {
            ss << " [Large Value/ID]";
        }
        
        // Check for common game patterns
        if (value == 0) {
            ss << " [Zero/Empty]";
        } else if (value == 1) {
            ss << " [Boolean/Flag]";
        } else if (value == 100 || value == 1000 || value == 10000) {
            ss << " [Round Number]";
        } else if (value % 10 == 0 && value > 0 && value <= 1000) {
            ss << " [Even/Increment]";
        }
        
        // Check if it's a valid ASCII character sequence
        if (value >= 32 && value <= 126) {
            char c = (char)value;
            ss << " [ASCII: '" << c << "']";
        }
        
        // Check if it could be a floating point (very rough check)
        float floatVal = *(float*)&value;
        if (floatVal >= -1000.0f && floatVal <= 1000.0f && floatVal == floatVal) { // NaN check: NaN != NaN
            ss << " [Float: " << std::fixed << std::setprecision(2) << floatVal << "]";
        }
        
        // Check for time-related values (common in games)
        if (value > 0 && value < 86400) { // Less than 24 hours in seconds
            int hours = value / 3600;
            int minutes = (value % 3600) / 60;
            int seconds = value % 60;
            ss << " [Time: " << hours << "h" << minutes << "m" << seconds << "s]";
        }
        
        // Check for memory addresses (common in game engines)
        if (value >= 0x10000000 && value <= 0x7FFFFFFF) {
            ss << " [Possible Address]";
        }
        
        return ss.str();
    }
};

// Enhanced GUI with real process functionality
class RealGameAnalyzerGUI {
private:
    HWND hwnd;
    HWND hListBox;
    HWND hStartButton;
    HWND hExportButton;
    HWND hRefreshButton;
    HWND hStatusLabel;
    HWND hMemList;
    HWND hSearchEdit;
    HWND hScanButton;
    HWND hAddressListBox;
    HWND hAddSelectedButton;
    HWND hProgressBar;
    HWND hCompareButton;
    HWND hValueChangeButton;
    HWND hLoadGameProfileButton;
    HWND hSaveGameProfileButton;
    HWND hScanGameDataButton;
    HWND hAddressSearchEdit;
    HWND hListProfilesButton;
    HWND hCaptureScreenButton;
    HWND hStartVisionAnalysisButton;
    HWND hVisionStatusLabel;
    HWND hHybridAnalysisButton;
    HWND hCompareMemoryVisionButton;
    HWND hAnalyticsButton;
    HWND hShowMetricsButton;
    HWND hExportAnalyticsButton;
    HWND hDashboardButton;
    HWND hRealTimeChartsButton;
    HWND hPerformanceMonitorButton;
    HWND hAboutButton;
    HWND hSettingsButton;
    HWND hHelpButton;
    
    // Top Ribbon Controls
    HWND hRibbonPanel;
    HWND hDarkModeToggle;
    HWND hSystemProcessesToggle;
    HWND hRefreshRibbonButton;
    HWND hQuickSettingsButton;
    
    // Modern UI state
    bool modernThemeEnabled = true;
    HBRUSH hBackgroundBrush;
    HBRUSH hPanelBrush;
    HBRUSH hCardBrush;
    HFONT hModernFont;
    HFONT hBoldFont;
    HFONT hHeaderFont;
    HWND hTooltipWindow;
    
    // Custom painting brushes
    HBRUSH hButtonHoverBrush;
    HBRUSH hButtonPressedBrush;
    HBRUSH hBorderBrush;
    
    std::vector<ProcessInfo> processes;
    std::vector<ProcessInfo*> filteredProcesses;
    std::vector<std::pair<std::string, uintptr_t>> memoryAddresses;
    std::vector<MemoryRegion> memoryRegions;
    std::vector<uintptr_t> discoveredAddresses;
    std::map<uintptr_t, int32_t> previousValues;
    std::vector<bool> addressChecked;
    std::atomic<bool> monitoring;
    std::atomic<bool> scanning;
    std::atomic<bool> visionAnalyzing;
    ProcessInfo* selectedProcess;
    bool showSystemProcesses;
    
    // Advanced screen capture and vision analysis
    OptimizedScreenCapture optimizedScreenCapture;
    AdvancedOCR advancedOCR;
    FrameCache frameCache;
    IntelligentRegionProcessor regionProcessor;
    
    // Game analytics and event detection
    GameEventDetector gameEventDetector;
    GameFingerprinting gameFingerprinting;
    BloombergAnalyticsEngine bloombergAnalytics;
    
    // Thread management
    ThreadManager threadManager;
    SmartDialogManager dialogManager;
    
    // Legacy compatibility
    std::vector<uint8_t> lastFrameData;
    int frameWidth, frameHeight;
    std::vector<std::string> detectedTexts;
    
    // Analytics engine
    struct PerformanceMetrics {
        std::vector<float> memoryValues;
        std::vector<float> visionConfidence;
        std::vector<int64_t> timestamps;
        float averageMemoryStability;
        float averageVisionAccuracy;
        int totalAnalysisRuns;
        std::map<std::string, int> valueChangeCounts;
        std::map<std::string, float> valueTrends;
    } analytics;
    
public:
    RealGameAnalyzerGUI() : hwnd(nullptr), monitoring(false), scanning(false), visionAnalyzing(false), selectedProcess(nullptr), showSystemProcesses(false), frameWidth(0), frameHeight(0), hBackgroundBrush(nullptr), hPanelBrush(nullptr), hCardBrush(nullptr), hModernFont(nullptr), hBoldFont(nullptr), hHeaderFont(nullptr), hTooltipWindow(nullptr), hButtonHoverBrush(nullptr), hButtonPressedBrush(nullptr), hBorderBrush(nullptr) {
        refreshProcesses();
        initializeModernUI();
    }
    
    ~RealGameAnalyzerGUI() {
        cleanupModernUI();
    }
    
    void initializeModernUI() {
        // Create modern brushes - will be updated based on theme
        updateThemeBrushes();
        
        // Create professional fonts matching modern apps
        hModernFont = CreateFontA(
            -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI Variable"
        );
        
        hBoldFont = CreateFontA(
            -14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI Variable"
        );
        
        hHeaderFont = CreateFontA(
            -18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI Variable"
        );
        
        // Apply fonts to all controls
        applyModernFonts();
    }
    
    void applyModernFonts() {
        // Apply modern fonts to all controls
        if (hwnd) {
            EnumChildWindows(hwnd, [](HWND hChild, LPARAM lParam) -> BOOL {
                RealGameAnalyzerGUI* pThis = (RealGameAnalyzerGUI*)lParam;
                
                char className[256];
                GetClassNameA(hChild, className, sizeof(className));
                
                if (strcmp(className, "Button") == 0 || 
                    strcmp(className, "Static") == 0 || 
                    strcmp(className, "Edit") == 0 || 
                    strcmp(className, "ListBox") == 0) {
                    SendMessage(hChild, WM_SETFONT, (WPARAM)pThis->hModernFont, TRUE);
                }
                
                return TRUE;
            }, (LPARAM)this);
        }
    }
    
    void drawModernButton(LPDRAWITEMSTRUCT lpDrawItem) {
        HDC hdc = lpDrawItem->hDC;
        RECT rect = lpDrawItem->rcItem;
        
        // Determine button state
        bool isPressed = (lpDrawItem->itemState & ODS_SELECTED);
        bool isFocused = (lpDrawItem->itemState & ODS_FOCUS);
        bool isDisabled = (lpDrawItem->itemState & ODS_DISABLED);
        
        // Choose colors based on theme and state
        COLORREF bgColor, textColor, borderColor, shadowColor;
        
        if (modernThemeEnabled) {
            if (isDisabled) {
                bgColor = RGB(35, 35, 35);
                textColor = RGB(100, 100, 100);
                borderColor = RGB(50, 50, 50);
                shadowColor = RGB(20, 20, 20);
            } else if (isPressed) {
                bgColor = ModernTheme::DARK_BUTTON_PRESSED;
                textColor = ModernTheme::DARK_TEXT_PRIMARY;
                borderColor = RGB(70, 70, 70);
                shadowColor = RGB(15, 15, 15);
            } else {
                bgColor = ModernTheme::DARK_BUTTON_NORMAL;
                textColor = ModernTheme::DARK_TEXT_PRIMARY;
                borderColor = RGB(70, 70, 70);
                shadowColor = RGB(20, 20, 20);
            }
        } else {
            if (isDisabled) {
                bgColor = RGB(245, 245, 245);
                textColor = RGB(150, 150, 150);
                borderColor = RGB(220, 220, 220);
                shadowColor = RGB(200, 200, 200);
            } else if (isPressed) {
                bgColor = ModernTheme::BUTTON_PRESSED;
                textColor = ModernTheme::TEXT_PRIMARY;
                borderColor = RGB(180, 180, 180);
                shadowColor = RGB(200, 200, 200);
            } else {
                bgColor = ModernTheme::BUTTON_NORMAL;
                textColor = ModernTheme::TEXT_PRIMARY;
                borderColor = RGB(200, 200, 200);
                shadowColor = RGB(220, 220, 220);
            }
        }
        
        // Create brushes
        HBRUSH bgBrush = CreateSolidBrush(bgColor);
        HBRUSH borderBrush = CreateSolidBrush(borderColor);
        HBRUSH shadowBrush = CreateSolidBrush(shadowColor);
        
        // Draw subtle shadow for depth
        RECT shadowRect = rect;
        shadowRect.left += 1;
        shadowRect.top += 1;
        shadowRect.right += 1;
        shadowRect.bottom += 1;
        FillRect(hdc, &shadowRect, shadowBrush);
        
        // Fill main background
        FillRect(hdc, &rect, bgBrush);
        
        // Draw clean border
        FrameRect(hdc, &rect, borderBrush);
        
        // Draw text with modern font
        char buttonText[256];
        GetWindowTextA(lpDrawItem->hwndItem, buttonText, sizeof(buttonText));
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);
        
        // Apply modern font
        HFONT oldFont = (HFONT)SelectObject(hdc, hModernFont);
        
        // Center text with better padding
        RECT textRect = rect;
        textRect.left += 12;
        textRect.right -= 12;
        textRect.top += 4;
        textRect.bottom -= 4;
        
        DrawTextA(hdc, buttonText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // Restore font
        SelectObject(hdc, oldFont);
        
        // Clean up
        DeleteObject(bgBrush);
        DeleteObject(borderBrush);
        DeleteObject(shadowBrush);
    }
    
    void updateThemeBrushes() {
        // Clean up old brushes
        if (hBackgroundBrush) DeleteObject(hBackgroundBrush);
        if (hPanelBrush) DeleteObject(hPanelBrush);
        if (hCardBrush) DeleteObject(hCardBrush);
        if (hButtonHoverBrush) DeleteObject(hButtonHoverBrush);
        if (hButtonPressedBrush) DeleteObject(hButtonPressedBrush);
        if (hBorderBrush) DeleteObject(hBorderBrush);
        
        // Create new brushes based on current theme
        if (modernThemeEnabled) {
            hBackgroundBrush = CreateSolidBrush(ModernTheme::DARK_BACKGROUND_PRIMARY);
            hPanelBrush = CreateSolidBrush(ModernTheme::DARK_PANEL_BACKGROUND);
            hCardBrush = CreateSolidBrush(ModernTheme::DARK_CARD_BACKGROUND);
            hButtonHoverBrush = CreateSolidBrush(ModernTheme::DARK_BUTTON_HOVER);
            hButtonPressedBrush = CreateSolidBrush(ModernTheme::DARK_BUTTON_PRESSED);
            hBorderBrush = CreateSolidBrush(ModernTheme::DARK_BORDER_PRIMARY);
        } else {
            hBackgroundBrush = CreateSolidBrush(ModernTheme::BACKGROUND_PRIMARY);
            hPanelBrush = CreateSolidBrush(ModernTheme::PANEL_BACKGROUND);
            hCardBrush = CreateSolidBrush(ModernTheme::CARD_BACKGROUND);
            hButtonHoverBrush = CreateSolidBrush(ModernTheme::BUTTON_HOVER);
            hButtonPressedBrush = CreateSolidBrush(ModernTheme::BUTTON_PRESSED);
            hBorderBrush = CreateSolidBrush(ModernTheme::BORDER_PRIMARY);
        }
    }
    
    void cleanupModernUI() {
        if (hBackgroundBrush) DeleteObject(hBackgroundBrush);
        if (hPanelBrush) DeleteObject(hPanelBrush);
        if (hCardBrush) DeleteObject(hCardBrush);
        if (hModernFont) DeleteObject(hModernFont);
        if (hBoldFont) DeleteObject(hBoldFont);
        if (hHeaderFont) DeleteObject(hHeaderFont);
        if (hButtonHoverBrush) DeleteObject(hButtonHoverBrush);
        if (hButtonPressedBrush) DeleteObject(hButtonPressedBrush);
        if (hBorderBrush) DeleteObject(hBorderBrush);
        if (hTooltipWindow) DestroyWindow(hTooltipWindow);
    }
    
    void initializeTooltips() {
        if (!hwnd) return;
        
        // Create tooltip window
        hTooltipWindow = CreateWindowEx(
            WS_EX_TOPMOST,
            TOOLTIPS_CLASS,
            nullptr,
            WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );
        
        if (hTooltipWindow) {
            // Set tooltip info for key buttons
            addTooltip(hRefreshButton, "Refresh the list of running processes");
            addTooltip(hScanButton, "Scan all readable memory regions in the selected process");
            addTooltip(hScanGameDataButton, "Scan for likely game data (health, score, etc.)");
            addTooltip(hStartButton, "Start real-time monitoring of selected addresses");
            addTooltip(hExportButton, "Export monitored data to CSV file");
            addTooltip(hCaptureScreenButton, "Capture current screen for vision analysis");
            addTooltip(hStartVisionAnalysisButton, "Analyze captured screen for text detection");
            addTooltip(hHybridAnalysisButton, "Combine memory and vision data for comprehensive analysis");
            addTooltip(hAnalyticsButton, "Start collecting performance analytics data");
            addTooltip(hDashboardButton, "Open professional analytics dashboard");
            addTooltip(hAboutButton, "View application information and version details");
            addTooltip(hSettingsButton, "View and modify application settings");
            addTooltip(hHelpButton, "Open help guide and documentation");
        }
    }
    
    void addTooltip(HWND hControl, const char* text) {
        if (!hTooltipWindow || !hControl) return;
        
        TOOLINFO ti = {};
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = hwnd;
        ti.uId = (UINT_PTR)hControl;
        ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        ti.lpszText = (LPSTR)text;
        
        SendMessage(hTooltipWindow, TTM_ADDTOOL, 0, (LPARAM)&ti);
    }
    
    // Enhanced status reporting methods
    void setStatus(const std::string& message) {
        SetWindowText(hStatusLabel, message.c_str());
    }
    
    void setStatus(const char* format, ...) {
        char buffer[500];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        SetWindowText(hStatusLabel, buffer);
    }
    
    // Enhanced error handling with user-friendly messages
    void showError(const std::string& title, const std::string& message) {
        PopupDialogs::showErrorDialog(this, title, message);
    }
    
    void showWarning(const std::string& title, const std::string& message) {
        PopupDialogs::showWarningDialog(this, title, message);
    }
    
    void showInfo(const std::string& title, const std::string& message) {
        PopupDialogs::showInfoDialog(this, title, message);
    }
    
    void showCustomMessageBox(const std::string& title, const std::string& message, UINT iconType) {
        // Create custom dialog window with dark mode support
        HWND dialog = CreateWindowEx(
            WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
            "STATIC", title.c_str(),
            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, 500, 300,
            hwnd, nullptr, GetModuleHandle(nullptr), nullptr
        );
        
        if (!dialog) return;
        
        // Enable dark title bar if dark mode is enabled
        if (modernThemeEnabled) {
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(dialog, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        }
        
        // Create message text control
        HWND messageControl = CreateWindow(
            "STATIC", message.c_str(),
            WS_VISIBLE | WS_CHILD | SS_LEFT | SS_NOPREFIX,
            20, 50, 460, 180,
            dialog, nullptr, GetModuleHandle(nullptr), nullptr
        );
        
        // Create OK button
        HWND okButton = CreateWindow(
            "BUTTON", "OK",
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            200, 250, 100, 30,
            dialog, (HMENU)1, GetModuleHandle(nullptr), nullptr
        );
        
        // Apply fonts
        SendMessage(messageControl, WM_SETFONT, (WPARAM)hModernFont, TRUE);
        SendMessage(okButton, WM_SETFONT, (WPARAM)hModernFont, TRUE);
        
        // Center dialog on screen
        RECT rect;
        GetWindowRect(dialog, &rect);
        int x = (GetSystemMetrics(SM_CXSCREEN) - (rect.right - rect.left)) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - (rect.bottom - rect.top)) / 2;
        SetWindowPos(dialog, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);
        
        // Set up custom window procedure for dialog
        SetWindowLongPtr(dialog, GWLP_USERDATA, (LONG_PTR)this);
        SetWindowLongPtr(dialog, GWLP_WNDPROC, (LONG_PTR)dialogWindowProc);
        
        // Show dialog
        ShowWindow(dialog, SW_SHOW);
        UpdateWindow(dialog);
        
        // Message loop for dialog
        MSG msg;
        BOOL bRet;
        while ((bRet = GetMessage(&msg, nullptr, 0, 0)) != 0) {
            if (bRet == -1) {
                break;
            } else {
                if (msg.hwnd == dialog || IsChild(dialog, msg.hwnd)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
                
                // Check if dialog should close
                if (msg.message == WM_COMMAND && LOWORD(msg.wParam) == 1) {
                    break; // OK button clicked
                }
                if (msg.message == WM_CLOSE && msg.hwnd == dialog) {
                    break; // Dialog closed
                }
            }
        }
        
        DestroyWindow(dialog);
    }
    
    static LRESULT CALLBACK dialogWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        RealGameAnalyzerGUI* pThis = (RealGameAnalyzerGUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        
        switch (uMsg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                RECT rect;
                GetClientRect(hwnd, &rect);
                
                // Fill with appropriate background
                if (pThis && pThis->modernThemeEnabled) {
                    FillRect(hdc, &rect, pThis->hBackgroundBrush);
                } else {
                    FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
                }
                
                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_CTLCOLORSTATIC: {
                HDC hdc = (HDC)wParam;
                if (pThis && pThis->modernThemeEnabled) {
                    SetBkColor(hdc, ModernTheme::DARK_BACKGROUND_PRIMARY);
                    SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                    return (LRESULT)pThis->hBackgroundBrush;
                } else {
                    SetBkColor(hdc, ModernTheme::BACKGROUND_PRIMARY);
                    SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                    return (LRESULT)pThis->hBackgroundBrush;
                }
            }
            case WM_CTLCOLORBTN: {
                HDC hdc = (HDC)wParam;
                if (pThis && pThis->modernThemeEnabled) {
                    SetBkColor(hdc, ModernTheme::DARK_BUTTON_NORMAL);
                    SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                    return (LRESULT)pThis->hCardBrush;
                } else {
                    SetBkColor(hdc, ModernTheme::BUTTON_NORMAL);
                    SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                    return (LRESULT)pThis->hCardBrush;
                }
            }
            case WM_DRAWITEM: {
                LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
                if (lpDrawItem->CtlType == ODT_BUTTON && pThis) {
                    pThis->drawModernButton(lpDrawItem);
                    return TRUE;
                }
                break;
            }
            case WM_COMMAND: {
                if (LOWORD(wParam) == 1) {
                    // OK button clicked
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return 0;
                }
                break;
            }
            case WM_CLOSE: {
                DestroyWindow(hwnd);
                return 0;
            }
        }
        
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    // Progress indicator methods
    void setProgress(int percentage) {
        SendMessage(hProgressBar, PBM_SETPOS, percentage, 0);
    }
    
    void resetProgress() {
        SendMessage(hProgressBar, PBM_SETPOS, 0, 0);
    }
    
    void showProgress(bool show) {
        ShowWindow(hProgressBar, show ? SW_SHOW : SW_HIDE);
    }
    
    bool createWindow() {
        // Initialize common controls for progress bar
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);
        
        WNDCLASSEX wc = {};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = "RealGameAnalyzerWindow";
        
        if (!RegisterClassEx(&wc)) {
            // Check if class is already registered (this is normal for subsequent runs)
            DWORD error = GetLastError();
            if (error != ERROR_CLASS_ALREADY_EXISTS) {
            return false;
            }
        }
        
        hwnd = CreateWindowEx(
            0,
            "RealGameAnalyzerWindow",
            "Game Analyzer Pro - Professional Gaming Analytics Platform",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            1200, 900,
            nullptr, nullptr,
            GetModuleHandle(nullptr),
            this
        );
        
        // Enable dark title bar for Windows 10/11
        if (hwnd) {
            BOOL darkMode = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
        }
        
        if (!hwnd) {
            return false;
        }
        
        createControls();
        
        // Initialize advanced components
        initializeAdvancedComponents();
        
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        
        return true;
    }
    
    void createControls() {
        // PROFESSIONAL UI DESIGN - VS Code/Steam Quality
        
        // Top Ribbon Panel - Ultra Clean Toolbar
        hRibbonPanel = CreateWindow("STATIC", "", WS_VISIBLE | WS_CHILD,
            0, 0, 1200, 45, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Ribbon Controls - Ultra Clean Spacing
        hDarkModeToggle = CreateWindow("BUTTON", "Dark Mode", 
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            15, 12, 90, 22, hwnd, (HMENU)3001, GetModuleHandle(nullptr), nullptr);
        SendMessage(hDarkModeToggle, BM_SETCHECK, modernThemeEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        
        hSystemProcessesToggle = CreateWindow("BUTTON", "System Processes", 
            WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            115, 12, 120, 22, hwnd, (HMENU)3002, GetModuleHandle(nullptr), nullptr);
        SendMessage(hSystemProcessesToggle, BM_SETCHECK, showSystemProcesses ? BST_CHECKED : BST_UNCHECKED, 0);
        
        hRefreshRibbonButton = CreateWindow("BUTTON", "Refresh", 
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            250, 10, 70, 28, hwnd, (HMENU)3003, GetModuleHandle(nullptr), nullptr);
        
        hQuickSettingsButton = CreateWindow("BUTTON", "Settings", 
            WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            330, 10, 70, 28, hwnd, (HMENU)3004, GetModuleHandle(nullptr), nullptr);
        
        // Ultra Clean Header Section
        CreateWindow("STATIC", "Game Analyzer Pro", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 60, 1200, 30, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("STATIC", "Professional Gaming Analytics Platform", WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 90, 1200, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Process Selection Panel - Ultra Clean Layout
        CreateWindow("STATIC", "Process Selection", WS_VISIBLE | WS_CHILD,
            20, 130, 200, 22, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hListBox = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_NOTIFY,
            20, 160, 400, 250, hwnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);
        
        // Process Control Buttons - Ultra Clean Spacing
        hRefreshButton = CreateWindow("BUTTON", "Refresh", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            440, 160, 80, 28, hwnd, (HMENU)4, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("BUTTON", "Select", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            440, 195, 80, 28, hwnd, (HMENU)9, GetModuleHandle(nullptr), nullptr);
        
        // Search Section - Ultra Clean Design
        CreateWindow("STATIC", "Search:", WS_VISIBLE | WS_CHILD,
            440, 230, 50, 18, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hSearchEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            440, 250, 120, 22, hwnd, (HMENU)6, GetModuleHandle(nullptr), nullptr);
        
        // Action Buttons - Ultra Clean Spacing
        hStartButton = CreateWindow("BUTTON", "Start Monitoring", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            440, 280, 100, 28, hwnd, (HMENU)2, GetModuleHandle(nullptr), nullptr);
        
        hExportButton = CreateWindow("BUTTON", "Export", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            550, 280, 70, 28, hwnd, (HMENU)3, GetModuleHandle(nullptr), nullptr);
        
        // Status Label - Ultra Clean
        hStatusLabel = CreateWindow("STATIC", "Ready - Select a process to begin", WS_VISIBLE | WS_CHILD,
            20, 420, 600, 18, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Populate process list
        refreshProcessList();
        
        // Memory Analysis Panel - Professional Layout
        CreateWindow("STATIC", "Memory Analysis", WS_VISIBLE | WS_CHILD,
            750, 150, 200, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hScanButton = CreateWindow("BUTTON", "Scan Memory", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            750, 180, 120, 32, hwnd, (HMENU)7, GetModuleHandle(nullptr), nullptr);
        
        hScanGameDataButton = CreateWindow("BUTTON", "Scan Game Data", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            880, 180, 120, 32, hwnd, (HMENU)14, GetModuleHandle(nullptr), nullptr);
        
        hProgressBar = CreateWindow(PROGRESS_CLASS, "", WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
            750, 220, 300, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Discovered Addresses Section - Professional Layout
        CreateWindow("STATIC", "Discovered Addresses", WS_VISIBLE | WS_CHILD,
            750, 250, 200, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        CreateWindow("STATIC", "Search:", WS_VISIBLE | WS_CHILD,
            750, 280, 60, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hAddressSearchEdit = CreateWindow("EDIT", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            750, 305, 200, 24, hwnd, (HMENU)15, GetModuleHandle(nullptr), nullptr);
        
        hAddressListBox = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | LBS_EXTENDEDSEL,
            750, 340, 400, 200, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Action Buttons Panel - Professional Grid Layout
        hAddSelectedButton = CreateWindow("BUTTON", "Add to Monitor", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            750, 550, 120, 32, hwnd, (HMENU)8, GetModuleHandle(nullptr), nullptr);
        
        hCompareButton = CreateWindow("BUTTON", "Compare", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            880, 550, 100, 32, hwnd, (HMENU)10, GetModuleHandle(nullptr), nullptr);
        
        hValueChangeButton = CreateWindow("BUTTON", "Add Changed", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            990, 550, 100, 32, hwnd, (HMENU)11, GetModuleHandle(nullptr), nullptr);
        
        hLoadGameProfileButton = CreateWindow("BUTTON", "Load Profile", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            750, 590, 100, 32, hwnd, (HMENU)12, GetModuleHandle(nullptr), nullptr);
        
        hSaveGameProfileButton = CreateWindow("BUTTON", "Save Profile", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            860, 590, 100, 32, hwnd, (HMENU)13, GetModuleHandle(nullptr), nullptr);
        
        hListProfilesButton = CreateWindow("BUTTON", "List Profiles", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            970, 590, 100, 32, hwnd, (HMENU)15, GetModuleHandle(nullptr), nullptr);
        
        // Vision Analysis Panel - Professional Card
        CreateWindow("STATIC", "Vision Analysis", WS_VISIBLE | WS_CHILD,
            30, 520, 200, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hCaptureScreenButton = CreateWindow("BUTTON", "Capture Screen", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            30, 550, 120, 32, hwnd, (HMENU)16, GetModuleHandle(nullptr), nullptr);
        
        hStartVisionAnalysisButton = CreateWindow("BUTTON", "Analyze Vision", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            160, 550, 120, 32, hwnd, (HMENU)17, GetModuleHandle(nullptr), nullptr);
        
        hVisionStatusLabel = CreateWindow("STATIC", "Vision: Ready", WS_VISIBLE | WS_CHILD | SS_LEFT,
            30, 590, 300, 20, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Hybrid Analysis Panel - Professional Card
        CreateWindow("STATIC", "Hybrid Analysis", WS_VISIBLE | WS_CHILD,
            300, 520, 200, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hHybridAnalysisButton = CreateWindow("BUTTON", "Run Hybrid Analysis", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            300, 550, 140, 32, hwnd, (HMENU)18, GetModuleHandle(nullptr), nullptr);
        
        hCompareMemoryVisionButton = CreateWindow("BUTTON", "Compare Data", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            450, 550, 120, 32, hwnd, (HMENU)19, GetModuleHandle(nullptr), nullptr);
        
        // Analytics Panel - Professional Card
        CreateWindow("STATIC", "Analytics Engine", WS_VISIBLE | WS_CHILD,
            30, 630, 200, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hAnalyticsButton = CreateWindow("BUTTON", "Start Analytics", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            30, 660, 120, 32, hwnd, (HMENU)20, GetModuleHandle(nullptr), nullptr);
        
        hShowMetricsButton = CreateWindow("BUTTON", "Show Metrics", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            160, 660, 120, 32, hwnd, (HMENU)21, GetModuleHandle(nullptr), nullptr);
        
        hExportAnalyticsButton = CreateWindow("BUTTON", "Export Analytics", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            290, 660, 120, 32, hwnd, (HMENU)22, GetModuleHandle(nullptr), nullptr);
        
        // Professional Dashboard Panel - Professional Card
        CreateWindow("STATIC", "Professional Dashboard", WS_VISIBLE | WS_CHILD,
            30, 700, 250, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hDashboardButton = CreateWindow("BUTTON", "Open Dashboard", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            30, 730, 120, 32, hwnd, (HMENU)23, GetModuleHandle(nullptr), nullptr);
        
        hRealTimeChartsButton = CreateWindow("BUTTON", "Real-time Charts", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            160, 730, 120, 32, hwnd, (HMENU)24, GetModuleHandle(nullptr), nullptr);
        
        hPerformanceMonitorButton = CreateWindow("BUTTON", "Performance Monitor", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            290, 730, 140, 32, hwnd, (HMENU)25, GetModuleHandle(nullptr), nullptr);
        
        // Application Control Panel - Professional Footer
        CreateWindow("STATIC", "Application", WS_VISIBLE | WS_CHILD,
            30, 770, 150, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hAboutButton = CreateWindow("BUTTON", "About", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            30, 800, 80, 32, hwnd, (HMENU)26, GetModuleHandle(nullptr), nullptr);
        
        hSettingsButton = CreateWindow("BUTTON", "Settings", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            120, 800, 80, 32, hwnd, (HMENU)27, GetModuleHandle(nullptr), nullptr);
        
        hHelpButton = CreateWindow("BUTTON", "Help & Guide", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
            210, 800, 100, 32, hwnd, (HMENU)28, GetModuleHandle(nullptr), nullptr);
        
        // Monitored Addresses Panel - Professional Card
        CreateWindow("STATIC", "Monitored Addresses", WS_VISIBLE | WS_CHILD,
            750, 630, 200, 25, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        hMemList = CreateWindow("LISTBOX", "", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL,
            750, 660, 400, 150, hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
        
        // Initialize tooltips after all controls are created
        initializeTooltips();
        
        // No sample addresses - start with empty list
    }
    
    void initializeAdvancedComponents() {
        // Initialize thread manager
        threadManager.initialize(8, 32);
        
        // Initialize advanced screen capture with GPU acceleration
        optimizedScreenCapture.initialize(true, true);
        optimizedScreenCapture.setCaptureMode(OptimizedScreenCapture::CaptureMode::GAME_WINDOW);
        optimizedScreenCapture.setMaxFPS(60);
        
        // Initialize advanced OCR with Windows.Media.Ocr (fallback to Tesseract)
        if (!advancedOCR.initialize(AdvancedOCR::OCRBackend::TESSERACT)) {
            // Fallback to basic initialization
        }
        advancedOCR.enableCaching(true);
        
        // Initialize frame cache
        frameCache.~FrameCache();
        new(&frameCache) FrameCache(100, 5000); // 100 frames, 5 second timeout
        
        // Initialize intelligent region processor
        regionProcessor = IntelligentRegionProcessor();
        
        // Add default processing regions for common game UI elements
        IntelligentRegionProcessor::ProcessingRegion hudRegion;
        hudRegion.name = "HUD";
        hudRegion.rect = cv::Rect(0, 0, 1920, 200); // Top HUD area
        hudRegion.fps = 30;
        hudRegion.requiredState = IntelligentRegionProcessor::GameState::GAMEPLAY;
        regionProcessor.addRegion(hudRegion);
        
        IntelligentRegionProcessor::ProcessingRegion healthRegion;
        healthRegion.name = "Health";
        healthRegion.rect = cv::Rect(50, 50, 200, 100); // Health bar area
        healthRegion.fps = 10;
        healthRegion.requiredState = IntelligentRegionProcessor::GameState::GAMEPLAY;
        regionProcessor.addRegion(healthRegion);
        
        IntelligentRegionProcessor::ProcessingRegion scoreRegion;
        scoreRegion.name = "Score";
        scoreRegion.rect = cv::Rect(1600, 50, 300, 100); // Score area
        scoreRegion.fps = 10;
        scoreRegion.requiredState = IntelligentRegionProcessor::GameState::GAMEPLAY;
        regionProcessor.addRegion(scoreRegion);
        
        // Initialize game event detector
        gameEventDetector.initialize();
        
        // Initialize game fingerprinting
        gameFingerprinting.initialize();
        
        // Initialize Bloomberg analytics engine
        bloombergAnalytics.initialize(20, 0.1); // 20 period lookback, 0.1 smoothing
        
        // Initialize dialog manager
        dialogManager.~SmartDialogManager();
        new(&dialogManager) SmartDialogManager();
        
        setStatus("Advanced components initialized - Ready for professional gaming analytics");
    }
    
    bool isUserApplication(const std::string& processName) {
        std::string lowerName = processName;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        
        // Comprehensive list of system/background processes to exclude
        std::vector<std::string> systemProcesses = {
            "system", "idle", "smss.exe", "csrss.exe", "wininit.exe", "winlogon.exe",
            "services.exe", "lsass.exe", "svchost.exe", "dwm.exe", "explorer.exe",
            "taskhost.exe", "taskhostw.exe", "runtimebroker.exe", "searchapp.exe",
            "conhost.exe", "audiodg.exe", "spoolsv.exe", "dllhost.exe", "wmiprvse.exe",
            "sihost.exe", "ctfmon.exe", "fontdrvhost.exe", "registry", "memory compression",
            "secure system", "nvcontainer.exe", "nvidia web helper.exe", "razer synapse",
            "asusoptimizationstartup", "asus_framework.exe", "textinputhost.exe",
            "securityhealthsystray.exe", "startmenuexperiencehost.exe", "steamwebhelper.exe",
            "steamservice.exe", "steamclient", "steamerrorreporter", "steamtours",
            "steamcompositor", "steamoverlayui", "steamwebhelper", "steamworks",
            "nvidia", "amd", "intel", "microsoft", "windows", "system32", "syswow64",
            "program files", "programdata", "appdata", "localappdata", "temp",
            "antimalware", "defender", "security", "update", "installer", "setup",
            "service", "daemon", "helper", "agent", "monitor", "tray", "notification",
            "background", "host", "broker", "manager", "controller", "driver",
            "framework", "runtime", "core", "engine", "platform", "sdk", "api"
        };
        
        // Check if it's a system process
        for (const auto& sysProc : systemProcesses) {
            if (lowerName.find(sysProc) != std::string::npos) {
                return false;
            }
        }
        
        // Only include processes that look like actual user applications
        // Must have .exe extension and not be in system directories
        if (lowerName.find(".exe") != std::string::npos) {
            // Exclude if it contains system-related keywords
            if (lowerName.find("system") != std::string::npos ||
                lowerName.find("windows") != std::string::npos ||
                lowerName.find("microsoft") != std::string::npos ||
                lowerName.find("service") != std::string::npos ||
                lowerName.find("helper") != std::string::npos ||
                lowerName.find("host") != std::string::npos ||
                lowerName.find("broker") != std::string::npos ||
                lowerName.find("container") != std::string::npos ||
                lowerName.find("framework") != std::string::npos ||
                lowerName.find("optimization") != std::string::npos ||
                lowerName.find("synapse") != std::string::npos ||
                lowerName.find("webhelper") != std::string::npos) {
                return false;
            }
            return true;
        }
        
        return false;
    }
    
    void refreshProcesses() {
        processes.clear();
        
        // Get all processes
        DWORD processIds[1024];
        DWORD cbNeeded;
        
        if (EnumProcesses(processIds, sizeof(processIds), &cbNeeded)) {
            DWORD processCount = cbNeeded / sizeof(DWORD);
            
            for (DWORD i = 0; i < processCount; i++) {
                if (processIds[i] != 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[i]);
                    if (hProcess) {
                        char processName[MAX_PATH] = {0};
                        if (GetModuleBaseName(hProcess, nullptr, processName, sizeof(processName))) {
                            std::string name = processName;
                            if (showSystemProcesses || isUserApplication(name)) {
                                processes.push_back(ProcessInfo(processIds[i], name));
                            }
                        }
                        CloseHandle(hProcess);
                    }
                }
            }
        }
    }
    
    void refreshProcessList() {
        SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
        
        // Get search text
        char searchText[256];
        GetWindowText(hSearchEdit, searchText, sizeof(searchText));
        std::string search = searchText;
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        
        // Store filtered processes for selection matching
        filteredProcesses.clear();
        
        for (auto& process : processes) {
            std::string display = process.name + " (PID: " + std::to_string(process.pid) + ")";
            
            // If search is empty or process name contains search text, add it
            if (search.empty()) {
                SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                filteredProcesses.push_back(&process);
            } else {
                std::string processName = process.name;
                std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
                if (processName.find(search) != std::string::npos) {
                    SendMessage(hListBox, LB_ADDSTRING, 0, (LPARAM)display.c_str());
                    filteredProcesses.push_back(&process);
                }
            }
        }
    }
    
    void addMemoryAddress(const std::string& address, const std::string& name) {
        uintptr_t addr = std::stoull(address, nullptr, 16);
        memoryAddresses.push_back({name, addr});
        
        std::string display = address + " - " + name;
        SendMessage(hMemList, LB_ADDSTRING, 0, (LPARAM)display.c_str());
    }
    
    void run() {
        MSG msg = {};
        while (GetMessage(&msg, nullptr, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    void onButtonClick(int buttonId) {
        switch (buttonId) {
            case 2: // Start/Stop Monitoring
                toggleMonitoring();
                break;
            case 3: // Export Data
                exportData();
                break;
            case 4: // Refresh Processes
                refreshProcesses();
                refreshProcessList();
                setStatus("Process list refreshed");
                break;
            case 5: // Toggle System Processes
                showSystemProcesses = !showSystemProcesses;
                refreshProcesses();
                refreshProcessList();
                setStatus("System processes %s", showSystemProcesses ? "shown" : "hidden");
                break;
            case 6: // Search text changed
                refreshProcessList();
                break;
            case 7: // Scan Process Memory
                scanProcessMemory();
                break;
            case 8: // Add Selected Addresses
                addSelectedAddresses();
                break;
            case 9: // Select Highlighted Process
                onProcessSelection();
                break;
            case 10: // Compare Values
                compareValues();
                break;
            case 11: // Add Changed to Monitor
                showChangedValues();
                break;
            case 12: // Load Game Profile
                loadGameProfile();
                break;
            case 13: // Save Game Profile
                saveGameProfile();
                break;
            case 14: // Scan Game Data
                scanGameData();
                break;
            case 15: // List Available Profiles
                listAvailableProfiles();
                break;
            case 16: // Capture Screen
                captureScreen();
                break;
            case 17: // Start Vision Analysis
                startVisionAnalysis();
                break;
            case 18: // Run Hybrid Analysis
                runHybridAnalysis();
                break;
            case 19: // Compare Memory vs Vision
                compareMemoryVision();
                break;
            case 20: // Start Analytics
                startAnalytics();
                break;
            case 21: // Show Metrics
                showMetrics();
                break;
            case 22: // Export Analytics
                exportAnalytics();
                break;
            case 23: // Open Dashboard
                openDashboard();
                break;
            case 24: // Real-time Charts
                showRealTimeCharts();
                break;
            case 25: // Performance Monitor
                showPerformanceMonitor();
                break;
            case 26: // About
                showAboutDialog();
                break;
            case 27: // Settings
                showSettingsDialog();
                break;
            case 28: // Help & Guide
                showHelpDialog();
                break;
            case 3001: // Dark Mode Toggle
                toggleDarkMode();
                break;
            case 3002: // System Processes Toggle
                toggleSystemProcesses();
                break;
            case 3003: // Refresh Ribbon Button
                refreshProcesses();
                refreshProcessList();
                break;
            case 3004: // Quick Settings
                showSettingsDialog();
                break;
        }
    }
    
    void onProcessSelection() {
        int sel = SendMessage(hListBox, LB_GETCURSEL, 0, 0);
        char debugMsg[200];
        sprintf(debugMsg, "Selection event: sel=%d, filteredProcesses.size()=%zu", sel, filteredProcesses.size());
        SetWindowText(hStatusLabel, debugMsg);
        
        if (sel != LB_ERR && sel < (int)filteredProcesses.size()) {
            selectedProcess = filteredProcesses[sel];
            char status[200];
            sprintf(status, "Selected: %s (PID: %d)", selectedProcess->name.c_str(), selectedProcess->pid);
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "No valid process selected");
        }
    }
    
    
    void toggleMonitoring() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (monitoring) {
            monitoring = false;
            SetWindowText(hStartButton, "Start Monitoring");
            SetWindowText(hStatusLabel, "Monitoring stopped.");
        } else {
            monitoring = true;
            SetWindowText(hStartButton, "Stop Monitoring");
            SetWindowText(hStatusLabel, "Monitoring started...");
            
            // Start monitoring thread
            std::thread([this]() {
                int sample = 0;
                while (monitoring && selectedProcess) {
                    char status[200];
                    sprintf(status, "Monitoring %s... Sample %d", selectedProcess->name.c_str(), sample);
                    SetWindowText(hStatusLabel, status);
                    
                    // Try to read memory from selected process
                    for (const auto& memAddr : memoryAddresses) {
                        int32_t value;
                        if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                            // Successfully read memory
                            char memStatus[300];
                            sprintf(memStatus, "%s: %s = %d", status, memAddr.first.c_str(), value);
                            SetWindowText(hStatusLabel, memStatus);
                        }
                    }
                    
                    sample++;
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
            }).detach();
        }
    }
    
    void exportData() {
        SetWindowText(hStatusLabel, "Exporting data to game_analysis.csv...");
        
        FILE* file = fopen("game_analysis.csv", "w");
        if (file) {
            fprintf(file, "Timestamp,Process,Address,Name,Value\n");
            
            time_t now = time(0);
            char timestamp[100];
            strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
            
            if (selectedProcess) {
                for (const auto& memAddr : memoryAddresses) {
                    int32_t value = 0;
                    if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                        fprintf(file, "%s,%s,0x%llX,%s,%d\n", 
                               timestamp, selectedProcess->name.c_str(), 
                               (unsigned long long)memAddr.second, memAddr.first.c_str(), value);
                    }
                }
            }
            
            fclose(file);
            
            char status[200];
            char cwd[256];
            getcwd(cwd, sizeof(cwd));
            sprintf(status, "Data exported to: %s/game_analysis.csv", cwd);
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "Error: Could not create export file!");
        }
    }
    
    void scanProcessMemory() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before scanning memory.");
            return;
        }
        
        if (scanning) {
            showWarning("Scan In Progress", "Memory scan is already in progress. Please wait for it to complete.");
            return;
        }
        
        scanning = true;
        SetWindowText(hScanButton, "Scanning...");
        EnableWindow(hScanButton, FALSE);
        showProgress(true);
        resetProgress();
        setStatus("Initializing memory scan...");
        
        // Clear previous results
        SendMessage(hAddressListBox, LB_RESETCONTENT, 0, 0);
        discoveredAddresses.clear();
        addressChecked.clear();
        
        // Start scanning in a separate thread
        std::thread([this]() {
            try {
                // Step 1: Get memory regions
                setStatus("Discovering memory regions...");
                setProgress(10);
                memoryRegions = MemoryScanner::scanProcessMemory(selectedProcess->pid);
                
                if (memoryRegions.empty()) {
                    showError("No Memory Regions", "No readable memory regions found in the selected process. The process may be protected or not have accessible memory.");
                    goto cleanup;
                }
                
                setStatus("Found %zu memory regions, scanning for readable addresses...", memoryRegions.size());
                setProgress(30);
                
                // Step 2: Find readable addresses
                discoveredAddresses = MemoryScanner::findReadableAddresses(selectedProcess->pid, memoryRegions, 200);
                setProgress(70);
                
                // Initialize checkbox state
                addressChecked.resize(discoveredAddresses.size(), false);
                
                // Step 3: Populate the address list using filter method
                filterAddressList();
                setProgress(90);
                
                setStatus("Scan complete! Found %zu readable addresses", discoveredAddresses.size());
                setProgress(100);
                
                if (discoveredAddresses.empty()) {
                    showWarning("No Addresses Found", "No readable memory addresses were found. Try running as Administrator or selecting a different process.");
                } else {
                    showInfo("Scan Complete", "Memory scan completed successfully. You can now use 'Compare Values' to detect changes.");
                }
                
            } catch (const std::exception& e) {
                showError("Scan Error", "An error occurred during memory scanning. Please try again or run as Administrator.");
            }
            
        cleanup:
            scanning = false;
            SetWindowText(hScanButton, "Scan Process Memory");
            EnableWindow(hScanButton, TRUE);
            showProgress(false);
        }).detach();
    }
    
    void filterAddressList() {
        // Get search text
        char searchText[256];
        GetWindowText(hAddressSearchEdit, searchText, sizeof(searchText));
        std::string search = searchText;
        
        // Clear and repopulate the list with filtered results
        SendMessage(hAddressListBox, LB_RESETCONTENT, 0, 0);
        
        for (size_t i = 0; i < discoveredAddresses.size(); ++i) {
            uintptr_t addr = discoveredAddresses[i];
            std::string addrStr = MemoryScanner::addressToString(addr);
            
            // Try to read the value at this address
            int32_t value = 0;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &value, sizeof(value))) {
                std::string interpretation = MemoryScanner::interpretValue(value);
                std::string display = addrStr + " (Value: " + std::to_string(value) + ")" + interpretation;
                
                // Add checkbox to display
                std::string checkboxDisplay = (addressChecked.size() > i && addressChecked[i]) ? "[✓] " : "[ ] ";
                checkboxDisplay += display;
                
                // Filter based on search text (case-insensitive)
                if (search.empty()) {
                    SendMessage(hAddressListBox, LB_ADDSTRING, 0, (LPARAM)checkboxDisplay.c_str());
                } else {
                    // Convert both strings to lowercase for case-insensitive search
                    // Search in the full checkboxDisplay text (includes checkbox and *** markers)
                    std::string checkboxDisplayLower = checkboxDisplay;
                    std::string searchLower = search;
                    std::transform(checkboxDisplayLower.begin(), checkboxDisplayLower.end(), checkboxDisplayLower.begin(), ::tolower);
                    std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
                    
                    if (checkboxDisplayLower.find(searchLower) != std::string::npos) {
                        SendMessage(hAddressListBox, LB_ADDSTRING, 0, (LPARAM)checkboxDisplay.c_str());
                    }
                }
            }
        }
    }
    
    void toggleAddressCheckbox() {
        int sel = SendMessage(hAddressListBox, LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR && sel < (int)discoveredAddresses.size()) {
            // Toggle the checkbox state
            if (sel < (int)addressChecked.size()) {
                addressChecked[sel] = !addressChecked[sel];
                // Refresh the display
                filterAddressList();
            }
        }
    }
    
    void addSelectedAddresses() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        int addedCount = 0;
        for (size_t i = 0; i < discoveredAddresses.size(); ++i) {
            if (i < addressChecked.size() && addressChecked[i]) {
                uintptr_t addr = discoveredAddresses[i];
                std::string addrStr = MemoryScanner::addressToString(addr);
                std::string name = "Discovered_" + std::to_string(addedCount + 1);
                
                // Check if address already exists
                bool exists = false;
                for (const auto& memAddr : memoryAddresses) {
                    if (memAddr.second == addr) {
                        exists = true;
                        break;
                    }
                }
                
                if (!exists) {
                    addMemoryAddress(addrStr, name);
                    addedCount++;
                }
            }
        }
        
        if (addedCount == 0) {
            SetWindowText(hStatusLabel, "No checked addresses to add. Click on [ ] to check addresses.");
        } else {
            char status[100];
            sprintf(status, "Added %d new addresses to monitoring list", addedCount);
            SetWindowText(hStatusLabel, status);
        }
    }
    
    void compareValues() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before comparing values.");
            return;
        }
        
        if (discoveredAddresses.empty()) {
            showWarning("No Addresses Found", "Please scan memory first using 'Scan Process Memory' or 'Scan Game Data'.");
            return;
        }
        
        setStatus("Reading current values for comparison...");
        showProgress(true);
        resetProgress();
        
        // Read current values and store them for comparison
        int successCount = 0;
        for (size_t i = 0; i < discoveredAddresses.size(); ++i) {
            uintptr_t addr = discoveredAddresses[i];
            int32_t value = 0;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &value, sizeof(value))) {
                previousValues[addr] = value;
                successCount++;
            }
            
            // Update progress
            int progress = (int)((i + 1) * 100 / discoveredAddresses.size());
            setProgress(progress);
        }
        
        showProgress(false);
        
        if (successCount == 0) {
            showError("Read Error", "Could not read any memory values. The process may be protected or no longer accessible. Try running as Administrator.");
        } else if (successCount < discoveredAddresses.size()) {
            char warningMsg[200];
            sprintf(warningMsg, "Only %d of %zu addresses could be read. Some values may be protected.", successCount, (int)discoveredAddresses.size());
            showWarning("Partial Read", warningMsg);
        } else {
            char infoMsg[200];
            sprintf(infoMsg, "Successfully stored %d values for comparison. Now interact with the game and click 'Show Changed Values' to see what changed.", successCount);
            showInfo("Baseline Captured", infoMsg);
        }
        
        setStatus("Baseline captured: %d values stored for comparison", successCount);
    }
    
    void showChangedValues() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (previousValues.empty()) {
            SetWindowText(hStatusLabel, "Please click 'Compare Values' first");
            return;
        }
        
        SetWindowText(hStatusLabel, "Checking for changed values and adding to monitoring...");
        
        int changedCount = 0;
        for (uintptr_t addr : discoveredAddresses) {
            int32_t currentValue = 0;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &currentValue, sizeof(currentValue))) {
                auto it = previousValues.find(addr);
                if (it != previousValues.end()) {
                    int32_t previousValue = it->second;
                    
                    if (currentValue != previousValue) {
                        // Address value changed - add it to monitoring
                        std::string addrStr = MemoryScanner::addressToString(addr);
                        std::string name = "Changed_" + std::to_string(changedCount + 1);
                        
                        // Check if address already exists in monitoring
                        bool exists = false;
                        for (const auto& memAddr : memoryAddresses) {
                            if (memAddr.second == addr) {
                                exists = true;
                                break;
                            }
                        }
                        
                        if (!exists) {
                            addMemoryAddress(addrStr, name);
                            changedCount++;
                        }
                    }
                }
            }
        }
        
        char status[200];
        if (changedCount > 0) {
            sprintf(status, "Found %d changed values! Added them to monitoring list.", changedCount);
        } else {
            sprintf(status, "No changed values found. Try doing something different in-game.");
        }
        SetWindowText(hStatusLabel, status);
    }
    
    void saveGameProfile() {
        if (!selectedProcess) {
            SetWindowText(hStatusLabel, "Please select a process first");
            return;
        }
        
        if (memoryAddresses.empty()) {
            SetWindowText(hStatusLabel, "No addresses to save");
            return;
        }
        
        // Create filename based on process name
        std::string filename = selectedProcess->name + "_profile.txt";
        
        FILE* file = fopen(filename.c_str(), "w");
        if (file) {
            fprintf(file, "# Game Profile for %s\n", selectedProcess->name.c_str());
            fprintf(file, "# Generated by Game Analyzer\n\n");
            
            for (const auto& memAddr : memoryAddresses) {
                fprintf(file, "0x%llX %s\n", (unsigned long long)memAddr.second, memAddr.first.c_str());
            }
            
            fclose(file);
            
            char status[200];
            sprintf(status, "Saved %zu addresses to %s", memoryAddresses.size(), filename.c_str());
            SetWindowText(hStatusLabel, status);
        } else {
            SetWindowText(hStatusLabel, "Error: Could not save profile file!");
        }
    }
    
    void loadGameProfile() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before loading a game profile.");
            return;
        }
        
        setStatus("Loading game profile for %s...", selectedProcess->name.c_str());
        
        // Try to load from game_profiles directory first
        std::string gameProfilesPath = "game_profiles/" + selectedProcess->name + ".txt";
        FILE* file = fopen(gameProfilesPath.c_str(), "r");
        
        // If not found in game_profiles, try local profile
        if (!file) {
            std::string localProfile = selectedProcess->name + "_profile.txt";
            file = fopen(localProfile.c_str(), "r");
        }
        
        if (file) {
            char line[256];
            int loadedCount = 0;
            std::string profileSource = (file == fopen(gameProfilesPath.c_str(), "r")) ? "game_profiles" : "local";
            
            while (fgets(line, sizeof(line), file)) {
                // Skip comments and empty lines
                if (line[0] == '#' || line[0] == '\n') continue;
                
                // Parse address and name (support both formats)
                uintptr_t address;
                char name[100];
                int parsed = 0;
                
                // Try format: Name=0xAddress
                if (sscanf(line, "%99[^=]=0x%llX", name, (unsigned long long*)&address) == 2) {
                    parsed = 2;
                }
                // Try format: 0xAddress Name
                else if (sscanf(line, "0x%llX %99s", (unsigned long long*)&address, name) == 2) {
                    parsed = 2;
                }
                
                if (parsed == 2) {
                    // Check if address already exists
                    bool exists = false;
                    for (const auto& memAddr : memoryAddresses) {
                        if (memAddr.second == address) {
                            exists = true;
                            break;
                        }
                    }
                    
                    if (!exists) {
                        std::string addrStr = MemoryScanner::addressToString(address);
                        addMemoryAddress(addrStr, name);
                        loadedCount++;
                    }
                }
            }
            
            fclose(file);
            
            if (loadedCount > 0) {
                char infoMsg[200];
                sprintf(infoMsg, "Successfully loaded %d addresses from %s profile. You can now start monitoring!", loadedCount, selectedProcess->name.c_str());
                showInfo("Profile Loaded", infoMsg);
                setStatus("Loaded %d addresses from %s profile", loadedCount, profileSource.c_str());
            } else {
                showWarning("No New Addresses", "Profile loaded but no new addresses were added. All addresses may already be in your monitoring list.");
            }
        } else {
            // Check if game_profiles directory exists
            if (fopen("game_profiles", "r")) {
                char warningMsg[200];
                sprintf(warningMsg, "No profile found for '%s'. Available profiles are in the 'game_profiles' folder. Save a profile first or use memory scanning.", selectedProcess->name.c_str());
                showWarning("No Profile Found", warningMsg);
            } else {
                char warningMsg[200];
                sprintf(warningMsg, "No profile found for '%s'. Use 'Save Game Profile' after scanning memory to create a profile.", selectedProcess->name.c_str());
                showWarning("No Profile Found", warningMsg);
            }
        }
    }
    
    void listAvailableProfiles() {
        setStatus("Scanning for available game profiles...");
        
        std::string profilesList = "Available Game Profiles:\n\n";
        int profileCount = 0;
        
        // Check game_profiles directory
        FILE* dir = fopen("game_profiles", "r");
        if (dir) {
            fclose(dir);
            profilesList += "Built-in Profiles (game_profiles/):\n";
            
            // List known profiles
            std::vector<std::string> knownProfiles = {
                "Counter-Strike 2.txt",
                "Valorant.txt", 
                "Dwarf Fortress.txt",
                "Planescape Torment.txt"
            };
            
            for (const auto& profile : knownProfiles) {
                std::string path = "game_profiles/" + profile;
                FILE* file = fopen(path.c_str(), "r");
                if (file) {
                    fclose(file);
                    profilesList += "  - " + profile + "\n";
                    profileCount++;
                }
            }
            profilesList += "\n";
        }
        
        // Check for local profiles
        profilesList += "Local Profiles (saved by you):\n";
        bool foundLocal = false;
        
        // This is a simplified check - in a real implementation, you'd scan the directory
        for (const auto& process : processes) {
            std::string localProfile = process.name + "_profile.txt";
            FILE* file = fopen(localProfile.c_str(), "r");
            if (file) {
                fclose(file);
                profilesList += "  - " + localProfile + "\n";
                profileCount++;
                foundLocal = true;
            }
        }
        
        if (!foundLocal) {
            profilesList += "  No local profiles found\n";
        }
        
        profilesList += "\nTo use a profile:\n";
        profilesList += "1. Select the matching process from the list\n";
        profilesList += "2. Click 'Load Game Profile'\n";
        profilesList += "3. Start monitoring!\n";
        
        if (profileCount > 0) {
            showInfo("Available Profiles", profilesList.c_str());
            setStatus("Found %d available game profiles", profileCount);
        } else {
            showWarning("No Profiles Found", "No game profiles were found. Use 'Save Game Profile' after scanning memory to create profiles.");
        }
    }
    
    void captureScreen() {
        setStatus("Initializing screen capture...");
        
        // Initialize screen capture if not already done
        if (!optimizedScreenCapture.initialize()) {
            showError("Capture Error", "Failed to initialize screen capture. Make sure DirectX 11 is available.");
            SetWindowText(hVisionStatusLabel, "Vision: Failed to initialize");
            return;
        }
        
        setStatus("Capturing screen frame...");
        
        // Capture a frame
        OptimizedScreenCapture::FrameData frameData;
        if (optimizedScreenCapture.captureFrame(frameData)) {
            char status[200];
            sprintf(status, "Vision: Captured %dx%d frame (%zu bytes)", frameWidth, frameHeight, lastFrameData.size());
            SetWindowText(hVisionStatusLabel, status);
            
            char infoMsg[200];
            sprintf(infoMsg, "Successfully captured screen frame! Frame size: %dx%d pixels", frameWidth, frameHeight);
            showInfo("Screen Captured", infoMsg);
            setStatus("Screen captured successfully - %dx%d pixels", frameWidth, frameHeight);
        } else {
            showError("Capture Failed", "Failed to capture screen frame. The screen may be protected or not accessible.");
            SetWindowText(hVisionStatusLabel, "Vision: Capture failed");
        }
    }
    
    void startVisionAnalysis() {
        if (lastFrameData.empty()) {
            showWarning("No Frame Available", "Please capture a screen frame first using 'Capture Screen'.");
            return;
        }
        
        if (visionAnalyzing) {
            showWarning("Analysis In Progress", "Vision analysis is already running. Please wait for it to complete.");
            return;
        }
        
        visionAnalyzing = true;
        SetWindowText(hStartVisionAnalysisButton, "Analyzing...");
        EnableWindow(hStartVisionAnalysisButton, FALSE);
        SetWindowText(hVisionStatusLabel, "Vision: Analyzing frame...");
        setStatus("Starting vision analysis...");
        
        // Start vision analysis in a separate thread
        std::thread([this]() {
            try {
                // Simulate vision analysis processing
                setStatus("Analyzing frame data...");
                
                // Basic frame analysis (placeholder for future OCR/vision processing)
                analyzeFrameData();
                
                SetWindowText(hVisionStatusLabel, "Vision: Analysis complete");
                
                // Show detected text results
                if (detectedTexts.empty()) {
                    showInfo("Vision Analysis Complete", "Frame analysis completed successfully, but no text was detected. Try capturing a different area of the screen.");
                } else {
                    std::string textResults = "Detected text regions:\n\n";
                    for (size_t i = 0; i < detectedTexts.size() && i < 10; ++i) {
                        textResults += "- " + detectedTexts[i] + "\n";
                    }
                    if (detectedTexts.size() > 10) {
                        textResults += "... and " + std::to_string(detectedTexts.size() - 10) + " more regions";
                    }
                    
                    showInfo("Vision Analysis Complete", textResults.c_str());
                }
                
                setStatus("Vision analysis completed - %zu text regions found", detectedTexts.size());
                
            } catch (const std::exception& e) {
                showError("Analysis Error", "An error occurred during vision analysis.");
                SetWindowText(hVisionStatusLabel, "Vision: Analysis failed");
            }
            
            visionAnalyzing = false;
            SetWindowText(hStartVisionAnalysisButton, "Start Vision Analysis");
            EnableWindow(hStartVisionAnalysisButton, TRUE);
        }).detach();
    }
    
    void analyzeFrameData() {
        setStatus("Processing frame data with OCR...");
        
        // Run OCR text detection
        cv::Mat frameMat = cv::Mat(frameHeight, frameWidth, CV_8UC3, lastFrameData.data());
        auto textRegions = advancedOCR.detectText(frameMat);
        
        // Store detected texts
        detectedTexts.clear();
        for (const auto& region : textRegions) {
            detectedTexts.push_back(region.text);
        }
        
        setStatus("OCR analysis: Found %zu text regions", textRegions.size());
        
        // Basic frame statistics
        int totalPixels = frameWidth * frameHeight;
        int brightPixels = 0;
        
        // Analyze pixel data for additional insights
        for (size_t i = 0; i < lastFrameData.size(); i += 4) {
            if (i + 2 < lastFrameData.size()) {
                uint8_t r = lastFrameData[i];
                uint8_t g = lastFrameData[i + 1];
                uint8_t b = lastFrameData[i + 2];
                
                // Bright pixels (for potential text/UI detection)
                if (r > 200 || g > 200 || b > 200) {
                    brightPixels++;
                }
            }
        }
        
        // Update vision status with detailed results
        char visionStatus[300];
        sprintf(visionStatus, "Vision: %dx%d, %d bright px, %zu text regions", 
                frameWidth, frameHeight, brightPixels, textRegions.size());
        SetWindowText(hVisionStatusLabel, visionStatus);
        
        setStatus("Vision analysis complete: %zu text regions detected", textRegions.size());
    }
    
    void runHybridAnalysis() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before running hybrid analysis.");
            return;
        }
        
        if (memoryAddresses.empty() && discoveredAddresses.empty()) {
            showWarning("No Memory Data", "Please scan memory first using 'Scan Process Memory' or 'Scan Game Data'.");
            return;
        }
        
        if (detectedTexts.empty()) {
            showWarning("No Vision Data", "Please capture and analyze screen first using 'Capture Screen' and 'Start Vision Analysis'.");
            return;
        }
        
        setStatus("Running hybrid analysis - combining memory and vision data...");
        showProgress(true);
        resetProgress();
        
        // Run hybrid analysis in a separate thread
        std::thread([this]() {
            try {
                setStatus("Analyzing memory values...");
                setProgress(20);
                
                // Analyze memory data
                std::vector<std::string> memoryInsights = analyzeMemoryData();
                
                setStatus("Analyzing vision data...");
                setProgress(50);
                
                // Analyze vision data
                std::vector<std::string> visionInsights = analyzeVisionData();
                
                setStatus("Correlating memory and vision data...");
                setProgress(80);
                
                // Correlate memory and vision data
                std::vector<std::string> correlations = correlateMemoryVision(memoryInsights, visionInsights);
                
                setProgress(100);
                showProgress(false);
                
                // Display hybrid analysis results
                displayHybridResults(memoryInsights, visionInsights, correlations);
                
            } catch (const std::exception& e) {
                showProgress(false);
                showError("Hybrid Analysis Error", "An error occurred during hybrid analysis.");
            }
        }).detach();
    }
    
    std::vector<std::string> analyzeMemoryData() {
        std::vector<std::string> insights;
        
        // Analyze monitored addresses
        for (const auto& memAddr : memoryAddresses) {
            int32_t value;
            if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                std::string interpretation = MemoryScanner::interpretValue(value);
                insights.push_back(memAddr.first + ": " + std::to_string(value) + interpretation);
            }
        }
        
        // Analyze discovered addresses
        for (size_t i = 0; i < discoveredAddresses.size(); ++i) {
            uintptr_t addr = discoveredAddresses[i];
            int32_t value;
            if (MemoryReader::readMemory(selectedProcess->pid, addr, &value, sizeof(value))) {
                std::string interpretation = MemoryScanner::interpretValue(value);
                insights.push_back("Discovered_" + std::to_string(i + 1) + ": " + std::to_string(value) + interpretation);
            }
        }
        
        return insights;
    }
    
    std::vector<std::string> analyzeVisionData() {
        std::vector<std::string> insights;
        
        for (const auto& text : detectedTexts) {
            // Analyze detected text for game-related patterns
            if (isNumeric(text)) {
                insights.push_back("Numeric Text: " + text + " (possible health/score/ammo)");
            } else if (isGameLabel(text)) {
                insights.push_back("Game Label: " + text + " (UI element detected)");
            } else {
                insights.push_back("Text Region: " + text + " (unknown content)");
            }
        }
        
        return insights;
    }
    
    std::vector<std::string> correlateMemoryVision(const std::vector<std::string>& memoryInsights, const std::vector<std::string>& visionInsights) {
        std::vector<std::string> correlations;
        
        // Look for potential correlations between memory values and detected text
        for (const auto& memInsight : memoryInsights) {
            for (const auto& visionInsight : visionInsights) {
                // Check if memory value matches detected text
                if (containsNumericMatch(memInsight, visionInsight)) {
                    correlations.push_back("CORRELATION: " + memInsight + " matches " + visionInsight);
                }
            }
        }
        
        // Add general insights
        if (!correlations.empty()) {
            correlations.insert(correlations.begin(), "Found " + std::to_string(correlations.size()) + " potential correlations between memory and vision data");
        } else {
            correlations.push_back("No direct correlations found - memory and vision data may represent different game aspects");
        }
        
        return correlations;
    }
    
    bool isNumeric(const std::string& text) {
        if (text.empty()) return false;
        for (char c : text) {
            if (!std::isdigit(c)) return false;
        }
        return true;
    }
    
    bool isGameLabel(const std::string& text) {
        std::vector<std::string> gameLabels = {"Health", "Score", "Ammo", "Money", "Level", "XP", "Energy", "Shield"};
        for (const auto& label : gameLabels) {
            if (text.find(label) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    bool containsNumericMatch(const std::string& memory, const std::string& vision) {
        // Extract numbers from both strings and compare
        std::string memNum, visNum;
        
        for (char c : memory) {
            if (std::isdigit(c)) memNum += c;
        }
        
        for (char c : vision) {
            if (std::isdigit(c)) visNum += c;
        }
        
        return !memNum.empty() && !visNum.empty() && memNum == visNum;
    }
    
    void displayHybridResults(const std::vector<std::string>& memoryInsights, const std::vector<std::string>& visionInsights, const std::vector<std::string>& correlations) {
        std::string results = "HYBRID ANALYSIS RESULTS\n\n";
        
        results += "MEMORY INSIGHTS (" + std::to_string(memoryInsights.size()) + "):\n";
        for (size_t i = 0; i < memoryInsights.size() && i < 5; ++i) {
            results += "- " + memoryInsights[i] + "\n";
        }
        if (memoryInsights.size() > 5) {
            results += "... and " + std::to_string(memoryInsights.size() - 5) + " more\n";
        }
        results += "\n";
        
        results += "VISION INSIGHTS (" + std::to_string(visionInsights.size()) + "):\n";
        for (size_t i = 0; i < visionInsights.size() && i < 5; ++i) {
            results += "- " + visionInsights[i] + "\n";
        }
        if (visionInsights.size() > 5) {
            results += "... and " + std::to_string(visionInsights.size() - 5) + " more\n";
        }
        results += "\n";
        
        results += "CORRELATIONS (" + std::to_string(correlations.size()) + "):\n";
        for (const auto& correlation : correlations) {
            results += "- " + correlation + "\n";
        }
        
        showInfo("Hybrid Analysis Complete", results.c_str());
        setStatus("Hybrid analysis completed - %zu memory, %zu vision, %zu correlations", memoryInsights.size(), visionInsights.size(), correlations.size());
    }
    
    void compareMemoryVision() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before comparing memory and vision data.");
            return;
        }
        
        if (memoryAddresses.empty() && discoveredAddresses.empty()) {
            showWarning("No Memory Data", "Please scan memory first using 'Scan Process Memory' or 'Scan Game Data'.");
            return;
        }
        
        if (detectedTexts.empty()) {
            showWarning("No Vision Data", "Please capture and analyze screen first using 'Capture Screen' and 'Start Vision Analysis'.");
            return;
        }
        
        setStatus("Comparing memory values with detected text...");
        
        std::string comparison = "MEMORY vs VISION COMPARISON\n\n";
        
        // Get current memory values
        std::vector<std::pair<std::string, int32_t>> currentValues;
        for (const auto& memAddr : memoryAddresses) {
            int32_t value;
            if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                currentValues.push_back({memAddr.first, value});
            }
        }
        
        comparison += "MEMORY VALUES:\n";
        for (const auto& val : currentValues) {
            comparison += "- " + val.first + ": " + std::to_string(val.second) + "\n";
        }
        
        comparison += "\nDETECTED TEXT:\n";
        for (const auto& text : detectedTexts) {
            comparison += "- " + text + "\n";
        }
        
        comparison += "\nANALYSIS:\n";
        comparison += "- Memory addresses: " + std::to_string(currentValues.size()) + "\n";
        comparison += "- Text regions: " + std::to_string(detectedTexts.size()) + "\n";
        comparison += "- Use 'Run Hybrid Analysis' for detailed correlation analysis\n";
        
        showInfo("Memory vs Vision Comparison", comparison.c_str());
        setStatus("Comparison complete - %zu memory values, %zu text regions", currentValues.size(), detectedTexts.size());
    }
    
    void startAnalytics() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before starting analytics.");
            return;
        }
        
        if (memoryAddresses.empty() && discoveredAddresses.empty()) {
            showWarning("No Data Available", "Please scan memory first to collect data for analytics.");
            return;
        }
        
        setStatus("Starting analytics engine - collecting performance metrics...");
        showProgress(true);
        resetProgress();
        
        // Run analytics in a separate thread
        std::thread([this]() {
            try {
                setStatus("Collecting memory stability metrics...");
                setProgress(25);
                
                // Collect memory stability data
                collectMemoryStability();
                
                setStatus("Analyzing vision accuracy metrics...");
                setProgress(50);
                
                // Collect vision accuracy data
                collectVisionAccuracy();
                
                setStatus("Calculating performance trends...");
                setProgress(75);
                
                // Calculate trends and patterns
                calculateTrends();
                
                setProgress(100);
                showProgress(false);
                
                analytics.totalAnalysisRuns++;
                
                showInfo("Analytics Complete", "Performance metrics collected successfully! Use 'Show Metrics' to view detailed analytics.");
                setStatus("Analytics completed - %d analysis runs performed", analytics.totalAnalysisRuns);
                
            } catch (const std::exception& e) {
                showProgress(false);
                showError("Analytics Error", "An error occurred during analytics collection.");
            }
        }).detach();
    }
    
    void collectMemoryStability() {
        // Collect memory values over time for stability analysis
        for (const auto& memAddr : memoryAddresses) {
            int32_t value;
            if (MemoryReader::readMemory(selectedProcess->pid, memAddr.second, &value, sizeof(value))) {
                analytics.memoryValues.push_back(static_cast<float>(value));
                analytics.timestamps.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
                
                // Track value changes
                std::string addrStr = MemoryScanner::addressToString(memAddr.second);
                analytics.valueChangeCounts[memAddr.first]++;
            }
        }
        
        // Calculate average stability (lower variance = higher stability)
        if (analytics.memoryValues.size() > 1) {
            float sum = 0.0f;
            for (float val : analytics.memoryValues) {
                sum += val;
            }
            float mean = sum / analytics.memoryValues.size();
            
            float variance = 0.0f;
            for (float val : analytics.memoryValues) {
                variance += (val - mean) * (val - mean);
            }
            variance /= analytics.memoryValues.size();
            
            // Stability score (0-100, higher is more stable)
            analytics.averageMemoryStability = std::max(0.0f, 100.0f - (variance / 1000.0f));
        }
    }
    
    void collectVisionAccuracy() {
        // Analyze vision detection accuracy
        int totalRegions = detectedTexts.size();
        int confidentRegions = 0;
        
        for (const auto& text : detectedTexts) {
            // Simple confidence scoring based on text characteristics
            float confidence = 0.0f;
            
            if (isNumeric(text)) {
                confidence = 0.9f; // Numeric text is usually accurate
            } else if (isGameLabel(text)) {
                confidence = 0.8f; // Game labels are usually accurate
            } else if (!text.empty()) {
                confidence = 0.6f; // Other text has lower confidence
            }
            
            analytics.visionConfidence.push_back(confidence);
            if (confidence > 0.7f) {
                confidentRegions++;
            }
        }
        
        // Calculate average vision accuracy
        if (!analytics.visionConfidence.empty()) {
            float sum = 0.0f;
            for (float conf : analytics.visionConfidence) {
                sum += conf;
            }
            analytics.averageVisionAccuracy = sum / analytics.visionConfidence.size() * 100.0f;
        }
    }
    
    void calculateTrends() {
        // Calculate trends for memory values
        if (analytics.memoryValues.size() >= 3) {
            // Simple linear trend calculation
            int n = analytics.memoryValues.size();
            float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
            
            for (int i = 0; i < n; i++) {
                sumX += i;
                sumY += analytics.memoryValues[i];
                sumXY += i * analytics.memoryValues[i];
                sumX2 += i * i;
            }
            
            float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
            analytics.valueTrends["Memory"] = slope;
        }
        
        // Calculate trends for vision confidence
        if (analytics.visionConfidence.size() >= 3) {
            int n = analytics.visionConfidence.size();
            float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
            
            for (int i = 0; i < n; i++) {
                sumX += i;
                sumY += analytics.visionConfidence[i];
                sumXY += i * analytics.visionConfidence[i];
                sumX2 += i * i;
            }
            
            float slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
            analytics.valueTrends["Vision"] = slope;
        }
    }
    
    void showMetrics() {
        if (analytics.totalAnalysisRuns == 0) {
            showWarning("No Analytics Data", "Please run 'Start Analytics' first to collect performance metrics.");
            return;
        }
        
        std::string metrics = "PERFORMANCE ANALYTICS REPORT\n\n";
        
        metrics += "OVERVIEW:\n";
        metrics += "- Total Analysis Runs: " + std::to_string(analytics.totalAnalysisRuns) + "\n";
        metrics += "- Memory Samples: " + std::to_string(analytics.memoryValues.size()) + "\n";
        metrics += "- Vision Samples: " + std::to_string(analytics.visionConfidence.size()) + "\n\n";
        
        metrics += "PERFORMANCE METRICS:\n";
        metrics += "- Memory Stability: " + std::to_string((int)analytics.averageMemoryStability) + "%\n";
        metrics += "- Vision Accuracy: " + std::to_string((int)analytics.averageVisionAccuracy) + "%\n\n";
        
        metrics += "TRENDS:\n";
        for (const auto& trend : analytics.valueTrends) {
            std::string trendDirection = trend.second > 0 ? "Increasing" : (trend.second < 0 ? "Decreasing" : "Stable");
            metrics += "- " + trend.first + ": " + trendDirection + " (" + std::to_string(trend.second) + ")\n";
        }
        
        metrics += "\nVALUE CHANGES:\n";
        for (const auto& change : analytics.valueChangeCounts) {
            metrics += "- " + change.first + ": " + std::to_string(change.second) + " changes\n";
        }
        
        showInfo("Performance Analytics", metrics.c_str());
        setStatus("Analytics report displayed - %d runs, %.1f%% stability, %.1f%% accuracy", 
                 analytics.totalAnalysisRuns, analytics.averageMemoryStability, analytics.averageVisionAccuracy);
    }
    
    void exportAnalytics() {
        if (analytics.totalAnalysisRuns == 0) {
            showWarning("No Analytics Data", "Please run 'Start Analytics' first to collect data for export.");
            return;
        }
        
        setStatus("Exporting analytics data to CSV...");
        
        FILE* file = fopen("analytics_report.csv", "w");
        if (file) {
            // Write CSV header
            fprintf(file, "Timestamp,Memory_Value,Vision_Confidence,Memory_Stability,Vision_Accuracy,Analysis_Run\n");
            
            // Write data points
            size_t maxSamples = std::max(analytics.memoryValues.size(), analytics.visionConfidence.size());
            for (size_t i = 0; i < maxSamples; ++i) {
                int64_t timestamp = (i < analytics.timestamps.size()) ? analytics.timestamps[i] : 0;
                float memoryVal = (i < analytics.memoryValues.size()) ? analytics.memoryValues[i] : 0.0f;
                float visionConf = (i < analytics.visionConfidence.size()) ? analytics.visionConfidence[i] : 0.0f;
                
                fprintf(file, "%lld,%.2f,%.2f,%.2f,%.2f,%d\n", 
                        timestamp, memoryVal, visionConf, 
                        analytics.averageMemoryStability, analytics.averageVisionAccuracy, 
                        analytics.totalAnalysisRuns);
            }
            
            fclose(file);
            
            showInfo("Analytics Exported", "Analytics data exported to 'analytics_report.csv'. You can open this in Excel or Google Sheets for detailed analysis.");
            setStatus("Analytics exported to analytics_report.csv");
        } else {
            showError("Export Error", "Could not create analytics export file!");
        }
    }
    
    void openDashboard() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before opening the dashboard.");
            return;
        }
        
        if (analytics.totalAnalysisRuns == 0) {
            showWarning("No Analytics Data", "Please run 'Start Analytics' first to collect data for the dashboard.");
            return;
        }
        
        setStatus("Opening professional analytics dashboard...");
        
        // Create comprehensive dashboard report
        std::string dashboard = " PROFESSIONAL GAMING ANALYTICS DASHBOARD \n";
        dashboard += "===========================================================\n\n";
        
        dashboard += " GAME: " + selectedProcess->name + " (PID: " + std::to_string(selectedProcess->pid) + ")\n";
        dashboard += " Session Duration: " + std::to_string(analytics.totalAnalysisRuns) + " analysis cycles\n\n";
        
        dashboard += " PERFORMANCE OVERVIEW:\n";
        dashboard += "------------------------------------------------------------------\n";
        dashboard += "- Memory Stability: " + std::to_string((int)analytics.averageMemoryStability) + "% ";
        if (analytics.averageMemoryStability > 80) dashboard += "🟢 EXCELLENT\n";
        else if (analytics.averageMemoryStability > 60) dashboard += "🟡 GOOD\n";
        else dashboard += "🔴 NEEDS ATTENTION\n";
        
        dashboard += "- Vision Accuracy: " + std::to_string((int)analytics.averageVisionAccuracy) + "% ";
        if (analytics.averageVisionAccuracy > 80) dashboard += "🟢 EXCELLENT\n";
        else if (analytics.averageVisionAccuracy > 60) dashboard += "🟡 GOOD\n";
        else dashboard += "🔴 NEEDS ATTENTION\n";
        
        dashboard += "- Data Points: " + std::to_string(analytics.memoryValues.size()) + " memory, " + 
                    std::to_string(analytics.visionConfidence.size()) + " vision\n\n";
        
        dashboard += " TREND ANALYSIS:\n";
        dashboard += "------------------------------------------------------------------\n";
        for (const auto& trend : analytics.valueTrends) {
            std::string trendIcon = trend.second > 0 ? "" : (trend.second < 0 ? "" : "→");
            std::string trendDirection = trend.second > 0 ? "RISING" : (trend.second < 0 ? "FALLING" : "STABLE");
            dashboard += trendIcon + " " + trend.first + ": " + trendDirection + " (slope: " + std::to_string(trend.second) + ")\n";
        }
        
        dashboard += "\n GAME STATE INSIGHTS:\n";
        dashboard += "------------------------------------------------------------------\n";
        
        // Generate insights based on analytics
        if (analytics.averageMemoryStability > 80) {
            dashboard += "✅ Game memory is highly stable - consistent performance\n";
        }
        if (analytics.averageVisionAccuracy > 80) {
            dashboard += "✅ UI detection is highly accurate - reliable text recognition\n";
        }
        if (analytics.valueChangeCounts.size() > 0) {
            dashboard += " " + std::to_string(analytics.valueChangeCounts.size()) + " memory addresses being tracked\n";
        }
        
        dashboard += "\n RECOMMENDATIONS:\n";
        dashboard += "------------------------------------------------------------------\n";
        if (analytics.averageMemoryStability < 70) {
            dashboard += "WARNING: Consider running as Administrator for better memory access\n";
        }
        if (analytics.averageVisionAccuracy < 70) {
            dashboard += "WARNING: Try capturing different screen areas for better text detection\n";
        }
        dashboard += " Continue monitoring for performance optimization opportunities\n";
        
        showInfo("Professional Analytics Dashboard", dashboard.c_str());
        setStatus("Dashboard displayed - Professional analytics overview complete");
    }
    
    void showRealTimeCharts() {
        if (analytics.totalAnalysisRuns == 0) {
            showWarning("No Analytics Data", "Please run 'Start Analytics' first to generate chart data.");
            return;
        }
        
        setStatus("Generating real-time performance charts...");
        
        // Create ASCII-style charts for real-time visualization
        std::string charts = " REAL-TIME PERFORMANCE CHARTS \n";
        charts += "===========================================================\n\n";
        
        // Memory Values Chart
        charts += " MEMORY VALUES TREND:\n";
        charts += "------------------------------------------------------------------\n";
        
        if (!analytics.memoryValues.empty()) {
            // Create ASCII bar chart
            float maxVal = *std::max_element(analytics.memoryValues.begin(), analytics.memoryValues.end());
            float minVal = *std::min_element(analytics.memoryValues.begin(), analytics.memoryValues.end());
            float range = maxVal - minVal;
            
            for (size_t i = 0; i < std::min(analytics.memoryValues.size(), (size_t)20); ++i) {
                float normalized = range > 0 ? (analytics.memoryValues[i] - minVal) / range : 0.5f;
                int barLength = (int)(normalized * 40);
                
                charts += "Sample " + std::to_string(i + 1) + ": ";
                for (int j = 0; j < barLength; ++j) {
                    charts += "█";
                }
                charts += " " + std::to_string((int)analytics.memoryValues[i]) + "\n";
            }
        } else {
            charts += "No memory data available for charting\n";
        }
        
        charts += "\n VISION ACCURACY TREND:\n";
        charts += "------------------------------------------------------------------\n";
        
        if (!analytics.visionConfidence.empty()) {
            for (size_t i = 0; i < std::min(analytics.visionConfidence.size(), (size_t)15); ++i) {
                int accuracy = (int)(analytics.visionConfidence[i] * 100);
                int barLength = accuracy / 2;
                
                charts += "Detection " + std::to_string(i + 1) + ": ";
                for (int j = 0; j < barLength; ++j) {
                    charts += "█";
                }
                charts += " " + std::to_string(accuracy) + "%\n";
            }
        } else {
            charts += "No vision data available for charting\n";
        }
        
        charts += "\n PERFORMANCE SUMMARY:\n";
        charts += "------------------------------------------------------------------\n";
        charts += "- Memory Stability: " + std::to_string((int)analytics.averageMemoryStability) + "%\n";
        charts += "- Vision Accuracy: " + std::to_string((int)analytics.averageVisionAccuracy) + "%\n";
        charts += "- Total Samples: " + std::to_string(analytics.memoryValues.size() + analytics.visionConfidence.size()) + "\n";
        charts += "- Analysis Runs: " + std::to_string(analytics.totalAnalysisRuns) + "\n";
        
        showInfo("Real-time Performance Charts", charts.c_str());
        setStatus("Real-time charts generated - %zu memory samples, %zu vision samples", 
                 analytics.memoryValues.size(), analytics.visionConfidence.size());
    }
    
    void showPerformanceMonitor() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before opening performance monitor.");
            return;
        }
        
        setStatus("Initializing performance monitor...");
        
        // Create performance monitoring dashboard
        std::string monitor = " REAL-TIME PERFORMANCE MONITOR \n";
        monitor += "===========================================================\n\n";
        
        monitor += " TARGET: " + selectedProcess->name + " (PID: " + std::to_string(selectedProcess->pid) + ")\n\n";
        
        monitor += " CURRENT STATUS:\n";
        monitor += "------------------------------------------------------------------\n";
        
        // Memory monitoring status
        monitor += " MEMORY ANALYSIS:\n";
        monitor += "- Monitored Addresses: " + std::to_string(memoryAddresses.size()) + "\n";
        monitor += "- Discovered Addresses: " + std::to_string(discoveredAddresses.size()) + "\n";
        monitor += "- Memory Regions: " + std::to_string(memoryRegions.size()) + "\n";
        if (analytics.totalAnalysisRuns > 0) {
            monitor += "- Stability Score: " + std::to_string((int)analytics.averageMemoryStability) + "%\n";
        }
        
        // Vision monitoring status
        monitor += "\n VISION ANALYSIS:\n";
        monitor += "- Captured Frames: " + (lastFrameData.empty() ? "None" : std::to_string(frameWidth) + "x" + std::to_string(frameHeight)) + "\n";
        monitor += "- Detected Text Regions: " + std::to_string(detectedTexts.size()) + "\n";
        if (analytics.totalAnalysisRuns > 0) {
            monitor += "- Accuracy Score: " + std::to_string((int)analytics.averageVisionAccuracy) + "%\n";
        }
        
        // Hybrid analysis status
        monitor += "\n🔗 HYBRID ANALYSIS:\n";
        monitor += "- Analysis Runs: " + std::to_string(analytics.totalAnalysisRuns) + "\n";
        monitor += "- Data Correlations: " + std::to_string(analytics.valueChangeCounts.size()) + "\n";
        monitor += "- Trend Analysis: " + std::to_string(analytics.valueTrends.size()) + " metrics\n";
        
        // System performance
        monitor += "\n⚡ SYSTEM PERFORMANCE:\n";
        monitor += "- Process Monitoring: " + std::string(monitoring ? "🟢 ACTIVE" : "🔴 INACTIVE") + "\n";
        monitor += "- Memory Scanning: " + std::string(scanning ? "🟡 IN PROGRESS" : "🟢 READY") + "\n";
        monitor += "- Vision Analysis: " + std::string(visionAnalyzing ? "🟡 IN PROGRESS" : "🟢 READY") + "\n";
        
        // Recommendations
        monitor += "\n PERFORMANCE RECOMMENDATIONS:\n";
        monitor += "------------------------------------------------------------------\n";
        
        if (memoryAddresses.empty() && discoveredAddresses.empty()) {
            monitor += "WARNING: No memory data - Run 'Scan Process Memory' or 'Scan Game Data'\n";
        }
        if (detectedTexts.empty()) {
            monitor += "WARNING: No vision data - Run 'Capture Screen' and 'Start Vision Analysis'\n";
        }
        if (analytics.totalAnalysisRuns == 0) {
            monitor += "WARNING: No analytics data - Run 'Start Analytics' for performance metrics\n";
        }
        
        if (!memoryAddresses.empty() && !detectedTexts.empty() && analytics.totalAnalysisRuns > 0) {
            monitor += "✅ All systems operational - Professional analysis ready\n";
            monitor += " Run 'Hybrid Analysis' for comprehensive game insights\n";
        }
        
        showInfo("Performance Monitor", monitor.c_str());
        setStatus("Performance monitor displayed - System status overview complete");
    }
    
    void showAboutDialog() {
        PopupDialogs::showAboutDialog(this);
    }
    
    void toggleDarkMode() {
        modernThemeEnabled = !modernThemeEnabled;
        SendMessage(hDarkModeToggle, BM_SETCHECK, modernThemeEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
        
        // Update theme brushes
        updateThemeBrushes();
        
        // Note: Dialog theme updates handled in dialog creation
        
        // Refresh the window to apply dark mode
        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);
    }
    
    // Dialog theme updates are handled during dialog creation
    
    void toggleSystemProcesses() {
        showSystemProcesses = !showSystemProcesses;
        SendMessage(hSystemProcessesToggle, BM_SETCHECK, showSystemProcesses ? BST_CHECKED : BST_UNCHECKED, 0);
        
        // Refresh process list
        refreshProcesses();
        refreshProcessList();
        
        setStatus("System processes %s", showSystemProcesses ? "enabled" : "disabled");
    }
    
    void showSettingsDialog() {
        PopupDialogs::showSettingsDialog(this);
    }
    
    void showHelpDialog() {
        PopupDialogs::showHelpDialog(this);
    }
    
    void scanGameData() {
        if (!selectedProcess) {
            showWarning("No Process Selected", "Please select a process from the list before scanning for game data.");
            return;
        }
        
        if (scanning) {
            showWarning("Scan In Progress", "Memory scan is already in progress. Please wait for it to complete.");
            return;
        }
        
        scanning = true;
        SetWindowText(hScanGameDataButton, "Scanning...");
        EnableWindow(hScanGameDataButton, FALSE);
        showProgress(true);
        resetProgress();
        setStatus("Initializing game data scan...");
        
        // Clear previous results
        SendMessage(hAddressListBox, LB_RESETCONTENT, 0, 0);
        discoveredAddresses.clear();
        addressChecked.clear();
        
        // Start scanning in a separate thread
        std::thread([this]() {
            try {
                // Step 1: Get memory regions
                setStatus("Discovering memory regions...");
                setProgress(15);
                memoryRegions = MemoryScanner::scanProcessMemory(selectedProcess->pid);
                
                if (memoryRegions.empty()) {
                    showError("No Memory Regions", "No readable memory regions found in the selected process. The process may be protected or not have accessible memory.");
                    goto cleanup;
                }
                
                setStatus("Found %zu memory regions, analyzing for game data patterns...", memoryRegions.size());
                setProgress(40);
                
                // Step 2: Find game data addresses (more focused scan)
                discoveredAddresses = MemoryScanner::findGameDataAddresses(selectedProcess->pid, memoryRegions, 200);
                setProgress(80);
                
                // Initialize checkbox state
                addressChecked.resize(discoveredAddresses.size(), false);
                
                // Step 3: Populate the address list using filter method
                filterAddressList();
                setProgress(95);
                
                setStatus("Game data scan complete! Found %zu potential game variables", discoveredAddresses.size());
                setProgress(100);
                
                if (discoveredAddresses.empty()) {
                    showWarning("No Game Data Found", "No potential game data was found. Try running as Administrator or use 'Scan Process Memory' for a broader search.");
                } else {
                    showInfo("Game Data Scan Complete", "Found potential game variables! Use 'Compare Values' to capture baseline, then interact with the game and click 'Show Changed Values'.");
                }
                
            } catch (const std::exception& e) {
                showError("Scan Error", "An error occurred during game data scanning. Please try again or run as Administrator.");
            }
            
        cleanup:
            scanning = false;
            SetWindowText(hScanGameDataButton, "Scan Game Data");
            EnableWindow(hScanGameDataButton, TRUE);
            showProgress(false);
        }).detach();
    }
    
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        RealGameAnalyzerGUI* pThis = nullptr;
        
        if (uMsg == WM_NCCREATE) {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (RealGameAnalyzerGUI*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
        } else {
            pThis = (RealGameAnalyzerGUI*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        
        if (pThis) {
            switch (uMsg) {
                case WM_CTLCOLORSTATIC:
                case WM_CTLCOLOREDIT:
                case WM_CTLCOLORLISTBOX:
                    {
                        HDC hdc = (HDC)wParam;
                        if (pThis->modernThemeEnabled) {
                            SetBkColor(hdc, ModernTheme::DARK_BACKGROUND_PRIMARY);
                            SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                            return (LRESULT)pThis->hBackgroundBrush;
                        } else {
                            SetBkColor(hdc, ModernTheme::BACKGROUND_PRIMARY);
                            SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                            return (LRESULT)pThis->hBackgroundBrush;
                        }
                    }
                    break;
                case WM_CTLCOLORBTN:
                    {
                        HDC hdc = (HDC)wParam;
                        if (pThis->modernThemeEnabled) {
                            SetBkColor(hdc, ModernTheme::DARK_BUTTON_NORMAL);
                            SetTextColor(hdc, ModernTheme::DARK_TEXT_PRIMARY);
                            return (LRESULT)pThis->hCardBrush;
                        } else {
                            SetBkColor(hdc, ModernTheme::BUTTON_NORMAL);
                            SetTextColor(hdc, ModernTheme::TEXT_PRIMARY);
                            return (LRESULT)pThis->hCardBrush;
                        }
                    }
                    break;
                case WM_DRAWITEM:
                    {
                        LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;
                        if (lpDrawItem->CtlType == ODT_BUTTON) {
                            pThis->drawModernButton(lpDrawItem);
                            return TRUE;
                        }
                    }
                    break;
                case WM_PAINT:
                    {
                        PAINTSTRUCT ps;
                        HDC hdc = BeginPaint(hwnd, &ps);
                        RECT rect;
                        GetClientRect(hwnd, &rect);
                        if (pThis->modernThemeEnabled) {
                            FillRect(hdc, &rect, pThis->hBackgroundBrush);
                        } else {
                            FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1));
                        }
                        EndPaint(hwnd, &ps);
                        return 0;
                    }
                    break;
                case WM_COMMAND:
                    if (HIWORD(wParam) == BN_CLICKED) {
                        pThis->onButtonClick(LOWORD(wParam));
                    } else if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 10) {
                        pThis->onProcessSelection();
                    } else if (HIWORD(wParam) == LBN_DBLCLK && LOWORD(wParam) == 10) {
                        // Double-click on process list
                        pThis->onProcessSelection();
                    } else if (HIWORD(wParam) == LBN_SELCHANGE && LOWORD(wParam) == 0) {
                        // Click on address list - toggle checkbox
                        pThis->toggleAddressCheckbox();
                    } else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 6) {
                        // Search text changed
                        pThis->refreshProcessList();
                    } else if (HIWORD(wParam) == EN_CHANGE && LOWORD(wParam) == 15) {
                        // Address search text changed
                        pThis->filterAddressList();
                    }
                    break;
                case WM_DESTROY:
                    PostQuitMessage(0);
                    return 0;
            }
        }
        
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Create a mutex to prevent multiple instances
    HANDLE hMutex = CreateMutex(nullptr, TRUE, "GameAnalyzerSingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(nullptr, "Game Analyzer is already running!", "Error", MB_OK | MB_ICONWARNING);
        return 1;
    }
    
    RealGameAnalyzerGUI app;
    
    if (!app.createWindow()) {
        MessageBox(nullptr, "Failed to create window!", "Error", MB_OK | MB_ICONERROR);
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
        return 1;
    }
    
    app.run();
    
    // Cleanup
    ReleaseMutex(hMutex);
    CloseHandle(hMutex);
    return 0;
}
