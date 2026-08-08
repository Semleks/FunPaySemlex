//
// Created by semleks on 08.08.2026.
//

#ifndef FUNPAYSEMLEX_CONFIG_H
#define FUNPAYSEMLEX_CONFIG_H

#include <nlohmann/json.hpp>
#include <string>

struct Config
{
    // Telegram (будущее)
    std::string token;
    std::string password;

    // FunPay

};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Config, token, password);

class ConfigManager
{
    std::filesystem::path m_configPath;
public:
    explicit ConfigManager(std::filesystem::path configPath);

    bool exists() const;
    bool load(Config& outConfig);
    bool save(const Config& config);
    Config setupInteractive();
};


#endif //FUNPAYSEMLEX_CONFIG_H
