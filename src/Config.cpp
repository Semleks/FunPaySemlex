//
// Created by semleks on 08.08.2026.
//

#include "../include/Config.h"

#include <filesystem>
#include <fstream>
#include <iostream>

ConfigManager::ConfigManager(std::filesystem::path configPath)
    : m_configPath(std::move(configPath)) {}

// Проверяем существует ли конфиг
bool ConfigManager::exists() const
{
    if (std::filesystem::exists(m_configPath))
        return true;

    return false;
}

bool ConfigManager::load(Config& outConfig)
{
    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        outConfig = j.get<Config>();
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка парсинга JSON: " << e.what() << std::endl;
        return false;
    }
}

Config ConfigManager::setupInteractive()
{
    std::cout << "Привет, Бро! Спасибо за скачивания моего проекта! Давай настроем всё для тебя!\n\nНапиши токен телеграмм бота (найти в @BotFather): ";
    std::string token;
    std::getline(std::cin, token);

    std::cout << std::endl;

    if (token.empty())
    {
        std::cout << "Токен пустой. Попробуй еще раз.";
    }

    Config config;
    config.token = token;

    std::cout << "Теперь введи пароль для доступа в тг бота: ";
    std::string pass;
    std::getline(std::cin, pass);

    std::cout << std::endl;

    if (pass.empty())
    {
        std::cout << "Пароль пустой. Попробуй еще раз.";
    }

    config.password = pass;

    std::cout << "Прокси для Telegram (например http://IP:PORT или socks5h://IP:PORT). "
                 "Нажми ENTER, чтобы подключаться напрямую: ";
    std::string telegramProxy;
    std::getline(std::cin, telegramProxy);
    config.telegramProxy = telegramProxy;

    std::cout << std::endl;

    std::cout << "Теперь введи golden_key (погугли): ";
    std::string goldenKey;
    std::getline(std::cin, goldenKey);

    std::cout << std::endl;

    if (goldenKey.empty())
    {
        std::cout << "Golden_Key пустой. Попробуй еще раз.";
    }

    config.goldenKey = goldenKey;

    std::cout << "Теперь введи UserAgent (погугли): ";
    std::string userAgent;
    std::getline(std::cin, userAgent);

    std::cout << std::endl;

    if (userAgent.empty())
    {
        std::cout << "userAgent пустой. Попробуй еще раз.";
    }

    config.userAgent = userAgent;
    return config;
}

bool ConfigManager::save(const Config& config)
{
    if (m_configPath.has_parent_path() && !std::filesystem::exists(m_configPath.parent_path())) {
        std::filesystem::create_directories(m_configPath.parent_path());
    }

    std::ofstream file(m_configPath);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json j = config;

    file << j.dump(4);
    return true;
}
