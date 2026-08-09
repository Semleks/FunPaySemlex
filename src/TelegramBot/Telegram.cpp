#include "TelegramBot/Telegram.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <thread>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

namespace
{
struct PendingMessage
{
    int64_t chatId = 0;
    std::string html;
};

std::string escapeHtml(const std::string& text)
{
    std::string result;
    result.reserve(text.size());
    for (const char character : text)
    {
        switch (character)
        {
        case '&': result += "&amp;"; break;
        case '<': result += "&lt;"; break;
        case '>': result += "&gt;"; break;
        default: result += character; break;
        }
    }
    return result;
}

std::string apiUrl(const std::string& token, const std::string& method)
{
    return "https://api.telegram.org/bot" + token + "/" + method;
}

cpr::Response telegramGet(const std::string& url,
                          const cpr::Parameters& parameters,
                          const std::string& proxy,
                          int timeoutMilliseconds)
{
    cpr::Session session;
    session.SetUrl(cpr::Url{url});
    session.SetParameters(parameters);
    session.SetTimeout(cpr::Timeout{timeoutMilliseconds});
    if (!proxy.empty())
    {
        session.SetProxies(cpr::Proxies{{"https", proxy}});
    }
    return session.Get();
}

cpr::Response telegramPost(const std::string& url,
                           const cpr::Payload& payload,
                           const std::string& proxy,
                           int timeoutMilliseconds)
{
    cpr::Session session;
    session.SetUrl(cpr::Url{url});
    session.SetPayload(payload);
    session.SetTimeout(cpr::Timeout{timeoutMilliseconds});
    if (!proxy.empty())
    {
        session.SetProxies(cpr::Proxies{{"https", proxy}});
    }
    return session.Post();
}

bool telegramResponseOk(const cpr::Response& response, nlohmann::json* body = nullptr)
{
    if (response.error || response.status_code != 200)
    {
        return false;
    }

    try
    {
        nlohmann::json parsed = nlohmann::json::parse(response.text);
        const bool ok = parsed.value("ok", false);
        if (body)
        {
            *body = std::move(parsed);
        }
        return ok;
    }
    catch (...)
    {
        return false;
    }
}

bool sendHtml(const std::string& token,
              const std::string& proxy,
              int64_t chatId,
              const std::string& html)
{
    const cpr::Response response = telegramPost(
        apiUrl(token, "sendMessage"),
        cpr::Payload{
            {"chat_id", std::to_string(chatId)},
            {"text", html},
            {"parse_mode", "HTML"}
        },
        proxy,
        10'000);

    if (telegramResponseOk(response))
    {
        return true;
    }

    std::cerr << "[Telegram] Не удалось отправить сообщение";
    if (response.status_code)
    {
        std::cerr << " (HTTP " << response.status_code << ')';
    }
    std::cerr << ".\n";
    return false;
}

bool isStartCommand(const std::string& text)
{
    return text == "/start" || text.starts_with("/start@");
}
}

struct Telegram::State
{
    std::mutex mutex;
    std::condition_variable queueChanged;
    std::deque<PendingMessage> queue;

    std::string token;
    std::string password;
    std::string proxy;
    std::filesystem::path statePath;
    int64_t authorizedChatId = 0;
    int64_t nextUpdateId = 0;

    std::atomic<bool> running = false;
    std::jthread receiveThread;
    std::jthread senderThread;

