#include "FunPayAPI/Lots/AutoRaiseService.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>

#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <nlohmann/json.hpp>

namespace
{
using XmlDoc = std::unique_ptr<_xmlDoc, decltype(&xmlFreeDoc)>;
using XPathContext = std::unique_ptr<xmlXPathContext, decltype(&xmlXPathFreeContext)>;
using XPathObject = std::unique_ptr<xmlXPathObject, decltype(&xmlXPathFreeObject)>;

XmlDoc parseHtml(const std::string& html)
{
    return XmlDoc{
        htmlReadMemory(
            html.c_str(), static_cast<int>(html.size()), nullptr, "UTF-8",
            HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_RECOVER),
        xmlFreeDoc};
}

XPathObject evaluate(xmlXPathContextPtr context, xmlNodePtr node, const char* expression)
{
    context->node = node;
    return XPathObject{
        xmlXPathEvalExpression(BAD_CAST expression, context),
        xmlXPathFreeObject};
}

std::string property(xmlNodePtr node, const char* name)
{
    xmlChar* rawValue = xmlGetProp(node, BAD_CAST name);
    if (!rawValue)
    {
        return {};
    }
    std::string value = reinterpret_cast<const char*>(rawValue);
    xmlFree(rawValue);
    return value;
}

std::string firstText(xmlXPathContextPtr context, xmlNodePtr node, const char* expression)
{
    XPathObject result = evaluate(context, node, expression);
    if (!result || !result->nodesetval || result->nodesetval->nodeNr == 0)
    {
        return {};
    }

    xmlChar* rawText = xmlNodeGetContent(result->nodesetval->nodeTab[0]);
    if (!rawText)
    {
        return {};
    }
    std::string text = reinterpret_cast<const char*>(rawText);
    xmlFree(rawText);
    return text;
}

int64_t subcategoryIdFromUrl(const std::string& url)
{
    constexpr std::string_view marker = "/lots/";
    const std::size_t begin = url.find(marker);
    if (begin == std::string::npos)
    {
        return 0;
    }

    const std::size_t idBegin = begin + marker.size();
    const std::size_t idEnd = url.find('/', idBegin);
    const std::string id = url.substr(idBegin, idEnd - idBegin);
    if (id.empty() || !std::all_of(id.begin(), id.end(), [](const unsigned char character)
        { return std::isdigit(character); }))
    {
        return 0;
    }
    return std::stoll(id);
}

int64_t jsonWait(const nlohmann::json& response)
{
    if (!response.contains("wait") || response["wait"].is_null())
    {
        return 0;
    }
    if (response["wait"].is_number_integer() || response["wait"].is_number_unsigned())
    {
        return std::max<int64_t>(0, response["wait"].get<int64_t>());
    }
    if (response["wait"].is_string())
    {
        const std::string value = response["wait"].get<std::string>();
        if (!value.empty() && std::all_of(value.begin(), value.end(), [](const unsigned char character)
            { return std::isdigit(character); }))
        {
            return std::stoll(value);
        }
    }
    return 0;
}

bool jsonTruthy(const nlohmann::json& value)
{
    if (value.is_null()) return false;
    if (value.is_boolean()) return value.get<bool>();
    if (value.is_number()) return value != 0;
    if (value.is_string()) return !value.get<std::string>().empty();
    return !value.empty();
}
}

AutoRaiseService::AutoRaiseService(std::string userAgent, std::string goldenKey)
    : request(std::move(userAgent), std::move(goldenKey))
{
}

