#include "DataExporter.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <ctime>

DataExporter::DataExporter() : initialized(false), defaultDirectory("exports") {
}

DataExporter::~DataExporter() {
    shutdown();
}

bool DataExporter::initialize() {
    initialized = true;
    createDirectoryIfNotExists(defaultDirectory);
    return true;
}

void DataExporter::shutdown() {
    initialized = false;
}

bool DataExporter::exportToCSV(const std::vector<MemoryAddress>& addresses, const std::string& filename) {
    if (!initialized || addresses.empty()) {
        return false;
    }
    
    std::string fullPath = defaultDirectory + "/" + sanitizeFilename(filename);
    if (!fullPath.ends_with(".csv")) {
        fullPath += ".csv";
    }
    
    std::ofstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        writeCSVHeader(file, addresses);
        writeCSVData(file, addresses);
        file.close();
        return true;
    } catch (const std::exception& e) {
        file.close();
        return false;
    }
}

bool DataExporter::exportToJSON(const std::vector<MemoryAddress>& addresses, const std::string& filename) {
    if (!initialized || addresses.empty()) {
        return false;
    }
    
    std::string fullPath = defaultDirectory + "/" + sanitizeFilename(filename);
    if (!fullPath.ends_with(".json")) {
        fullPath += ".json";
    }
    
    std::ofstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        writeJSONHeader(file);
        writeJSONAddresses(file, addresses);
        writeJSONFooter(file);
        file.close();
        return true;
    } catch (const std::exception& e) {
        file.close();
        return false;
    }
}

bool DataExporter::exportToBinary(const std::vector<MemoryAddress>& addresses, const std::string& filename) {
    if (!initialized || addresses.empty()) {
        return false;
    }
    
    std::string fullPath = defaultDirectory + "/" + sanitizeFilename(filename);
    if (!fullPath.ends_with(".bin")) {
        fullPath += ".bin";
    }
    
    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        // Write header
        uint32_t version = 1;
        uint32_t addressCount = static_cast<uint32_t>(addresses.size());
        
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));
        file.write(reinterpret_cast<const char*>(&addressCount), sizeof(addressCount));
        
        // Write each address
        for (const auto& address : addresses) {
            // Write address info
            uint32_t nameLength = static_cast<uint32_t>(address.name.length());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(address.name.c_str(), nameLength);
            
            file.write(reinterpret_cast<const char*>(&address.address), sizeof(address.address));
            
            uint32_t typeLength = static_cast<uint32_t>(address.type.length());
            file.write(reinterpret_cast<const char*>(&typeLength), sizeof(typeLength));
            file.write(address.type.c_str(), typeLength);
            
            // Write values
            uint32_t valueCount = static_cast<uint32_t>(address.values.size());
            file.write(reinterpret_cast<const char*>(&valueCount), sizeof(valueCount));
            
            if (!address.values.empty()) {
                file.write(reinterpret_cast<const char*>(address.values.data()), 
                          valueCount * sizeof(double));
            }
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        file.close();
        return false;
    }
}

bool DataExporter::loadFromBinary(std::vector<MemoryAddress>& addresses, const std::string& filename) {
    if (!initialized) {
        return false;
    }
    
    std::string fullPath = defaultDirectory + "/" + sanitizeFilename(filename);
    if (!fullPath.ends_with(".bin")) {
        fullPath += ".bin";
    }
    
    std::ifstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    try {
        addresses.clear();
        
        // Read header
        uint32_t version;
        uint32_t addressCount;
        
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        file.read(reinterpret_cast<char*>(&addressCount), sizeof(addressCount));
        
        if (version != 1) {
            file.close();
            return false; // Unsupported version
        }
        
        // Read each address
        for (uint32_t i = 0; i < addressCount; ++i) {
            MemoryAddress address("", 0, "");
            
            // Read name
            uint32_t nameLength;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            
            std::string name(nameLength, '\0');
            file.read(&name[0], nameLength);
            address.name = name;
            
            // Read address
            file.read(reinterpret_cast<char*>(&address.address), sizeof(address.address));
            
            // Read type
            uint32_t typeLength;
            file.read(reinterpret_cast<char*>(&typeLength), sizeof(typeLength));
            
            std::string type(typeLength, '\0');
            file.read(&type[0], typeLength);
            address.type = type;
            
            // Read values
            uint32_t valueCount;
            file.read(reinterpret_cast<char*>(&valueCount), sizeof(valueCount));
            
            if (valueCount > 0) {
                address.values.resize(valueCount);
                file.read(reinterpret_cast<char*>(address.values.data()), 
                         valueCount * sizeof(double));
            }
            
            addresses.push_back(address);
        }
        
        file.close();
        return true;
    } catch (const std::exception& e) {
        file.close();
        return false;
    }
}

