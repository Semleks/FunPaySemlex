//
// Created by semleks on 08.08.2026.
//

#include <utility>

#include "../../include/FunPayAPI/FunPayAccount.h"

FunPayAccount::FunPayAccount(std::string userAgent, std::string goldenKey) : userAgent(std::move(userAgent)), goldenKey(std::move(goldenKey)), request{userAgent, goldenKey}
{
    std::string html = request.getMainPage();
    name = parser.parseNameFromHomePage(html);
    balance = parser.parseBalanceFromHomePage(html);
}