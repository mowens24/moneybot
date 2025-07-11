#pragma once

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <ctime>
#include <vector>
#include <nlohmann/json.hpp>

namespace moneybot {

class ConfigManager {
public:
    static ConfigManager& getInstance();
    
    // Load configuration from file and environment variables
    bool loadConfig(const std::string& config_path);
    bool saveConfig(const std::string& config_path = "") const;
    
    // Get configuration data
    const nlohmann::json& getConfig() const { return config_; }
    nlohmann::json& getConfig() { return config_; }
    
    // Get/Set specific configuration values
    template<typename T>
    T getValue(const std::string& key, const T& default_value = T{}) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return config_.value(key, default_value);
    }
    
    template<typename T>
    void setValue(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        config_[key] = value;
    }
    
    // Nested configuration access using dot notation (e.g., "exchange.binance.api_key")
    std::string getNestedValue(const std::string& nested_key, const std::string& default_value = "") const;
    void setNestedValue(const std::string& nested_key, const std::string& value);
    
    // Get exchange configuration with resolved API keys
    nlohmann::json getExchangeConfig(const std::string& exchange_name) const;
    
    // Check if API keys are properly configured
    bool validateApiKeys() const;
    
    // Get environment variable with fallback
    std::string getEnvVar(const std::string& var_name, const std::string& default_value = "") const;
    
    // Security checks
    bool isProductionMode() const;
    bool isDryRunMode() const;
    void setDryRunMode(bool dry_run);
    void setProductionMode(bool production);
    
    // Configuration validation
    bool validateConfiguration() const;
    std::vector<std::string> getValidationErrors() const;
    
    // Hot-reload configuration
    void enableHotReload(const std::string& config_path);
    void disableHotReload();
    bool isHotReloadEnabled() const;
    
private:
    ConfigManager() = default;
    ~ConfigManager();
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Load API keys from environment variables
    void loadApiKeysFromEnv();
    
    // Validate exchange configuration
    bool validateExchangeConfig(const nlohmann::json& exchange_config) const;
    
    // Hot-reload functionality
    void watchConfigFile();
    bool checkConfigFileChanged();
    
    mutable std::mutex mutex_;
    nlohmann::json config_;
    std::map<std::string, std::string> api_keys_;
    bool production_mode_ = false;
    bool dry_run_mode_ = true;
    
    // File watching for hot-reload
    std::string config_file_path_;
    std::time_t last_modified_time_ = 0;
    bool hot_reload_enabled_ = false;
    std::thread watch_thread_;
    std::atomic<bool> watch_running_{false};
};

} // namespace moneybot
