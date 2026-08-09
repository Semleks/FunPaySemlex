//
// Created by semleks on 09.08.2026.
//

#ifndef FUNPAYSEMLEX_FUNPAYPARSER_H
#define FUNPAYSEMLEX_FUNPAYPARSER_H

#include <string>

class FunPayParser
{
public:
    std::string parseNameFromHomePage(const std::string& html);

    int parseBalanceFromHomePage(const std::string& html);

    std::string parseCsrfToken(const std::string& html);
};


#endif //FUNPAYSEMLEX_FUNPAYPARSER_H
