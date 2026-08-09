//
// Created by semleks on 09.08.2026.
//

#ifndef FUNPAYSEMLEX_CHATPOLLINGSERVICE_H
#define FUNPAYSEMLEX_CHATPOLLINGSERVICE_H
#include <cstdint>
#include <string>
#include <unordered_map>

#include "FunPayAPI/FunPayParser.h"
#include "FunPayAPI/FunPayRequest.h"


class ChatPollingService
{
    FunPayRequest& request;
    FunPayParser& parser;

    int64_t accountId;
    const std::string& csrfToken;
    std::string lastEventTag = "00000000";

    std::unordered_map<int64_t, int64_t> knownLastMsgIds;

    bool isFirstRun = true;

public:
    ChatPollingService(FunPayRequest& req, FunPayParser& pars,
                       int64_t accountId, const std::string& csrfToken)
        : request(req), parser(pars), accountId(accountId), csrfToken(csrfToken) {}

    void PollOne();
};


#endif //FUNPAYSEMLEX_CHATPOLLINGSERVICE_H
