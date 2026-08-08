//
// Created by semleks on 08.08.2026.
//

#include "SystemUtils.h"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
    #define OS_WINDOWS
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
    #define OS_LINUX
#endif

std::filesystem::path SystemUtils::getExecutableDirectory() {
#if defined(OS_WINDOWS)
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();

#elif defined(OS_LINUX)
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (count != -1) {
        buffer[count] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
#endif

    return std::filesystem::current_path();
}