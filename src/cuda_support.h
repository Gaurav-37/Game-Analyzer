#pragma once

#include <string>
#include <vector>

class CudaSupport {
public:
    static bool isAvailable();
    static int getDeviceCount();
    static std::string getDeviceName(int device);
    static bool hasMinComputeCapability(int major, int minor);
    static void selectBestDevice();
    static bool initialize();
    static void cleanup();
    
    // Device information
    static int getCurrentDevice();
    static void setDevice(int device);
    
    // Memory management
    static size_t getFreeMemory();
    static size_t getTotalMemory();
    
private:
    static bool initialized;
    static int currentDevice;
};
