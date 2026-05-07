#include "installer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/utils/web.hpp>
#include <matjson.hpp>
#include <thread>
#include <set>

using namespace geode::prelude;

bool isAlreadyInstalled(std::string const& id) {
    return Loader::get()->getInstalledMod(id) != nullptr;
}

static void mainThread(std::function<void()> fn) {
    Loader::get()->queueInMainThread(std::move(fn));
}

static std::string platformStr() {
    return GEODE_PLATFORM_SHORT_IDENTIFIER;
}

static bool downloadOne(std::string const& id, std::string const& version = "") {
    if (isAlreadyInstalled(id)) {
        auto installed = Loader::get()->getInstalledMod(id);
        if (installed && !installed->isOrWillBeEnabled()) {
            (void)installed->enable();
        }
        return true;
    }

    std::string url;
    if (version.empty()) {
        url = fmt::format(
            "https://api.geode-sdk.org/v1/mods/{}/versions/latest/download?platform={}",
            id, platformStr()
        );
    } else {
        url = fmt::format(
            "https://api.geode-sdk.org/v1/mods/{}/versions/{}/download?platform={}",
            id, version, platformStr()
        );
    }
    auto target = dirs::getModsDir() / fmt::format("{}.geode", id);

    web::WebRequest req;
    req.userAgent("ModRoulette/1.0.5 (Axiom)");
    req.timeout(std::chrono::seconds(60));
    req.followRedirects(true);

    auto res = req.getSync(url);
    if (!res.ok()) return false;

    auto wrote = res.into(target);
    if (wrote.isErr()) return false;

    log::info("saved {} {}", id, version.empty() ? "(latest)" : version);
    return true;
}

std::vector<DepInfo> fetchDependencies(std::string const& id) {
    std::vector<DepInfo> deps;

    auto url = fmt::format(
        "https://api.geode-sdk.org/v1/mods/{}/versions/latest?platform={}",
        id, platformStr()
    );

    web::WebRequest req;
    req.userAgent("ModRoulette/1.0.5 (Axiom)");
    req.timeout(std::chrono::seconds(15));

    auto res = req.getSync(url);
    if (!res.ok()) return deps;
    auto strRes = res.string();
    if (strRes.isErr()) return deps;
    auto parsed = matjson::parse(strRes.unwrap());
    if (parsed.isErr()) return deps;
    auto root = parsed.unwrap();
    if (!root.contains("payload")) return deps;
    auto const& payload = root["payload"];
    if (!payload.contains("dependencies")) return deps;
    auto const& depsField = payload["dependencies"];
    if (depsField.isNull()) return deps;
    if (!depsField.isArray()) return deps;

    auto const& depArr = payload["dependencies"];
    for (size_t i = 0; i < depArr.size(); ++i) {
        auto const& d = depArr[i];
        if (d.isNull()) continue;
        if (!d.isObject()) continue;
        if (!d.contains("mod_id")) continue;
        auto depId = d["mod_id"].asString().unwrapOr("");
        if (depId.empty()) continue;
        if (depId == "geode.node-ids") continue; // most mods use and shii so "has a dep" appaers each time
        if (d.contains("importance") && d["importance"].isString()) {
            auto imp = d["importance"].asString().unwrapOr("");
            if (imp == "suggested") continue; // reasoning was, if not needed dont give a shit lmao
        }
        DepInfo info;
        info.id = depId;
        if (d.contains("version") && d["version"].isString()) {
            auto ver = d["version"].asString().unwrapOr("");
            if (!ver.empty() && ver[0] == '=') {
                info.version = ver.substr(1);
            }
        }
        deps.push_back(info);
    }
    return deps;
}

void autoInstallCursedMod(
    std::string const& id,
    std::function<void(bool, std::string)> onDone
) {
    auto installed = Loader::get()->getInstalledMod(id);
    if (installed) {
        if (!installed->isOrWillBeEnabled()) {
            auto res = installed->enable();
            if (res.isErr()) {
                onDone(false, fmt::format("already installed but couldnt enable: {}", res.unwrapErr()));
                return;
            }
            onDone(true, "already installed, flipped it on. restart to apply");
            return;
        }
        onDone(true, "already installed n live. restart couldnt hurt tho");
        return;
    }

    std::thread([id, onDone]() {
        std::set<std::string> visited;
        std::vector<std::pair<std::string, std::string>> queue = { {id, ""} };
        std::vector<std::string> failed;
        int depCount = 0;

        while (!queue.empty()) {
            auto cur = queue.back();
            queue.pop_back();
            auto curId = cur.first;
            auto curVer = cur.second;
            if (visited.count(curId)) continue;
            visited.insert(curId);

            if (curId != id) depCount++;

            if (!downloadOne(curId, curVer)) {
                failed.push_back(curId);
                continue;
            }

            auto deps = fetchDependencies(curId);
            for (auto& d : deps) {
                if (!visited.count(d.id) && !isAlreadyInstalled(d.id)) {
                    queue.push_back({d.id, d.version});
                }
            }
        }

        mainThread([onDone, failed, depCount]() {
            if (!failed.empty()) {
                std::string list;
                for (auto& f : failed) {
                    if (!list.empty()) list += ", ";
                    list += f;
                }
                onDone(false, fmt::format("failed: {}", list));
                return;
            }
            if (depCount > 0) {
                onDone(true, fmt::format("installed + {} deps. restart gd", depCount));
            } else {
                onDone(true, "installed! restart gd to activate");
            }
        });
    }).detach();
}
