#pragma once

#include <cstdint>
#include <filesystem>
#include <stop_token>
#include <string>

class Telegram final
{
public:
    static bool Start(const std::string& token,
                      const std::string& password,
                      const std::string& proxy,
                      const std::filesystem::path& statePath);
    static void Stop();

    static bool Send(const std::string& text);
    static bool SendNotification(int64_t funPayChatId,
                                 const std::string& nickname,
                                 const std::string& message);

    static bool IsRunning();
    static bool IsAuthorized();

private:
    struct State;

    static State& state();
    static void receiveLoop(std::stop_token stopToken);
    static void sendLoop(std::stop_token stopToken);
};
