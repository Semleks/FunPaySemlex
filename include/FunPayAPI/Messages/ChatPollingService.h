//
// Created by semleks on 09.08.2026.
//

#ifndef FUNPAYSEMLEX_CHATPOLLINGSERVICE_H
#define FUNPAYSEMLEX_CHATPOLLINGSERVICE_H
#include "FunPayAPI/FunPayParser.h"
#include "FunPayAPI/FunPayRequest.h"


class ChatPollingService
{
    FunPayRequest& request;
    FunPayParser& parser;

    std::unordered_map<int64_t, int64_t> knownLastMsgIds;

    bool isFirstRun = true;

public:
    ChatPollingService(FunPayRequest& req, FunPayParser& pars)
        : request(req), parser(pars) {}

    void PollOne()
    {

    }
};


#endif //FUNPAYSEMLEX_CHATPOLLINGSERVICE_H
