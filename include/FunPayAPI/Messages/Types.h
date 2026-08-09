#pragma once
#include <string>
#include <cstdint>

struct ChatPreview {
    int64_t chatId = 0;
    int64_t lastMessageId = 0;
    std::string interlocutorName; // Ник
    bool unread = false;
};

// Полное сообщение
struct FunPayMessage {
    int64_t messageId = 0;
    int64_t chatId = 0;
    std::string authorName;       // Кто написал
    int64_t authorId = 0;
    std::string text;             // Текст сообщения
};