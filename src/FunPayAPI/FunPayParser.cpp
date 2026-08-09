//
// Created by semleks on 09.08.2026.
//

#include "../../include/FunPayAPI/FunPayParser.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <nlohmann/json.hpp>

namespace
{
using Json = nlohmann::json;

std::string trim(std::string value)
{
    const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

int64_t jsonInteger(const Json& value)
{
    if (value.is_number_integer() || value.is_number_unsigned())
    {
        return value.get<int64_t>();
    }
    if (value.is_string())
    {
        return std::stoll(value.get<std::string>());
    }
    throw std::runtime_error("FunPay вернул ID в неизвестном формате");
}

std::string bodyAppData(const std::string& html)
{
    htmlDocPtr doc = htmlReadMemory(
        html.c_str(), static_cast<int>(html.size()), nullptr, "UTF-8",
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc)
    {
        throw std::runtime_error("HTML не удалось разобрать");
    }

    xmlNodePtr body = xmlDocGetRootElement(doc);
    while (body && xmlStrcmp(body->name, BAD_CAST "html") != 0)
    {
        body = body->next;
    }
    if (body)
    {
        body = body->children;
        while (body && xmlStrcmp(body->name, BAD_CAST "body") != 0)
        {
            body = body->next;
        }
    }

    xmlChar* rawData = body ? xmlGetProp(body, BAD_CAST "data-app-data") : nullptr;
    if (!rawData)
    {
        xmlFreeDoc(doc);
        throw std::runtime_error("На главной странице не найден data-app-data");
    }

    std::string result = reinterpret_cast<const char*>(rawData);
    xmlFree(rawData);
    xmlFreeDoc(doc);
    return result;
}

std::string nodeProperty(xmlNodePtr node, const char* property)
{
    xmlChar* rawValue = xmlGetProp(node, BAD_CAST property);
    if (!rawValue)
    {
        return {};
    }
    std::string value = reinterpret_cast<const char*>(rawValue);
    xmlFree(rawValue);
    return value;
}

std::string descendantText(xmlXPathContextPtr context, xmlNodePtr parent, const char* expression)
{
    context->node = parent;
    xmlXPathObjectPtr result = xmlXPathEvalExpression(BAD_CAST expression, context);
    if (!result || !result->nodesetval || result->nodesetval->nodeNr == 0)
    {
        if (result)
        {
            xmlXPathFreeObject(result);
        }
        return {};
    }

    xmlChar* rawText = xmlNodeGetContent(result->nodesetval->nodeTab[0]);
    std::string text;
    if (rawText)
    {
        text = reinterpret_cast<const char*>(rawText);
        xmlFree(rawText);
    }
    xmlXPathFreeObject(result);
    return trim(text);
}
}

std::string FunPayParser::parseNameFromHomePage(const std::string& html)
{
    if (html.empty()) {
        throw std::runtime_error("HTML-строка пуста");
    }

    htmlDocPtr doc = htmlReadMemory(
        html.c_str(),
        static_cast<int>(html.size()),
        nullptr,
        "UTF-8",
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
    );

    if (!doc) {
        throw std::runtime_error("HTML не удалось разобрать");
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFreeDoc(doc);
        throw std::runtime_error("Не удалось создать XPath-контекст");
    }

    const xmlChar* xpathExpr = BAD_CAST "//div[contains(concat(' ', normalize-space(@class), ' '), ' user-link-name ')]";
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (!xpathObj) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        throw std::runtime_error("Ошибка выполнения XPath-запроса");
    }

    if (!xpathObj->nodesetval || xpathObj->nodesetval->nodeNr == 0) {
        xmlXPathFreeObject(xpathObj);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        throw std::runtime_error("Не найден ник: ключ невалиден или HTML FunPay изменился");
    }

    xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
    xmlChar* rawContent = xmlNodeGetContent(node);

