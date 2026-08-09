//
// Created by semleks on 09.08.2026.
//

#ifndef FUNPAYSEMLEX_FUNPAYPARSER_H
#define FUNPAYSEMLEX_FUNPAYPARSER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "FunPayAPI/Messages/Types.h"

class FunPayParser
{
public:
    std::string parseNameFromHomePage(const std::string& html);

    int parseBalanceFromHomePage(const std::string& html);

    std::string parseCsrfToken(const std::string& html);

    int64_t parseUserId(const std::string& html);

    std::vector<ChatPreview> parseChatPreviews(
        const std::string& runnerResponse,
        std::string& eventTag);

    std::vector<FunPayMessage> parseMessages(
        const std::string& runnerResponse,
        const std::unordered_map<int64_t, std::string>& chatNames);
};


#endif //FUNPAYSEMLEX_FUNPAYPARSER_H
