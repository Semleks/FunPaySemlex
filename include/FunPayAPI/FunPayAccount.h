//
// Created by semleks on 08.08.2026.
//

#ifndef FUNPAYSEMLEX_FUNPAYACCOUNT_H
#define FUNPAYSEMLEX_FUNPAYACCOUNT_H

#include <string>
#include <thread>

#include "FunPayAPI/FunPayParser.h"
#include "FunPayRequest.h"

class FunPayAccount
{
    FunPayParser parser;
    FunPayRequest request;

    // Токен для последующих сообщений, important XD
    std::string CsrfToken;

    int balance = 0;
    int64_t id = 0;
    std::string name;

    std::string goldenKey;
    std::string userAgent;
    std::jthread autoRaiseThread;

public:
    std::string getName() const
    {
        return name;
    }

    int getBalance() const
    {
        return balance;
    }

    void runMessagePolling();
    void startAutoRaise();

    FunPayAccount(std::string userAgent, std::string goldenKey);
};


#endif //FUNPAYSEMLEX_FUNPAYACCOUNT_H
