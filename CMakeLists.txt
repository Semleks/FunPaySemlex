cmake_minimum_required(VERSION 3.18)
project(FunPaySemlex)

include(FetchContent)

FetchContent_Declare(
        cpr
        GIT_REPOSITORY https://github.com/libcpr/cpr.git
        GIT_TAG        1.10.5
)
FetchContent_MakeAvailable(cpr)

find_package(LibXml2 REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_executable(FunPaySemlex main.cpp
        src/Config.cpp
        include/Config.h
        src/SystemUtils.cpp
        include/SystemUtils.h
        src/FunPayAPI/FunPayAccount.cpp
        include/FunPayAPI/FunPayAccount.h
        src/FunPayAPI/FunPayRequest.cpp
        include/FunPayAPI/FunPayRequest.h
        src/FunPayAPI/FunPayParser.cpp
        include/FunPayAPI/FunPayParser.h
        include/FunPayAPI/Messages/Types.h
        src/FunPayAPI/Messages/ChatPollingService.cpp
        include/FunPayAPI/Messages/ChatPollingService.h
        src/FunPayAPI/Lots/AutoRaiseService.cpp
        include/FunPayAPI/Lots/AutoRaiseService.h
        src/TelegramBot/Telegram.cpp
        include/TelegramBot/Telegram.h)

target_include_directories(FunPaySemlex PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(FunPaySemlex PRIVATE cpr::cpr LibXml2::LibXml2)