    ~State()
    {
        running = false;
        queueChanged.notify_all();
        receiveThread.request_stop();
        senderThread.request_stop();
    }
};

Telegram::State& Telegram::state()
{
    static State instance;
    return instance;
}

bool Telegram::Start(const std::string& token,
                     const std::string& password,
                     const std::string& proxy,
                     const std::filesystem::path& statePath)
{
    State& bot = state();
    if (bot.running)
    {
        return true;
    }
    if (token.empty())
    {
        std::cerr << "[Telegram] Бот не запущен: токен пустой.\n";
        return false;
    }
    if (password.empty())
    {
        std::cerr << "[Telegram] Бот не запущен: пароль доступа пустой.\n";
        return false;
    }

    nlohmann::json getMeBody;
    const cpr::Response getMeResponse = telegramGet(
        apiUrl(token, "getMe"), {}, proxy, 10'000);
    if (!telegramResponseOk(getMeResponse, &getMeBody))
    {
        std::cerr << "[Telegram] Бот не запущен: токен неверный или Telegram недоступен.\n";
        return false;
    }

    bot.token = token;
    bot.password = password;
    bot.proxy = proxy;
    bot.statePath = statePath;

    if (std::ifstream stateFile{statePath}; stateFile.is_open())
    {
        try
        {
            nlohmann::json savedState;
            stateFile >> savedState;
            bot.authorizedChatId = savedState.value("authorizedChatId", int64_t{0});
        }
        catch (const std::exception& exception)
        {
            std::cerr << "[Telegram] Не удалось прочитать сохранённый ID: "
                      << exception.what() << "\n";
        }
    }

    const std::string username = getMeBody["result"].value("username", "без username");
    std::cout << "[Telegram] Бот @" << username << " успешно запущен.\n";
    std::cout << "[Telegram] Подключение: "
              << (proxy.empty() ? "напрямую" : "через прокси") << ".\n";
    if (bot.authorizedChatId)
    {
        std::cout << "[Telegram] ID владельца загружен: " << bot.authorizedChatId << ".\n";
    }
    else
    {
        std::cout << "[Telegram] Владелец ещё не назначен. Отправьте боту /start.\n";
    }

    bot.running = true;
    bot.receiveThread = std::jthread([](const std::stop_token stopToken)
    {
        receiveLoop(stopToken);
    });
    bot.senderThread = std::jthread([](const std::stop_token stopToken)
    {
        sendLoop(stopToken);
    });
    return true;
}

void Telegram::Stop()
{
    State& bot = state();
    if (!bot.running.exchange(false))
    {
        return;
    }
    bot.queueChanged.notify_all();
    bot.receiveThread.request_stop();
    bot.senderThread.request_stop();
}

bool Telegram::Send(const std::string& text)
{
    State& bot = state();
    std::lock_guard lock{bot.mutex};
    if (!bot.running || !bot.authorizedChatId)
    {
        return false;
    }

    bot.queue.push_back({bot.authorizedChatId, escapeHtml(text)});
    bot.queueChanged.notify_one();
    return true;
}

bool Telegram::SendNotification(int64_t funPayChatId,
                                const std::string& nickname,
                                const std::string& message)
{
    State& bot = state();
    std::lock_guard lock{bot.mutex};
    if (!bot.running || !bot.authorizedChatId)
    {
        return false;
    }

    std::string safeMessage = message;
    constexpr std::size_t maxMessageLength = 3000;
    if (safeMessage.size() > maxMessageLength)
    {
        safeMessage.resize(maxMessageLength);
        safeMessage += "…";
    }

    std::string html =
        "🔔 <b>Новое сообщение FunPay</b>\n"
        "━━━━━━━━━━━━━━\n"
        "💬 <b>Чат:</b> <code>" + std::to_string(funPayChatId) + "</code>\n"
        "👤 <b>Ник:</b> " + escapeHtml(nickname.empty() ? "Неизвестно" : nickname) + "\n\n"
        "📝 <b>Сообщение:</b>\n<blockquote>" + escapeHtml(safeMessage) + "</blockquote>";

    bot.queue.push_back({bot.authorizedChatId, std::move(html)});
    bot.queueChanged.notify_one();
    return true;
}

bool Telegram::IsRunning()
{
    return state().running;
}

bool Telegram::IsAuthorized()
{
    State& bot = state();
    std::lock_guard lock{bot.mutex};
    return bot.authorizedChatId != 0;
}

void Telegram::receiveLoop(const std::stop_token stopToken)
{
    State& bot = state();

    while (bot.running && !stopToken.stop_requested())
    {
        std::string token;
        std::string proxy;
        int64_t offset;
        {
            std::lock_guard lock{bot.mutex};
            token = bot.token;
            proxy = bot.proxy;
            offset = bot.nextUpdateId;
        }

        const cpr::Response response = telegramGet(
            apiUrl(token, "getUpdates"),
            cpr::Parameters{
                {"offset", std::to_string(offset)},
                {"timeout", "20"},
                {"allowed_updates", "[\"message\"]"}
            },
            proxy,
            25'000);

        nlohmann::json body;
        if (!telegramResponseOk(response, &body))
        {
            if (bot.running)
            {
                std::cerr << "[Telegram] Ошибка получения команд. Повтор через 3 секунды.\n";
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
            continue;
        }

        for (const nlohmann::json& update : body.value("result", nlohmann::json::array()))
        {
            const int64_t updateId = update.value("update_id", int64_t{0});
            {
                std::lock_guard lock{bot.mutex};
                bot.nextUpdateId = std::max(bot.nextUpdateId, updateId + 1);
            }

            if (!update.contains("message") || !update["message"].is_object())
            {
                continue;
            }
            const nlohmann::json& telegramMessage = update["message"];
            if (!telegramMessage.contains("chat") ||
                telegramMessage["chat"].value("type", "") != "private")
            {
                continue;
            }

            const int64_t chatId = telegramMessage["chat"].value("id", int64_t{0});
            const std::string text = telegramMessage.value("text", "");
            int64_t ownerId;
            std::string password;
            std::filesystem::path statePath;
            {
                std::lock_guard lock{bot.mutex};
                ownerId = bot.authorizedChatId;
                password = bot.password;
                statePath = bot.statePath;
            }

            if (!ownerId)
            {
                if (text == password)
                {
                    {
                        std::lock_guard lock{bot.mutex};
                        bot.authorizedChatId = chatId;
                    }

                    try
                    {
                        std::filesystem::create_directories(statePath.parent_path());
                        std::ofstream stateFile{statePath};
                        if (!stateFile.is_open())
                        {
                            throw std::runtime_error("файл telegram.json не открылся");
                        }
                        stateFile << nlohmann::json{{"authorizedChatId", chatId}}.dump(4);
                        stateFile.flush();
                        if (!stateFile.good())
                        {
                            throw std::runtime_error("ошибка записи telegram.json");
                        }
                        std::cout << "[Telegram] Владелец подтверждён. ID сохранён: " << chatId << ".\n";
                        sendHtml(token, proxy, chatId,
                            "✅ <b>Доступ подтверждён</b>\n\n"
                            "Теперь сюда будут приходить уведомления FunPay.");
                    }
                    catch (const std::exception& exception)
                    {
                        std::cerr << "[Telegram] ID подтверждён, но не сохранён: "
                                  << exception.what() << "\n";
                        sendHtml(token, proxy, chatId,
                            "⚠️ Доступ подтверждён, но ID не удалось сохранить.");
                    }
                }
                else
                {
                    sendHtml(token, proxy, chatId,
                        "🔐 <b>Авторизация FunPaySemlex</b>\n\n"
                        "Отправьте пароль доступа, указанный при настройке программы.");
                }
                continue;
            }

            if (chatId != ownerId)
            {
                sendHtml(token, proxy, chatId, "⛔ <b>Доступ запрещён.</b>");
                continue;
            }

            if (isStartCommand(text))
            {
                sendHtml(token, proxy, chatId,
                    "🟢 <b>FunPaySemlex работает</b>\n\n"
                    "✅ Telegram-бот подключён\n"
                    "✅ Владелец авторизован\n"
                    "✅ Уведомления FunPay включены");
            }
        }
    }
}

void Telegram::sendLoop(const std::stop_token stopToken)
{
    State& bot = state();

    while (bot.running && !stopToken.stop_requested())
    {
        PendingMessage message;
        std::string token;
        std::string proxy;
        {
            std::unique_lock lock{bot.mutex};
            bot.queueChanged.wait(lock, [&bot]
            {
                return !bot.queue.empty() || !bot.running;
            });
            if (!bot.running)
            {
                break;
            }

            message = std::move(bot.queue.front());
            bot.queue.pop_front();
            token = bot.token;
            proxy = bot.proxy;
        }

        sendHtml(token, proxy, message.chatId, message.html);
    }
}
