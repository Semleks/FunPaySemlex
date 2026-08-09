//
// Created by semleks on 09.08.2026.
//

#include "../../include/FunPayAPI/FunPayRequest.h"

#include <stdexcept>

namespace
{
void validateResponse(const cpr::Response& response)
{
    if (response.error)
    {
        throw std::runtime_error("Ошибка сети: " + response.error.message);
    }

    if (response.status_code == 403)
    {
        throw std::runtime_error("FunPay отклонил golden_key или User-Agent");
    }

    if (response.status_code != 200)
    {
        throw std::runtime_error(
            "FunPay вернул HTTP-код: " + std::to_string(response.status_code));
    }
}
}

FunPayRequest::FunPayRequest(std::string userAgent, std::string goldenKey)
    : userAgent(std::move(userAgent))
{
    session.SetUrl(cpr::Url{"https://funpay.com/"});

    session.SetHeader(cpr::Header{
    {"User-Agent", this->userAgent},
    });

    session.SetCookies(cpr::Cookies{
          {"golden_key", std::move(goldenKey)},
          {"cookie_prefs", "1"}
      });

    session.SetTimeout(cpr::Timeout{10'000});

}

std::string FunPayRequest::getMainPage()
{
    session.SetUrl(cpr::Url{"https://funpay.com/"});
    session.SetHeader(cpr::Header{{"User-Agent", userAgent}});
    const cpr::Response response = session.Get();
    validateResponse(response);
    return response.text;
}

std::string FunPayRequest::postRunner(const std::string& csrfToken, const std::string& objectsJson)
{
    session.SetUrl(cpr::Url{"https://funpay.com/runner/"});
    session.SetHeader(cpr::Header{
        {"User-Agent", userAgent},
        {"Accept", "*/*"},
        {"Content-Type", "application/x-www-form-urlencoded; charset=UTF-8"},
        {"X-Requested-With", "XMLHttpRequest"}
    });
    session.SetPayload(cpr::Payload{
        {"csrf_token", csrfToken},
        {"objects", objectsJson},
        {"request", "false"}
    });

    const cpr::Response response = session.Post();
    validateResponse(response);
    return response.text;
}
