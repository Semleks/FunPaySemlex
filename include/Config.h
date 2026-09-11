//
// Created by semleks on 08.08.2026.
//

#ifndef FUNPAYSEMLEX_CONFIG_H
#define FUNPAYSEMLEX_CONFIG_H

#include <nlohmann/json.hpp>
#include <string>
#include <filesystem>

struct Config
{
    // Telegram
    std::string token;
    std::string password;
    std::string telegramProxy;

    // FunPay
    std::string goldenKey;
    std::string userAgent;
};

inline void to_json(nlohmann::json& json, const Config& config)
{
    json = nlohmann::json{
        {"token", config.token},
        {"password", config.password},
        {"telegramProxy", config.telegramProxy},
        {"goldenKey", config.goldenKey},
        {"userAgent", config.userAgent}
    };
}

inline void from_json(const nlohmann::json& json, Config& config)
{
    config.token = json.value("token", "");
    config.password = json.value("password", "");
    config.telegramProxy = json.value("telegramProxy", "");
    config.goldenKey = json.value("goldenKey", "");
    config.userAgent = json.value("userAgent", "");
}

class ConfigManager
{
    std::filesystem::path m_configPath;
public:
    explicit ConfigManager(std::filesystem::path configPath);

    bool exists() const;
    bool load(Config& outConfig);
    bool save(const Config& config);
    std::optional<Config> setupInteractive();
};


#endif //FUNPAYSEMLEX_CONFIG_H
