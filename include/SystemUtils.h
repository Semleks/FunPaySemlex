//
// Created by semleks on 08.08.2026.
//

#ifndef FUNPAYSEMLEX_SYSTEMUTILS_H
#define FUNPAYSEMLEX_SYSTEMUTILS_H
#include <filesystem>


class SystemUtils
{
public:
    static std::filesystem::path getExecutableDirectory();
};


#endif //FUNPAYSEMLEX_SYSTEMUTILS_H