void AutoRaiseService::Run(const std::stop_token stopToken)
{
    std::cout << "[AutoRaise] Фоновый сервис запущен.\n";

    while (!stopToken.stop_requested())
    {
        if (categories.empty())
        {
            try
            {
                loadCategories();
                if (categories.empty())
                {
                    std::cout << "[AutoRaise] Активные лоты не найдены. Повторная проверка через 5 минут.\n";
                    if (!wait(stopToken, 300)) return;
                    continue;
                }

                std::cout << "[AutoRaise] Найдено категорий с активными лотами: "
                          << categories.size() << ".\n";
            }
            catch (const std::exception& exception)
            {
                std::cerr << "[AutoRaise] Не удалось получить категории: "
                          << exception.what() << ". Повтор через 60 секунд.\n";
                if (!wait(stopToken, 60)) return;
                continue;
            }
        }

        const int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        int64_t nearestTime = std::numeric_limits<int64_t>::max();
        bool processedAnything = false;

        for (const Category& category : categories)
        {
            if (stopToken.stop_requested()) return;

            const int64_t scheduledTime = nextRaiseTime.contains(category.id)
                ? nextRaiseTime.at(category.id)
                : 0;
            if (scheduledTime > now)
            {
                nearestTime = std::min(nearestTime, scheduledTime);
                continue;
            }

            processedAnything = true;
            int64_t waitSeconds = 60;
            try
            {
                const RaiseResult result = raiseCategory(category);
                waitSeconds = std::max<int64_t>(2, result.waitSeconds);

                if (result.success)
                {
                    std::cout << "[AutoRaise] Категория «" << category.name << "» поднята.\n";
                }
                else
                {
                    std::cout << "[AutoRaise] Категория «" << category.name
                              << "» пока не поднята";
                    if (!result.message.empty())
                    {
                        std::cout << ": " << result.message;
                    }
                    std::cout << ".\n";
                }
            }
            catch (const std::exception& exception)
            {
                std::cerr << "[AutoRaise] Ошибка категории «" << category.name
                          << "»: " << exception.what() << ".\n";
            }

            const int64_t nextTime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count() + waitSeconds;
            nextRaiseTime[category.id] = nextTime;
            nearestTime = std::min(nearestTime, nextTime);
            std::cout << "[AutoRaise] Следующая попытка «" << category.name << "» через "
                      << formatDuration(waitSeconds) << ".\n";

            if (!wait(stopToken, 1)) return;
        }

        if (!processedAnything && nearestTime != std::numeric_limits<int64_t>::max())
        {
            const int64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (!wait(stopToken, std::max<int64_t>(1, nearestTime - currentTime))) return;
        }
    }
}

void AutoRaiseService::loadCategories()
{
    const std::string mainPage = request.getMainPage();
    const int64_t userId = parser.parseUserId(mainPage);
    const std::string profilePage = request.getUserPage(userId);
    categories = parseCategories(mainPage, profilePage);
    nextRaiseTime.clear();
}

AutoRaiseService::RaiseResult AutoRaiseService::raiseCategory(const Category& category)
{
    RaiseResult firstResult = parseRaiseResult(request.raiseLots(category.id, category.subcategoryIds));
    if (!firstResult.success || firstResult.waitSeconds > 0)
    {
        if (!firstResult.waitSeconds)
        {
            firstResult.waitSeconds = 60;
        }
        return firstResult;
    }

    // После успешного поднятия повторный запрос возвращает точный серверный cooldown.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    RaiseResult cooldownResult;
    try
    {
        cooldownResult = parseRaiseResult(request.raiseLots(category.id, category.subcategoryIds));
    }
    catch (const std::exception& exception)
    {
        // Лоты уже подняты первым запросом. При неизвестном cooldown не опрашиваем FunPay слишком часто.
        return {true, 3600, "cooldown не получен: " + std::string{exception.what()}};
    }
    if (!cooldownResult.waitSeconds)
    {
        cooldownResult.waitSeconds = 60;
    }
    cooldownResult.success = true;
    return cooldownResult;
}

