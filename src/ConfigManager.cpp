#include "ConfigManager.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem>

ConfigManager::ConfigManager() : initialized(false), configFile("config.json") {
}

ConfigManager::~ConfigManager() {
    shutdown();
}

bool ConfigManager::initialize() {
    initialized = true;
    setDefaultSettings();
    return loadConfiguration();
}

void ConfigManager::shutdown() {
    if (initialized) {
        saveConfiguration();
    }
    initialized = false;
}

bool ConfigManager::loadConfiguration() {
    if (!initialized) {
        return false;
    }
    
    return readFromFile();
}

bool ConfigManager::saveConfiguration() {
    if (!initialized) {
        return false;
    }
    
    return writeToFile();
}

void ConfigManager::setConfigFile(const std::string& filename) {
    configFile = filename;
}

void ConfigManager::setSetting(const std::string& key, const std::string& value) {
    settings[key] = value;
}

std::string ConfigManager::getSetting(const std::string& key, const std::string& defaultValue) {
    auto it = settings.find(key);
    if (it != settings.end()) {
        return it->second;
    }
    return defaultValue;
}

bool ConfigManager::hasSetting(const std::string& key) {
    return settings.find(key) != settings.end();
}

void ConfigManager::removeSetting(const std::string& key) {
    settings.erase(key);
}

void ConfigManager::setMemoryAddresses(const std::vector<MemoryAddress>& addresses) {
    savedAddresses = addresses;
}

std::vector<MemoryAddress> ConfigManager::getMemoryAddresses() {
    return savedAddresses;
}

void ConfigManager::addMemoryAddress(const MemoryAddress& address) {
    // Remove existing address with same name
    removeMemoryAddress(address.name);
    savedAddresses.push_back(address);
}

void ConfigManager::removeMemoryAddress(const std::string& name) {
    savedAddresses.erase(
        std::remove_if(savedAddresses.begin(), savedAddresses.end(),
                      [&name](const MemoryAddress& addr) { return addr.name == name; }),
        savedAddresses.end());
}

void ConfigManager::clearMemoryAddresses() {
    savedAddresses.clear();
}

void ConfigManager::setDefaultSettings() {
    settings.clear();
    
    // Default application settings
    settings["update_interval"] = "100";
    settings["max_data_points"] = "1000";
    settings["auto_save"] = "false";
    settings["data_directory"] = "data";
    settings["export_directory"] = "exports";
    settings["chart_height"] = "300";
    settings["show_grid"] = "true";
    settings["show_legend"] = "true";
    settings["auto_scale"] = "true";
    settings["line_thickness"] = "2.0";
    settings["window_width"] = "1200";
    settings["window_height"] = "800";
    settings["theme"] = "dark";
    settings["font_size"] = "13";
    settings["docking_enabled"] = "true";
    settings["viewports_enabled"] = "true";
}

void ConfigManager::resetToDefaults() {
    setDefaultSettings();
    clearMemoryAddresses();
}

bool ConfigManager::validateConfiguration() {
    // Validate numeric settings
    try {
        int updateInterval = std::stoi(getSetting("update_interval", "100"));
        if (updateInterval < 10 || updateInterval > 5000) {
            return false;
        }
        
        int maxDataPoints = std::stoi(getSetting("max_data_points", "1000"));
        if (maxDataPoints < 100 || maxDataPoints > 100000) {
            return false;
        }
        
        float lineThickness = std::stof(getSetting("line_thickness", "2.0"));
        if (lineThickness < 0.5f || lineThickness > 10.0f) {
            return false;
        }
        
    } catch (const std::exception&) {
        return false;
    }
    
    return true;
}

std::vector<std::string> ConfigManager::getConfigurationErrors() {
    std::vector<std::string> errors;
    
    try {
        int updateInterval = std::stoi(getSetting("update_interval", "100"));
        if (updateInterval < 10 || updateInterval > 5000) {
            errors.push_back("Update interval must be between 10 and 5000 milliseconds");
        }
    } catch (const std::exception&) {
        errors.push_back("Invalid update interval value");
    }
    
    try {
        int maxDataPoints = std::stoi(getSetting("max_data_points", "1000"));
        if (maxDataPoints < 100 || maxDataPoints > 100000) {
            errors.push_back("Max data points must be between 100 and 100000");
        }
    } catch (const std::exception&) {
        errors.push_back("Invalid max data points value");
    }
    
    try {
        float lineThickness = std::stof(getSetting("line_thickness", "2.0"));
        if (lineThickness < 0.5f || lineThickness > 10.0f) {
            errors.push_back("Line thickness must be between 0.5 and 10.0");
        }
    } catch (const std::exception&) {
        errors.push_back("Invalid line thickness value");
    }
    
    return errors;
}

