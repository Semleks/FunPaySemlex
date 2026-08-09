#pragma once

#include <cstdint>
#include <stop_token>
#include <string>
#include <unordered_map>
#include <vector>

#include "FunPayAPI/FunPayParser.h"
#include "FunPayAPI/FunPayRequest.h"

class AutoRaiseService
{
    struct Category
    {
        int64_t id = 0;
        std::string name;
        std::vector<int64_t> subcategoryIds;
    };

    struct RaiseResult
    {
        bool success = false;
        int64_t waitSeconds = 0;
        std::string message;
    };

    FunPayRequest request;
    FunPayParser parser;
    std::vector<Category> categories;
    std::unordered_map<int64_t, int64_t> nextRaiseTime;

public:
    AutoRaiseService(std::string userAgent, std::string goldenKey);

    void Run(std::stop_token stopToken);

private:
    void loadCategories();
    RaiseResult raiseCategory(const Category& category);

    static std::vector<Category> parseCategories(
        const std::string& mainPage,
        const std::string& profilePage);
    static RaiseResult parseRaiseResult(const std::string& response);
    static int64_t parseWaitTime(const std::string& message);
    static std::string formatDuration(int64_t seconds);
    static bool wait(std::stop_token stopToken, int64_t seconds);
};