std::vector<AutoRaiseService::Category> AutoRaiseService::parseCategories(
    const std::string& mainPage,
    const std::string& profilePage)
{
    struct CategoryBySubcategory
    {
        int64_t categoryId = 0;
        std::string name;
    };

    XmlDoc mainDocument = parseHtml(mainPage);
    XmlDoc profileDocument = parseHtml(profilePage);
    if (!mainDocument || !profileDocument)
    {
        throw std::runtime_error("не удалось разобрать HTML категорий");
    }

    XPathContext mainContext{xmlXPathNewContext(mainDocument.get()), xmlXPathFreeContext};
    XPathContext profileContext{xmlXPathNewContext(profileDocument.get()), xmlXPathFreeContext};
    if (!mainContext || !profileContext)
    {
        throw std::runtime_error("не удалось создать XPath-контекст категорий");
    }

    std::unordered_map<int64_t, CategoryBySubcategory> categoryBySubcategory;
    std::unordered_map<int64_t, std::vector<int64_t>> allSubcategoriesByCategory;
    XPathObject categoryLists = evaluate(
        mainContext.get(), xmlDocGetRootElement(mainDocument.get()),
        "//ul[contains(concat(' ', normalize-space(@class), ' '), ' list-inline ')][@data-id]");

    if (categoryLists && categoryLists->nodesetval)
    {
        for (int index = 0; index < categoryLists->nodesetval->nodeNr; ++index)
        {
            xmlNodePtr list = categoryLists->nodesetval->nodeTab[index];
            const std::string rawCategoryId = property(list, "data-id");
            if (rawCategoryId.empty()) continue;

            const int64_t categoryId = std::stoll(rawCategoryId);
            std::string categoryName = firstText(
                mainContext.get(), list,
                "ancestor::div[contains(concat(' ', normalize-space(@class), ' '), ' promo-game-item ')][1]"
                "//div[contains(concat(' ', normalize-space(@class), ' '), ' game-title ')]//a");
            if (categoryName.empty())
            {
                categoryName = "Категория " + std::to_string(categoryId);
            }

            XPathObject links = evaluate(mainContext.get(), list, ".//a[contains(@href, '/lots/')]");
            if (!links || !links->nodesetval) continue;
            for (int linkIndex = 0; linkIndex < links->nodesetval->nodeNr; ++linkIndex)
            {
                const int64_t subcategoryId = subcategoryIdFromUrl(
                    property(links->nodesetval->nodeTab[linkIndex], "href"));
                if (subcategoryId)
                {
                    categoryBySubcategory[subcategoryId] = {categoryId, categoryName};
                    auto& allSubcategories = allSubcategoriesByCategory[categoryId];
                    if (std::find(allSubcategories.begin(), allSubcategories.end(), subcategoryId) ==
                        allSubcategories.end())
                    {
                        allSubcategories.push_back(subcategoryId);
                    }
                }
            }
        }
    }

    std::vector<Category> result;
    std::unordered_set<int64_t> addedCategories;
    std::unordered_set<int64_t> addedSubcategories;
    XPathObject activeLinks = evaluate(
        profileContext.get(), xmlDocGetRootElement(profileDocument.get()),
        "//div[contains(concat(' ', normalize-space(@class), ' '), ' offer-list-title-container ')]"
        "//a[contains(@href, '/lots/')]");

    if (activeLinks && activeLinks->nodesetval)
    {
        for (int index = 0; index < activeLinks->nodesetval->nodeNr; ++index)
        {
            const int64_t subcategoryId = subcategoryIdFromUrl(
                property(activeLinks->nodesetval->nodeTab[index], "href"));
            if (!subcategoryId || addedSubcategories.contains(subcategoryId)) continue;

            const auto mapping = categoryBySubcategory.find(subcategoryId);
            if (mapping == categoryBySubcategory.end()) continue;

            const int64_t categoryId = mapping->second.categoryId;
            if (!addedCategories.contains(categoryId))
            {
                result.push_back({
                    categoryId,
                    mapping->second.name,
                    allSubcategoriesByCategory.at(categoryId)
                });
                addedCategories.insert(categoryId);
            }
            addedSubcategories.insert(subcategoryId);
        }
    }

    return result;
}

AutoRaiseService::RaiseResult AutoRaiseService::parseRaiseResult(const std::string& responseText)
{
    const nlohmann::json response = nlohmann::json::parse(responseText);
    const bool hasError = response.contains("error") && jsonTruthy(response["error"]);
    const bool hasUrl = response.contains("url") && jsonTruthy(response["url"]);

    RaiseResult result;
    result.success = !hasError && !hasUrl;
    result.waitSeconds = jsonWait(response);
    if (response.contains("msg") && response["msg"].is_string())
    {
        result.message = response["msg"].get<std::string>();
    }
    else if (response.contains("url") && response["url"].is_string())
    {
        result.message = response["url"].get<std::string>();
    }

    if (!result.success && !result.waitSeconds)
    {
        result.waitSeconds = parseWaitTime(result.message);
    }
    return result;
}

int64_t AutoRaiseService::parseWaitTime(const std::string& message)
{
    std::string digits;
    for (const unsigned char character : message)
    {
        if (std::isdigit(character)) digits += static_cast<char>(character);
    }
    const int64_t amount = digits.empty() ? 1 : std::stoll(digits);

    if (message.find("секунд") != std::string::npos || message.find("second") != std::string::npos)
        return amount;
    if (message.find("минут") != std::string::npos || message.find("хвилин") != std::string::npos ||
        message.find("minute") != std::string::npos)
        return amount * 60;
    if (message.find("час") != std::string::npos || message.find("годин") != std::string::npos ||
        message.find("hour") != std::string::npos)
        return amount * 3600;
    return 60;
}

std::string AutoRaiseService::formatDuration(int64_t seconds)
{
    const int64_t hours = seconds / 3600;
    const int64_t minutes = (seconds % 3600) / 60;
    const int64_t remainingSeconds = seconds % 60;

    std::string result;
    if (hours) result += std::to_string(hours) + " ч ";
    if (minutes) result += std::to_string(minutes) + " мин ";
    if (remainingSeconds || result.empty()) result += std::to_string(remainingSeconds) + " сек";
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

bool AutoRaiseService::wait(const std::stop_token stopToken, int64_t seconds)
{
    while (seconds-- > 0)
    {
        if (stopToken.stop_requested()) return false;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return !stopToken.stop_requested();
}
