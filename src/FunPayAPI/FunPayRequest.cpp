//
// Created by semleks on 09.08.2026.
//

#include "../../include/FunPayAPI/FunPayRequest.h"

FunPayRequest::FunPayRequest(std::string userAgent, std::string goldenKey)
{
    session.SetUrl(cpr::Url{"https://funpay.com/"});

    session.SetHeader(cpr::Header{
    {"User-Agent", std::move(userAgent)},
    });

    session.SetCookies(cpr::Cookies{
          {"golden_key", std::move(goldenKey)},
          {"cookie_prefs", "1"}
      });

    session.SetTimeout(cpr::Timeout{10'000});

}

std::string FunPayRequest::getMainPage()
{
    const cpr::Response response = session.Get();

    if (response.error)
    {
        throw std::runtime_error(
            "Ошибка сети: " + response.error.message
        );
    }

    if (response.status_code == 403)
    {
        throw std::runtime_error(
            "FunPay отклонил golden_key или User-Agent"
        );
    }

    if (response.status_code != 200)
    {
        throw std::runtime_error(
            "FunPay вернул HTTP-код: " +
            std::to_string(response.status_code)
        );
    }

    return response.text;
}