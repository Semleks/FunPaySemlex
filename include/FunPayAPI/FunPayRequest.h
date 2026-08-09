//
// Created by semleks on 09.08.2026.
//

#ifndef FUNPAYSEMLEX_FUNPAYREQUEST_H
#define FUNPAYSEMLEX_FUNPAYREQUEST_H

#include <cpr/session.h>
#include <cstdint>
#include <string>
#include <vector>

class FunPayRequest
{
    cpr::Session session;
    std::string userAgent;
public:
    FunPayRequest(std::string userAgent, std::string goldenKey);

    std::string getMainPage();

    std::string getUserPage(int64_t userId);

    std::string postRunner(const std::string& csrfToken, const std::string& objectsJson);

    std::string raiseLots(int64_t categoryId, const std::vector<int64_t>& subcategoryIds);
};


#endif //FUNPAYSEMLEX_FUNPAYREQUEST_H