    std::string nickName;
    if (rawContent) {
        nickName = reinterpret_cast<const char*>(rawContent);
        xmlFree(rawContent);
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return nickName;
}

int FunPayParser::parseBalanceFromHomePage(const std::string& html)
{
    if (html.empty()) {
        throw std::runtime_error("HTML-строка пуста");
    }

    htmlDocPtr doc = htmlReadMemory(
        html.c_str(),
        static_cast<int>(html.size()),
        nullptr,
        "UTF-8",
        HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING
    );

    if (!doc) {
        throw std::runtime_error("Не удалось разобрать HTML");
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx) {
        xmlFreeDoc(doc);
        throw std::runtime_error("Не удалось создать XPath-контекст");
    }

    const xmlChar* xpathExpr = BAD_CAST "//span[contains(concat(' ', normalize-space(@class), ' '), ' badge-balance ')]";
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if (!xpathObj) {
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        throw std::runtime_error("Ошибка выполнения XPath");
    }

    if (!xpathObj->nodesetval || xpathObj->nodesetval->nodeNr == 0) {
        xmlXPathFreeObject(xpathObj);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        throw std::runtime_error("Баланс не найден на странице");
    }

    xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
    xmlChar* rawContent = xmlNodeGetContent(node);

    std::string balanceText;
    if (rawContent) {
        balanceText = reinterpret_cast<const char*>(rawContent);
        xmlFree(rawContent);
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);

    return stoi(balanceText); // Вернет например "721 ₽ | 72.1%"
}

std::string FunPayParser::parseCsrfToken(const std::string& html)
{
    const Json appData = Json::parse(bodyAppData(html));
    if (!appData.contains("csrf-token"))
    {
        throw std::runtime_error("В data-app-data не найден csrf-token");
    }
    return appData.at("csrf-token").get<std::string>();
}

int64_t FunPayParser::parseUserId(const std::string& html)
{
    const Json appData = Json::parse(bodyAppData(html));
    if (!appData.contains("userId"))
    {
        throw std::runtime_error("В data-app-data не найден userId");
    }
    return jsonInteger(appData.at("userId"));
}

std::vector<ChatPreview> FunPayParser::parseChatPreviews(
    const std::string& runnerResponse,
    std::string& eventTag)
{
    const Json response = Json::parse(runnerResponse);
    std::vector<ChatPreview> previews;

    for (const Json& object : response.value("objects", Json::array()))
    {
        if (object.value("type", "") != "chat_bookmarks")
        {
            continue;
        }

        if (object.contains("tag") && object["tag"].is_string())
        {
            eventTag = object["tag"].get<std::string>();
        }
        if (!object.contains("data") || !object["data"].is_object())
        {
            continue;
        }

        const std::string html = object["data"].value("html", "");
        if (html.empty())
        {
            continue;
        }

        htmlDocPtr doc = htmlReadMemory(
            html.c_str(), static_cast<int>(html.size()), nullptr, "UTF-8",
            HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_RECOVER);
        if (!doc)
        {
            throw std::runtime_error("Не удалось разобрать список чатов FunPay");
        }

        xmlXPathContextPtr context = xmlXPathNewContext(doc);
        xmlXPathObjectPtr chats = context
            ? xmlXPathEvalExpression(
                BAD_CAST "//a[contains(concat(' ', normalize-space(@class), ' '), ' contact-item ')]",
                context)
            : nullptr;

        if (chats && chats->nodesetval)
        {
            for (int index = 0; index < chats->nodesetval->nodeNr; ++index)
            {
                xmlNodePtr chat = chats->nodesetval->nodeTab[index];
                const std::string chatId = nodeProperty(chat, "data-id");
                const std::string messageId = nodeProperty(chat, "data-node-msg");
                if (chatId.empty() || messageId.empty())
                {
                    continue;
                }

                ChatPreview preview;
                preview.chatId = std::stoll(chatId);
                preview.lastMessageId = std::stoll(messageId);
                preview.interlocutorName = descendantText(
                    context, chat,
                    ".//div[contains(concat(' ', normalize-space(@class), ' '), ' media-user-name ')]");
                preview.unread = (" " + nodeProperty(chat, "class") + " ").find(" unread ") != std::string::npos;
                previews.push_back(std::move(preview));
            }
        }

        if (chats)
        {
            xmlXPathFreeObject(chats);
        }
        if (context)
        {
            xmlXPathFreeContext(context);
        }
        xmlFreeDoc(doc);
    }

    return previews;
}

std::vector<FunPayMessage> FunPayParser::parseMessages(
    const std::string& runnerResponse,
    const std::unordered_map<int64_t, std::string>& chatNames)
{
    const Json response = Json::parse(runnerResponse);
    std::vector<FunPayMessage> result;

    for (const Json& object : response.value("objects", Json::array()))
    {
        if (object.value("type", "") != "chat_node" ||
            !object.contains("data") || !object["data"].is_object())
        {
            continue;
        }

        const Json& data = object["data"];
        if (!data.contains("node") || !data["node"].is_object() ||
            !data.contains("messages") || !data["messages"].is_array())
        {
            continue;
        }

        const int64_t chatId = jsonInteger(data["node"].at("id"));
        const auto knownName = chatNames.find(chatId);

        for (const Json& rawMessage : data["messages"])
        {
            FunPayMessage message;
            message.messageId = jsonInteger(rawMessage.at("id"));
            message.chatId = chatId;
            message.authorId = jsonInteger(rawMessage.at("author"));
            if (knownName != chatNames.end())
            {
                message.authorName = knownName->second;
            }

            const std::string html = rawMessage.value("html", "");
            htmlDocPtr doc = htmlReadMemory(
                html.c_str(), static_cast<int>(html.size()), nullptr, "UTF-8",
                HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_RECOVER);
            if (doc)
            {
                xmlXPathContextPtr context = xmlXPathNewContext(doc);
                if (context)
                {
                    message.text = descendantText(
                        context, xmlDocGetRootElement(doc),
                        "//div[contains(concat(' ', normalize-space(@class), ' '), ' chat-msg-text ')]");
                    const std::string parsedAuthor = descendantText(
                        context, xmlDocGetRootElement(doc),
                        "//div[contains(concat(' ', normalize-space(@class), ' '), ' media-user-name ')]//a");
                    if (!parsedAuthor.empty())
                    {
                        message.authorName = parsedAuthor;
                    }
                    if (message.text.empty())
                    {
                        context->node = xmlDocGetRootElement(doc);
                        xmlXPathObjectPtr image = xmlXPathEvalExpression(
                            BAD_CAST "//a[contains(concat(' ', normalize-space(@class), ' '), ' chat-img-link ')]",
                            context);
                        if (image && image->nodesetval && image->nodesetval->nodeNr > 0)
                        {
                            message.text = "[изображение]";
                        }
                        if (image)
                        {
                            xmlXPathFreeObject(image);
                        }
                    }
                    xmlXPathFreeContext(context);
                }
                xmlFreeDoc(doc);
            }

            result.push_back(std::move(message));
        }
    }

    return result;
}
