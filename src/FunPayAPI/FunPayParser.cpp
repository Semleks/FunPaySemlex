//
// Created by semleks on 09.08.2026.
//

#include "../../include/FunPayAPI/FunPayParser.h"

#include <stdexcept>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>

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