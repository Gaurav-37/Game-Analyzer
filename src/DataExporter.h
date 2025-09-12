#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include "GameAnalyzer.h"

class DataExporter {
private:
    bool initialized;
    std::string defaultDirectory;
    
public:
    DataExporter();
    ~DataExporter();
    
    bool initialize();
    void shutdown();
    
    // Export data to CSV format
    bool exportToCSV(const std::vector<MemoryAddress>& addresses, const std::string& filename);
    
    // Export data to JSON format
    bool exportToJSON(const std::vector<MemoryAddress>& addresses, const std::string& filename);
    
    // Export data to binary format (for fast loading)
    bool exportToBinary(const std::vector<MemoryAddress>& addresses, const std::string& filename);
    
    // Load data from binary format
    bool loadFromBinary(std::vector<MemoryAddress>& addresses, const std::string& filename);
    
    // Set default export directory
    void setDefaultDirectory(const std::string& directory);
    
    // Get available export formats
    std::vector<std::string> getExportFormats();
    
    // Create filename with timestamp
    std::string createTimestampedFilename(const std::string& prefix, const std::string& extension);
    
private:
    // Helper functions
    std::string formatTimestamp();
    bool createDirectoryIfNotExists(const std::string& path);
    std::string sanitizeFilename(const std::string& filename);
    
    // CSV specific functions
    void writeCSVHeader(std::ofstream& file, const std::vector<MemoryAddress>& addresses);
    void writeCSVData(std::ofstream& file, const std::vector<MemoryAddress>& addresses);
    
    // JSON specific functions
    void writeJSONHeader(std::ofstream& file);
    void writeJSONAddresses(std::ofstream& file, const std::vector<MemoryAddress>& addresses);
    void writeJSONFooter(std::ofstream& file);
};
