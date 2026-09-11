#include <filesystem>
#include <fstream>
#include <iostream>

#include "Config.h"
#include "SystemUtils.h"
#include "FunPayAPI/FunPayRequest.h"

#include "FunPayAPI/FunPayAccount.h"
#include "TelegramBot/Telegram.h"

void printLogo()
{
    std::cout << "\033[1;36m";
    std::cout << R"(
███████╗██╗   ██╗███╗   ██╗██████╗ █████╗ ██╗   ██╗
██╔════╝██║   ██║████╗  ██║██╔══██╗██╔══██╗╚██╗ ██╔╝
█████╗  ██║   ██║██╔██╗ ██║██████╔╝███████║ ╚████╔╝
██╔══╝  ██║   ██║██║╚██╗██║██╔═══╝ ██╔══██║  ╚██╔╝
██║     ╚██████╔╝██║ ╚████║██║     ██║  ██║   ██║
╚═╝      ╚═════╝ ╚═╝  ╚═══╝╚═╝     ╚═╝  ╚═╝   ╚═╝
███████╗███████╗███╗   ███╗██╗     ███████╗██╗  ██╗
██╔════╝██╔════╝████╗ ████║██║     ██╔════╝╚██╗██╔╝
███████╗█████╗  ██╔████╔██║██║     █████╗   ╚███╔╝
╚════██║██╔══╝  ██║╚██╔╝██║██║     ██╔══╝   ██╔██╗
███████║███████╗██║ ╚═╝ ██║███████╗███████╗██╔╝ ██╗
╚══════╝╚══════╝╚═╝     ╚═╝╚══════╝╚══════╝╚═╝  ╚═╝
)";
    std::cout << "\033[0m" << std::endl;
}


int main()
{
    std::cout << "FunPayCardinal медленный из-за Python. Поэтому я создал этот движок, чтобы вы могли запускать СОТНИ акков на FunPay, а также писать плагины как на Python, так и на C++." << std::endl;
    std::cout << std::endl;

    printLogo();

    // Проверка конфигов.
    const std::filesystem::path path = SystemUtils::getExecutableDirectory();

    ConfigManager configManager{path / "configs" / "config.json"};

    if (!configManager.exists())
    {
        const std::optional<Config> cfg = configManager.setupInteractive();
        if (!cfg) {
            std::cerr << "\n\nЧто-то пошло не так =(\nПопробуй еще раз!";
            return 1;
        }

        configManager.save(*cfg);

        std::cout << "Перезапусти приложение. Я всё сохранил! (._.)";
        return 0;
    }

    Config config;
    if (!configManager.load(config))
    {
        std::cerr << "Не удалось загрузить конфиг. Попробуй удалить его и перезапуститься с нуля.";
        return 0;
    }

    Telegram::Start(
        config.token,
        config.password,
        config.telegramProxy,
        path / "configs" / "telegram.json");

    FunPayAccount funpayAccount{config.userAgent, config.goldenKey};
    std::cout << "-----------------\n";
    std::cout << "Привет, " << funpayAccount.getName() << "!" << std::endl;
    std::cout << "Твой баланс: " << funpayAccount.getBalance() << " рублей. \nХороших продаж!" << std::endl;
    std::cout << "-----------------\n";
    funpayAccount.startAutoRaise();
    funpayAccount.runMessagePolling();
    return 0;
}
