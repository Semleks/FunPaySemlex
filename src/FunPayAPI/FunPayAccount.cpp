//
// Created by semleks on 08.08.2026.
//

#include <utility>
#include <chrono>
#include <iostream>
#include <thread>

#include "../../include/FunPayAPI/FunPayAccount.h"
#include "../../include/FunPayAPI/Lots/AutoRaiseService.h"
#include "../../include/FunPayAPI/Messages/ChatPollingService.h"

FunPayAccount::FunPayAccount(std::string userAgent, std::string goldenKey) : userAgent(std::move(userAgent)), goldenKey(std::move(goldenKey)), request{userAgent, goldenKey}
{
    std::string html = request.getMainPage();
    name = parser.parseNameFromHomePage(html);
    balance = parser.parseBalanceFromHomePage(html);
    CsrfToken = parser.parseCsrfToken(html);
    id = parser.parseUserId(html);
}

void FunPayAccount::startAutoRaise()
{
    if (autoRaiseThread.joinable())
    {
        return;
    }

    autoRaiseThread = std::jthread(
        [userAgent = userAgent, goldenKey = goldenKey](const std::stop_token stopToken)
        {
            AutoRaiseService service{userAgent, goldenKey};
            service.Run(stopToken);
        });
}

void FunPayAccount::runMessagePolling()
{
    ChatPollingService pollingService{request, parser, id, CsrfToken};

    while (true)
    {
        try
        {
            pollingService.PollOne();
        }
        catch (const std::exception& exception)
        {
            std::cerr << "Ошибка polling сообщений: " << exception.what() << '\n';
        }

        std::this_thread::sleep_for(std::chrono::seconds(6));
    }
}