void DataExporter::setDefaultDirectory(const std::string& directory) {
    defaultDirectory = directory;
    createDirectoryIfNotExists(defaultDirectory);
}

std::vector<std::string> DataExporter::getExportFormats() {
    return {"CSV", "JSON", "Binary"};
}

std::string DataExporter::createTimestampedFilename(const std::string& prefix, const std::string& extension) {
    std::string timestamp = formatTimestamp();
    return prefix + "_" + timestamp + "." + extension;
}

std::string DataExporter::formatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

bool DataExporter::createDirectoryIfNotExists(const std::string& path) {
    try {
        if (!std::filesystem::exists(path)) {
            return std::filesystem::create_directories(path);
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

std::string DataExporter::sanitizeFilename(const std::string& filename) {
    std::string sanitized = filename;
    
    // Replace invalid characters
    const std::string invalidChars = "<>:\"/\\|?*";
    for (char c : invalidChars) {
        std::replace(sanitized.begin(), sanitized.end(), c, '_');
    }
    
    // Remove leading/trailing spaces and dots
    sanitized.erase(0, sanitized.find_first_not_of(" ."));
    sanitized.erase(sanitized.find_last_not_of(" .") + 1);
    
    return sanitized;
}

void DataExporter::writeCSVHeader(std::ofstream& file, const std::vector<MemoryAddress>& addresses) {
    file << "Timestamp";
    
    for (const auto& address : addresses) {
        file << "," << address.name << "_Value";
        file << "," << address.name << "_Change";
    }
    
    file << "\n";
}

void DataExporter::writeCSVData(std::ofstream& file, const std::vector<MemoryAddress>& addresses) {
    if (addresses.empty()) {
        return;
    }
    
    // Find the maximum number of data points
    size_t maxPoints = 0;
    for (const auto& address : addresses) {
        maxPoints = std::max(maxPoints, address.values.size());
    }
    
    // Write data row by row
    for (size_t i = 0; i < maxPoints; ++i) {
        // Timestamp (simplified - in real implementation, you'd store actual timestamps)
        file << i;
        
        for (const auto& address : addresses) {
            file << ",";
            
            if (i < address.values.size()) {
                file << std::fixed << std::setprecision(6) << address.values[i];
            } else {
                file << "";
            }
            
            file << ",";
            
            if (i < address.values.size() && i > 0) {
                file << std::fixed << std::setprecision(6) 
                     << (address.values[i] - address.values[i-1]);
            } else {
                file << "";
            }
        }
        
        file << "\n";
    }
}

void DataExporter::writeJSONHeader(std::ofstream& file) {
    file << "{\n";
    file << "  \"export_info\": {\n";
    file << "    \"timestamp\": \"" << formatTimestamp() << "\",\n";
    file << "    \"version\": \"1.0\",\n";
    file << "    \"format\": \"Game Analyzer Export\"\n";
    file << "  },\n";
    file << "  \"memory_addresses\": [\n";
}

void DataExporter::writeJSONAddresses(std::ofstream& file, const std::vector<MemoryAddress>& addresses) {
    for (size_t i = 0; i < addresses.size(); ++i) {
        const auto& address = addresses[i];
        
        file << "    {\n";
        file << "      \"name\": \"" << address.name << "\",\n";
        file << "      \"address\": \"0x" << std::hex << std::uppercase << address.address << "\",\n";
        file << "      \"type\": \"" << address.type << "\",\n";
        file << "      \"last_value\": " << std::fixed << std::setprecision(6) << address.lastValue << ",\n";
        file << "      \"change\": " << std::fixed << std::setprecision(6) << address.change << ",\n";
        file << "      \"values\": [";
        
        for (size_t j = 0; j < address.values.size(); ++j) {
            file << std::fixed << std::setprecision(6) << address.values[j];
            if (j < address.values.size() - 1) {
                file << ", ";
            }
        }
        
        file << "]\n";
        file << "    }";
        
        if (i < addresses.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
}

void DataExporter::writeJSONFooter(std::ofstream& file) {
    file << "  ]\n";
    file << "}\n";
}
