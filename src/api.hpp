#pragma once

#include <string>
#include <vector>
#include <functional>

struct CursedMod {
    std::string id;
    std::string name;
    std::string developer;
    std::string description;
    std::string version;
    int downloads;
};

void rollCursedMod(
    std::function<void(CursedMod)> onSuccess,
    std::function<void(std::string)> onFail
);
