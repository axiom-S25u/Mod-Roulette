#pragma once

#include <string>
#include <functional>
#include <vector>

struct DepInfo {
    std::string id;
    std::string version;
};

bool isAlreadyInstalled(std::string const& id);

std::vector<DepInfo> fetchDependencies(std::string const& id);

std::vector<std::string> fetchIncompatibilities(std::string const& id);

bool disableMod(std::string const& id);

void autoInstallCursedMod(
    std::string const& id,
    std::function<void(bool ok, std::string msg)> onDone
);
