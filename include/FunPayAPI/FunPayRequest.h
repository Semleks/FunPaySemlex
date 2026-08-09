//
// Created by semleks on 09.08.2026.
//

#ifndef FUNPAYSEMLEX_FUNPAYREQUEST_H
#define FUNPAYSEMLEX_FUNPAYREQUEST_H

#include <cpr/session.h>
#include <string>

class FunPayRequest
{
    cpr::Session session;
    std::string userAgent;
public:
    FunPayRequest(std::string userAgent, std::string goldenKey);

    std::string getMainPage();

    std::string postRunner(const std::string& csrfToken, const std::string& objectsJson);
};


#endif //FUNPAYSEMLEX_FUNPAYREQUEST_H
