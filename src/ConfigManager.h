#pragma once

#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <memory>
#include "GameAnalyzer.h"

class ConfigManager {
private:
    bool initialized;
    std::string configFile;
    std::map<std::string, std::string> settings;
    std::vector<MemoryAddress> savedAddresses;
    
public:
    ConfigManager();
    ~ConfigManager();
    
    bool initialize();
    void shutdown();
    
    // Configuration management
    bool loadConfiguration();
    bool saveConfiguration();
    void setConfigFile(const std::string& filename);
    
    // Settings management
    void setSetting(const std::string& key, const std::string& value);
    std::string getSetting(const std::string& key, const std::string& defaultValue = "");
    bool hasSetting(const std::string& key);
    void removeSetting(const std::string& key);
    
    // Memory addresses management
    void setMemoryAddresses(const std::vector<MemoryAddress>& addresses);
    std::vector<MemoryAddress> getMemoryAddresses();
    void addMemoryAddress(const MemoryAddress& address);
    void removeMemoryAddress(const std::string& name);
    void clearMemoryAddresses();
    
    // Default settings
    void setDefaultSettings();
    void resetToDefaults();
    
    // Configuration validation
    bool validateConfiguration();
    std::vector<std::string> getConfigurationErrors();
    
private:
    // File I/O
    bool writeToFile();
    bool readFromFile();
    
    // JSON parsing (simple implementation)
    std::string serializeToJSON();
    bool deserializeFromJSON(const std::string& json);
    
    // Helper functions
    std::string escapeString(const std::string& str);
    std::string unescapeString(const std::string& str);
    std::string trimString(const std::string& str);
    
    // Memory address serialization
    std::string serializeMemoryAddress(const MemoryAddress& address);
    MemoryAddress deserializeMemoryAddress(const std::string& json);
};