bool ConfigManager::writeToFile() {
    try {
        std::ofstream file(configFile);
        if (!file.is_open()) {
            return false;
        }
        
        file << serializeToJSON();
        file.close();
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

bool ConfigManager::readFromFile() {
    try {
        if (!std::filesystem::exists(configFile)) {
            return true; // File doesn't exist, use defaults
        }
        
        std::ifstream file(configFile);
        if (!file.is_open()) {
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        
        return deserializeFromJSON(buffer.str());
        
    } catch (const std::exception&) {
        return false;
    }
}

std::string ConfigManager::serializeToJSON() {
    std::ostringstream json;
    
    json << "{\n";
    json << "  \"version\": \"1.0\",\n";
    json << "  \"settings\": {\n";
    
    bool first = true;
    for (const auto& setting : settings) {
        if (!first) {
            json << ",\n";
        }
        json << "    \"" << escapeString(setting.first) << "\": \"" 
             << escapeString(setting.second) << "\"";
        first = false;
    }
    
    json << "\n  },\n";
    json << "  \"memory_addresses\": [\n";
    
    first = true;
    for (const auto& address : savedAddresses) {
        if (!first) {
            json << ",\n";
        }
        json << "    " << serializeMemoryAddress(address);
        first = false;
    }
    
    json << "\n  ]\n";
    json << "}\n";
    
    return json.str();
}

bool ConfigManager::deserializeFromJSON(const std::string& json) {
    // Simple JSON parsing - in a real implementation, you'd use a proper JSON library
    // This is a basic implementation for demonstration
    
    try {
        // Reset to defaults first
        setDefaultSettings();
        clearMemoryAddresses();
        
        // Find settings section
        size_t settingsStart = json.find("\"settings\":");
        if (settingsStart != std::string::npos) {
            size_t settingsEnd = json.find("}", settingsStart);
            if (settingsEnd != std::string::npos) {
                std::string settingsSection = json.substr(settingsStart, settingsEnd - settingsStart);
                
                // Parse key-value pairs (simplified)
                size_t pos = 0;
                while ((pos = settingsSection.find("\"", pos)) != std::string::npos) {
                    size_t keyStart = pos + 1;
                    size_t keyEnd = settingsSection.find("\"", keyStart);
                    if (keyEnd == std::string::npos) break;
                    
                    std::string key = settingsSection.substr(keyStart, keyEnd - keyStart);
                    
                    size_t valueStart = settingsSection.find("\"", keyEnd + 1);
                    if (valueStart == std::string::npos) break;
                    valueStart++;
                    
                    size_t valueEnd = settingsSection.find("\"", valueStart);
                    if (valueEnd == std::string::npos) break;
                    
                    std::string value = settingsSection.substr(valueStart, valueEnd - valueStart);
                    
                    settings[key] = unescapeString(value);
                    pos = valueEnd + 1;
                }
            }
        }
        
        return true;
        
    } catch (const std::exception&) {
        return false;
    }
}

std::string ConfigManager::escapeString(const std::string& str) {
    std::string escaped = str;
    
    // Replace common escape characters
    size_t pos = 0;
    while ((pos = escaped.find("\\", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\\");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\n", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\n");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\r", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\r");
        pos += 2;
    }
    
    pos = 0;
    while ((pos = escaped.find("\t", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\t");
        pos += 2;
    }
    
    return escaped;
}

std::string ConfigManager::unescapeString(const std::string& str) {
    std::string unescaped = str;
    
    // Replace escaped characters
    size_t pos = 0;
    while ((pos = unescaped.find("\\t", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\t");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\r", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\r");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\n", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\n");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\\"", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\"");
        pos += 1;
    }
    
    pos = 0;
    while ((pos = unescaped.find("\\\\", pos)) != std::string::npos) {
        unescaped.replace(pos, 2, "\\");
        pos += 1;
    }
    
    return unescaped;
}

std::string ConfigManager::trimString(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
        return "";
    }
    
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::string ConfigManager::serializeMemoryAddress(const MemoryAddress& address) {
    std::ostringstream json;
    
    json << "{\n";
    json << "      \"name\": \"" << escapeString(address.name) << "\",\n";
    json << "      \"address\": \"" << std::hex << std::uppercase << address.address << "\",\n";
    json << "      \"type\": \"" << escapeString(address.type) << "\",\n";
    json << "      \"last_value\": " << std::fixed << std::setprecision(6) << address.lastValue << ",\n";
    json << "      \"change\": " << std::fixed << std::setprecision(6) << address.change << "\n";
    json << "    }";
    
    return json.str();
}

MemoryAddress ConfigManager::deserializeMemoryAddress(const std::string& json) {
    MemoryAddress address("", 0, "");
    
    // Simple parsing - in a real implementation, use a proper JSON library
    try {
        size_t nameStart = json.find("\"name\":");
        if (nameStart != std::string::npos) {
            size_t nameValueStart = json.find("\"", nameStart + 7);
            if (nameValueStart != std::string::npos) {
                nameValueStart++;
                size_t nameValueEnd = json.find("\"", nameValueStart);
                if (nameValueEnd != std::string::npos) {
                    address.name = unescapeString(json.substr(nameValueStart, nameValueEnd - nameValueStart));
                }
            }
        }
        
        size_t addressStart = json.find("\"address\":");
        if (addressStart != std::string::npos) {
            size_t addressValueStart = json.find("\"", addressStart + 10);
            if (addressValueStart != std::string::npos) {
                addressValueStart++;
                size_t addressValueEnd = json.find("\"", addressValueStart);
                if (addressValueEnd != std::string::npos) {
                    std::string addressStr = json.substr(addressValueStart, addressValueEnd - addressValueStart);
                    address.address = std::stoull(addressStr, nullptr, 16);
                }
            }
        }
        
        size_t typeStart = json.find("\"type\":");
        if (typeStart != std::string::npos) {
            size_t typeValueStart = json.find("\"", typeStart + 7);
            if (typeValueStart != std::string::npos) {
                typeValueStart++;
                size_t typeValueEnd = json.find("\"", typeValueStart);
                if (typeValueEnd != std::string::npos) {
                    address.type = unescapeString(json.substr(typeValueStart, typeValueEnd - typeValueStart));
                }
            }
        }
        
    } catch (const std::exception&) {
        // Return default address on error
    }
    
    return address;
}
