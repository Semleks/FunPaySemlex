#include <filesystem>
#include <fstream>
#include <iostream>

#include "Config.h"
#include "SystemUtils.h"

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
        const Config cfg = configManager.setupInteractive();
        configManager.save(cfg);

        std::cout << "Перезапусти приложение. Я всё сохранил!";
        return 0;
    }

    Config config;
    configManager.load(config);
    std::cout << config.token;
    return 0;
}
