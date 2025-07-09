#pragma once

#include <string>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace moneybot {

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // Load configuration from file and environment variables
    bool loadConfig(const std::string& config_path);
    
    // Get configuration data
    const nlohmann::json& getConfig() const { return config_; }
    
    // Get exchange configuration with resolved API keys
    nlohmann::json getExchangeConfig(const std::string& exchange_name) const;
    
    // Check if API keys are properly configured
    bool validateApiKeys() const;
    
    // Get environment variable with fallback
    std::string getEnvVar(const std::string& var_name, const std::string& default_value = "") const;
    
    // Security checks
    bool isProductionMode() const;
    bool isDryRunMode() const;
    
private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Load API keys from environment variables
    void loadApiKeysFromEnv();
    
    // Validate exchange configuration
    bool validateExchangeConfig(const nlohmann::json& exchange_config) const;
    
    nlohmann::json config_;
    std::map<std::string, std::string> api_keys_;
    bool production_mode_ = false;
    bool dry_run_mode_ = true;
};

} // namespace moneybot
