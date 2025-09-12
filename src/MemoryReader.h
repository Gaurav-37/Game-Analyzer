#pragma once

#include <windows.h>
#include <string>
#include <vector>

class MemoryReader {
private:
    bool initialized;
    
public:
    MemoryReader();
    ~MemoryReader();
    
    bool initialize();
    void shutdown();
    
    // Read memory from a process
    template<typename T>
    bool readMemory(DWORD processId, uintptr_t address, T* buffer, size_t size = sizeof(T)) {
        return readMemory(processId, address, static_cast<void*>(buffer), size);
    }
    
    bool readMemory(DWORD processId, uintptr_t address, void* buffer, size_t size);
    
    // Read different data types
    bool readInt32(DWORD processId, uintptr_t address, int32_t* value);
    bool readFloat(DWORD processId, uintptr_t address, float* value);
    bool readDouble(DWORD processId, uintptr_t address, double* value);
    bool readString(DWORD processId, uintptr_t address, std::string& value, size_t maxLength = 256);
    
    // Memory scanning functions
    std::vector<uintptr_t> scanForValue(DWORD processId, const void* value, size_t valueSize, 
                                       uintptr_t startAddress = 0, size_t scanSize = 0);
    
    // Process memory information
    bool getMemoryInfo(DWORD processId, uintptr_t address, MEMORY_BASIC_INFORMATION& mbi);
    std::vector<MEMORY_BASIC_INFORMATION> getMemoryRegions(DWORD processId);
    
private:
    HANDLE openProcess(DWORD processId);
    void closeProcess(HANDLE processHandle);
};
