//
// Created by semleks on 09.08.2026.
//

#include "../../../include/FunPayAPI/Messages/ChatPollingService.h"

#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

#include "TelegramBot/Telegram.h"

void ChatPollingService::PollOne()
{
    nlohmann::json bookmarkObjects = nlohmann::json::array({{
        {"type", "chat_bookmarks"},
        {"id", accountId},
        {"tag", lastEventTag},
        {"data", false}
    }});

    const std::string bookmarksResponse = request.postRunner(csrfToken, bookmarkObjects.dump());
    const std::vector<ChatPreview> previews = parser.parseChatPreviews(bookmarksResponse, lastEventTag);

    if (isFirstRun)
    {
        for (const ChatPreview& preview : previews)
        {
            knownLastMsgIds[preview.chatId] = preview.lastMessageId;
        }
        isFirstRun = false;
        return;
    }

    std::vector<ChatPreview> changedChats;
    for (const ChatPreview& preview : previews)
    {
        const auto known = knownLastMsgIds.find(preview.chatId);
        if (known == knownLastMsgIds.end() || preview.lastMessageId > known->second)
        {
            changedChats.push_back(preview);
        }
    }

    constexpr std::size_t maxChatsPerRequest = 10;
    for (std::size_t offset = 0; offset < changedChats.size(); offset += maxChatsPerRequest)
    {
        const std::size_t end = std::min(offset + maxChatsPerRequest, changedChats.size());
        nlohmann::json chatObjects = nlohmann::json::array();
        std::unordered_map<int64_t, std::string> chatNames;

        for (std::size_t index = offset; index < end; ++index)
        {
            const ChatPreview& chat = changedChats[index];
            const auto known = knownLastMsgIds.find(chat.chatId);
            const int64_t lastMessageId = known == knownLastMsgIds.end() ? -1 : known->second;
            chatNames[chat.chatId] = chat.interlocutorName;

            chatObjects.push_back({
                {"type", "chat_node"},
                {"id", chat.chatId},
                {"tag", "00000000"},
                {"data", {
                    {"node", chat.chatId},
                    {"last_message", lastMessageId},
                    {"content", ""}
                }}
            });
        }

        const std::string messagesResponse = request.postRunner(csrfToken, chatObjects.dump());
        std::vector<FunPayMessage> messages = parser.parseMessages(messagesResponse, chatNames);
        std::sort(messages.begin(), messages.end(), [](const FunPayMessage& left, const FunPayMessage& right)
        {
            return left.messageId < right.messageId;
        });

        for (const FunPayMessage& message : messages)
        {
            const auto known = knownLastMsgIds.find(message.chatId);
            const int64_t lastKnownId = known == knownLastMsgIds.end() ? 0 : known->second;
            if (message.messageId <= lastKnownId || message.authorId == accountId || message.authorId == 0)
            {
                continue;
            }

            std::cout << "Новое сообщение. Чат: " << message.chatId
                      << ", Ник: " << (message.authorName.empty() ? "Неизвестно" : message.authorName)
                      << ", Сообщение: " << message.text << '\n';

            Telegram::SendNotification(
                message.chatId,
                message.authorName,
                message.text);
        }

        // Состояние обновляется только после успешного запроса и разбора истории.
        for (std::size_t index = offset; index < end; ++index)
        {
            const ChatPreview& chat = changedChats[index];
            knownLastMsgIds[chat.chatId] = chat.lastMessageId;
        }
    }
}
