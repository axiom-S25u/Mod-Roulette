#pragma once

#include <string>
#include <functional>

bool isAlreadyInstalled(std::string const& id);

void autoInstallCursedMod(
    std::string const& id,
    std::function<void(bool ok, std::string msg)> onDone
);
