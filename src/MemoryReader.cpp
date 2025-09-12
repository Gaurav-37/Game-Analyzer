#include "MemoryReader.h"
#include <iostream>
#include <algorithm>

MemoryReader::MemoryReader() : initialized(false) {
}

MemoryReader::~MemoryReader() {
    shutdown();
}

bool MemoryReader::initialize() {
    initialized = true;
    return true;
}

void MemoryReader::shutdown() {
    initialized = false;
}

bool MemoryReader::readMemory(DWORD processId, uintptr_t address, void* buffer, size_t size) {
    if (!initialized) {
        return false;
    }
    
    HANDLE processHandle = openProcess(processId);
    if (processHandle == nullptr) {
        return false;
    }
    
    SIZE_T bytesRead = 0;
    BOOL result = ReadProcessMemory(
        processHandle,
        reinterpret_cast<LPCVOID>(address),
        buffer,
        size,
        &bytesRead
    );
    
    closeProcess(processHandle);
    
    return result && (bytesRead == size);
}

bool MemoryReader::readInt32(DWORD processId, uintptr_t address, int32_t* value) {
    return readMemory(processId, address, value, sizeof(int32_t));
}

bool MemoryReader::readFloat(DWORD processId, uintptr_t address, float* value) {
    return readMemory(processId, address, value, sizeof(float));
}

bool MemoryReader::readDouble(DWORD processId, uintptr_t address, double* value) {
    return readMemory(processId, address, value, sizeof(double));
}

bool MemoryReader::readString(DWORD processId, uintptr_t address, std::string& value, size_t maxLength) {
    if (!initialized) {
        return false;
    }
    
    HANDLE processHandle = openProcess(processId);
    if (processHandle == nullptr) {
        return false;
    }
    
    std::vector<char> buffer(maxLength + 1, 0);
    SIZE_T bytesRead = 0;
    
    BOOL result = ReadProcessMemory(
        processHandle,
        reinterpret_cast<LPCVOID>(address),
        buffer.data(),
        maxLength,
        &bytesRead
    );
    
    closeProcess(processHandle);
    
    if (result && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        value = std::string(buffer.data());
        return true;
    }
    
    return false;
}

std::vector<uintptr_t> MemoryReader::scanForValue(DWORD processId, const void* value, size_t valueSize, 
                                                 uintptr_t startAddress, size_t scanSize) {
    std::vector<uintptr_t> results;
    
    if (!initialized) {
        return results;
    }
    
    HANDLE processHandle = openProcess(processId);
    if (processHandle == nullptr) {
        return results;
    }
    
    // If no start address specified, start from 0
    if (startAddress == 0) {
        startAddress = 0x10000; // Skip first 64KB
    }
    
    // If no scan size specified, scan up to 2GB
    if (scanSize == 0) {
        scanSize = 0x7FFFFFFF - startAddress;
    }
    
    const size_t bufferSize = 4096; // 4KB buffer
    std::vector<uint8_t> buffer(bufferSize);
    
    for (uintptr_t currentAddress = startAddress; 
         currentAddress < startAddress + scanSize; 
         currentAddress += bufferSize) {
        
        SIZE_T bytesRead = 0;
        BOOL result = ReadProcessMemory(
            processHandle,
            reinterpret_cast<LPCVOID>(currentAddress),
            buffer.data(),
            bufferSize,
            &bytesRead
        );
        
        if (!result || bytesRead == 0) {
            continue;
        }
        
        // Search for the value in the buffer
        for (size_t i = 0; i <= bytesRead - valueSize; ++i) {
            if (memcmp(buffer.data() + i, value, valueSize) == 0) {
                results.push_back(currentAddress + i);
            }
        }
    }
    
    closeProcess(processHandle);
    return results;
}

bool MemoryReader::getMemoryInfo(DWORD processId, uintptr_t address, MEMORY_BASIC_INFORMATION& mbi) {
    if (!initialized) {
        return false;
    }
    
    HANDLE processHandle = openProcess(processId);
    if (processHandle == nullptr) {
        return false;
    }
    
    SIZE_T result = VirtualQueryEx(
        processHandle,
        reinterpret_cast<LPCVOID>(address),
        &mbi,
        sizeof(MEMORY_BASIC_INFORMATION)
    );
    
    closeProcess(processHandle);
    
    return result == sizeof(MEMORY_BASIC_INFORMATION);
}

std::vector<MEMORY_BASIC_INFORMATION> MemoryReader::getMemoryRegions(DWORD processId) {
    std::vector<MEMORY_BASIC_INFORMATION> regions;
    
    if (!initialized) {
        return regions;
    }
    
    HANDLE processHandle = openProcess(processId);
    if (processHandle == nullptr) {
        return regions;
    }
    
    uintptr_t address = 0;
    MEMORY_BASIC_INFORMATION mbi;
    
    while (address < 0x7FFFFFFF) { // 2GB limit
        SIZE_T result = VirtualQueryEx(
            processHandle,
            reinterpret_cast<LPCVOID>(address),
            &mbi,
            sizeof(MEMORY_BASIC_INFORMATION)
        );
        
        if (result == 0) {
            break;
        }
        
        // Only include committed, readable regions
        if (mbi.State == MEM_COMMIT && 
            (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            regions.push_back(mbi);
        }
        
        address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    }
    
    closeProcess(processHandle);
    return regions;
}

HANDLE MemoryReader::openProcess(DWORD processId) {
    return OpenProcess(
        PROCESS_VM_READ | PROCESS_QUERY_INFORMATION,
        FALSE,
        processId
    );
}

void MemoryReader::closeProcess(HANDLE processHandle) {
    if (processHandle != nullptr) {
        CloseHandle(processHandle);
    }
}
