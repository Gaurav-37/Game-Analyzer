#include "cuda_support.h"
#include <iostream>
#include <opencv2/opencv.hpp>

bool CudaSupport::initialized = false;
int CudaSupport::currentDevice = -1;

bool CudaSupport::isAvailable() {
    return cv::cuda::getCudaEnabledDeviceCount() > 0;
}

int CudaSupport::getDeviceCount() {
    return cv::cuda::getCudaEnabledDeviceCount();
}

std::string CudaSupport::getDeviceName(int device) {
    if (!isAvailable() || device >= getDeviceCount()) {
        return "No CUDA Device";
    }
    
    cv::cuda::DeviceInfo info(device);
    return info.name();
}

bool CudaSupport::hasMinComputeCapability(int major, int minor) {
    if (!isAvailable()) return false;
    
    for (int i = 0; i < getDeviceCount(); ++i) {
        cv::cuda::DeviceInfo info(i);
        if (info.majorVersion() > major || 
            (info.majorVersion() == major && info.minorVersion() >= minor)) {
            return true;
        }
    }
    return false;
}

void CudaSupport::selectBestDevice() {
    if (!isAvailable()) return;
    
    int bestDevice = 0;
    size_t bestMemory = 0;
    
    for (int i = 0; i < getDeviceCount(); ++i) {
        cv::cuda::DeviceInfo info(i);
        size_t freeMemory = info.freeMemory();
        
        if (freeMemory > bestMemory) {
            bestMemory = freeMemory;
            bestDevice = i;
        }
    }
    
    cv::cuda::setDevice(bestDevice);
    currentDevice = bestDevice;
}

bool CudaSupport::initialize() {
    if (!isAvailable()) {
        std::cout << "CUDA not available, using CPU fallback" << std::endl;
        return false;
    }
    
    selectBestDevice();
    cv::cuda::setDevice(currentDevice);
    initialized = true;
    
    std::cout << "CUDA initialized on device: " << getDeviceName(currentDevice) << std::endl;
    return true;
}

void CudaSupport::cleanup() {
    if (initialized) {
        cv::cuda::resetDevice();
        initialized = false;
        currentDevice = -1;
    }
}

int CudaSupport::getCurrentDevice() {
    return currentDevice;
}

void CudaSupport::setDevice(int device) {
    if (device >= 0 && device < getDeviceCount()) {
        cv::cuda::setDevice(device);
        currentDevice = device;
    }
}

size_t CudaSupport::getFreeMemory() {
    if (!isAvailable()) return 0;
    
    cv::cuda::DeviceInfo info(currentDevice >= 0 ? currentDevice : 0);
    return info.freeMemory();
}

size_t CudaSupport::getTotalMemory() {
    if (!isAvailable()) return 0;
    
    cv::cuda::DeviceInfo info(currentDevice >= 0 ? currentDevice : 0);
    return info.totalMemory();
}
