#include "../include/config_manager.h"
#include <fstream>
#include <cstdlib>
#include <algorithm>
#include <iostream>

namespace moneybot {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::loadConfig(const std::string& config_path) {
    try {
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            std::cerr << "ERROR: Failed to open config file: " << config_path << std::endl;
            return false;
        }

        config_file >> config_;
        
        // Load API keys from environment variables
        loadApiKeysFromEnv();
        
        // Check for production mode
        production_mode_ = getEnvVar("MONEYBOT_PRODUCTION", "false") == "true";
        dry_run_mode_ = getEnvVar("MONEYBOT_DRY_RUN", "true") == "true";
        
        // Override config dry_run if environment variable is set
        if (config_.contains("dry_run")) {
            config_["dry_run"] = dry_run_mode_;
        }
        
        std::cout << "INFO: Configuration loaded successfully" << std::endl;
        std::cout << "INFO: Production mode: " << (production_mode_ ? "true" : "false") << std::endl;
        std::cout << "INFO: Dry run mode: " << (dry_run_mode_ ? "true" : "false") << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: Failed to load config: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::loadApiKeysFromEnv() {
    // Binance keys
    std::string binance_key = getEnvVar("BINANCE_API_KEY");
    std::string binance_secret = getEnvVar("BINANCE_SECRET_KEY");
    if (!binance_key.empty() && !binance_secret.empty()) {
        api_keys_["binance_api_key"] = binance_key;
        api_keys_["binance_secret_key"] = binance_secret;
        std::cout << "INFO: Binance API keys loaded from environment" << std::endl;
    }
    
    // Coinbase keys
    std::string coinbase_key = getEnvVar("COINBASE_API_KEY");
    std::string coinbase_secret = getEnvVar("COINBASE_SECRET_KEY");
    std::string coinbase_passphrase = getEnvVar("COINBASE_PASSPHRASE");
    if (!coinbase_key.empty() && !coinbase_secret.empty() && !coinbase_passphrase.empty()) {
        api_keys_["coinbase_api_key"] = coinbase_key;
        api_keys_["coinbase_secret_key"] = coinbase_secret;
        api_keys_["coinbase_passphrase"] = coinbase_passphrase;
        std::cout << "INFO: Coinbase API keys loaded from environment" << std::endl;
    }
    
    // Kraken keys
    std::string kraken_key = getEnvVar("KRAKEN_API_KEY");
    std::string kraken_secret = getEnvVar("KRAKEN_SECRET_KEY");
    if (!kraken_key.empty() && !kraken_secret.empty()) {
        api_keys_["kraken_api_key"] = kraken_key;
        api_keys_["kraken_secret_key"] = kraken_secret;
        std::cout << "INFO: Kraken API keys loaded from environment" << std::endl;
    }
}

nlohmann::json ConfigManager::getExchangeConfig(const std::string& exchange_name) const {
    if (!config_.contains("multi_asset") || !config_["multi_asset"].contains("exchanges")) {
        return nlohmann::json{};
    }
    
    for (const auto& exchange : config_["multi_asset"]["exchanges"]) {
        if (exchange["name"] == exchange_name) {
            nlohmann::json exchange_config = exchange;
            
            // Replace placeholder keys with environment variables
            std::string key_prefix = exchange_name;
            std::transform(key_prefix.begin(), key_prefix.end(), key_prefix.begin(), ::toupper);
            
            auto api_key_it = api_keys_.find(exchange_name + "_api_key");
            auto secret_key_it = api_keys_.find(exchange_name + "_secret_key");
            
            if (api_key_it != api_keys_.end()) {
                exchange_config["api_key"] = api_key_it->second;
            }
            if (secret_key_it != api_keys_.end()) {
                exchange_config["secret_key"] = secret_key_it->second;
            }
            
            // Special handling for Coinbase passphrase
            if (exchange_name == "coinbase") {
                auto passphrase_it = api_keys_.find("coinbase_passphrase");
                if (passphrase_it != api_keys_.end()) {
                    exchange_config["passphrase"] = passphrase_it->second;
                }
            }
            
            return exchange_config;
        }
    }
    
    return nlohmann::json{};
}

bool ConfigManager::validateApiKeys() const {
    if (dry_run_mode_) {
        std::cout << "INFO: Dry run mode - API key validation skipped" << std::endl;
        return true;
    }
    
    bool all_valid = true;
    
    // Check enabled exchanges have valid API keys
    if (config_.contains("multi_asset") && config_["multi_asset"].contains("exchanges")) {
        for (const auto& exchange : config_["multi_asset"]["exchanges"]) {
            if (exchange["enabled"] == true) {
                std::string name = exchange["name"];
                
                auto api_key_it = api_keys_.find(name + "_api_key");
                auto secret_key_it = api_keys_.find(name + "_secret_key");
                
                if (api_key_it == api_keys_.end() || secret_key_it == api_keys_.end()) {
                    std::cerr << "ERROR: Missing API keys for enabled exchange: " << name << std::endl;
                    all_valid = false;
                } else if (api_key_it->second.empty() || secret_key_it->second.empty()) {
                    std::cerr << "ERROR: Empty API keys for enabled exchange: " << name << std::endl;
                    all_valid = false;
                } else {
                    std::cout << "INFO: API keys validated for exchange: " << name << std::endl;
                }
                
                // Special validation for Coinbase
                if (name == "coinbase") {
                    auto passphrase_it = api_keys_.find("coinbase_passphrase");
                    if (passphrase_it == api_keys_.end() || passphrase_it->second.empty()) {
                        std::cerr << "ERROR: Missing passphrase for Coinbase" << std::endl;
                        all_valid = false;
                    }
                }
            }
        }
    }
    
    return all_valid;
}

std::string ConfigManager::getEnvVar(const std::string& var_name, const std::string& default_value) const {
    const char* env_val = std::getenv(var_name.c_str());
    return env_val ? std::string(env_val) : default_value;
}

bool ConfigManager::isProductionMode() const {
    return production_mode_;
}

bool ConfigManager::isDryRunMode() const {
    return dry_run_mode_;
}

bool ConfigManager::validateExchangeConfig(const nlohmann::json& exchange_config) const {
    const std::vector<std::string> required_fields = {
        "name", "rest_url", "ws_url", "taker_fee", "maker_fee"
    };
    
    for (const auto& field : required_fields) {
        if (!exchange_config.contains(field)) {
            std::cerr << "ERROR: Missing required field in exchange config: " << field << std::endl;
            return false;
        }
    }
    
    return true;
}

} // namespace moneybot
