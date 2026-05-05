#include "api.hpp"
#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/utils/string.hpp>
#include <matjson.hpp>
#include <random>
#include <thread>

using namespace geode::prelude;

static bool isBlacklisted(std::string const& id, std::string const& name) { // would add more lwk
    static std::vector<std::string> banned = {
        "axiom.mod-roulette",
        "geode.node-ids",
        "cvolton.level-id-api",
        "prevter.imageplus"
    };
    for (auto& b : banned) if (b == id) return true;
    auto lowerName = geode::utils::string::toLower(name);
    auto lowerId = geode::utils::string::toLower(id);
    if (geode::utils::string::contains(lowerName, "api")) return true;
    if (geode::utils::string::contains(lowerId, "api")) return true;
    return false;
}

static void bounceToMain(std::function<void()> fn) {
    Loader::get()->queueInMainThread(std::move(fn));
}

static std::string currentPlatformStr() {
#ifdef GEODE_IS_WINDOWS
    return "windows";
#else
    return GEODE_PLATFORM_SHORT_IDENTIFIER;
#endif
}

void rollCursedMod(
    std::function<void(CursedMod)> onSuccess,
    std::function<void(std::string)> onFail
) {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> pageDist(1, 10);
    int page = pageDist(rng);

    auto url = fmt::format(
        "https://api.geode-sdk.org/v1/mods?page={}&per_page=100&sort=downloads&platform={}",
        page, currentPlatformStr()
    );

    std::thread([url, onSuccess, onFail]() {
        web::WebRequest req;
        req.userAgent("RandomModDownload/1.0 (Axiom)");
        req.timeout(std::chrono::seconds(15));

        auto res = req.getSync(url);
        if (!res.ok()) {
            int code = res.code();
            bounceToMain([onFail, code]() {
                onFail(fmt::format("api kinda dead rn (http {})", code));
            });
            return;
        }
        auto strRes = res.string();
        if (strRes.isErr()) {
            bounceToMain([onFail]() { onFail("api gave us garbage. it happens"); });
            return;
        }
        auto parsed = matjson::parse(strRes.unwrap());
        if (parsed.isErr()) {
            bounceToMain([onFail]() { onFail("json parse died. blame the index"); });
            return;
        }
        auto root = parsed.unwrap();
        if (!root.contains("payload")) {
            bounceToMain([onFail]() { onFail("response shape sus, no payload"); });
            return;
        }
        auto const& payload = root["payload"];
        if (!payload.contains("data") || !payload["data"].isArray()) {
            bounceToMain([onFail]() { onFail("payload.data aint an array"); });
            return;
        }

        std::vector<CursedMod> pool;
        auto const& dataArr = payload["data"];
        for (size_t i = 0; i < dataArr.size(); ++i) {
            auto const& entry = dataArr[i];
            if (!entry.contains("id")) continue;
            auto id = entry["id"].asString().unwrapOr("");
            if (id.empty()) continue;

            CursedMod cm;
            cm.id = id;
            cm.downloads = (int)entry["download_count"].asInt().unwrapOr(0);

            bool platformOk = false;
            auto myPlatform = currentPlatformStr();
            if (entry.contains("versions") && entry["versions"].isArray()
                && entry["versions"].size() > 0) {
                auto const& v = entry["versions"][0];
                cm.name = v["name"].asString().unwrapOr(id);
                cm.description = v["description"].asString().unwrapOr("no description");
                cm.version = v["version"].asString().unwrapOr("v1.0.0");

                if (v.contains("platforms") && v["platforms"].isArray()) {
                    for (size_t j = 0; j < v["platforms"].size(); ++j) {
                        auto p = v["platforms"][j].asString().unwrapOr("");
                        if (geode::utils::string::contains(myPlatform, p)) {
                            platformOk = true;
                            break;
                        }
                    }
                } else {
                    platformOk = true;
                }
            } else {
                cm.name = id;
                cm.description = "no description";
                cm.version = "v1.0.0";
            }
            if (!platformOk) continue;

            if (entry.contains("developers") && entry["developers"].isArray()
                && entry["developers"].size() > 0) {
                cm.developer = entry["developers"][0]["display_name"]
                    .asString().unwrapOr("???");
            } else {
                cm.developer = "???";
            }

            if (isBlacklisted(cm.id, cm.name)) continue;

            pool.push_back(cm);
        }

        if (pool.empty()) {
            bounceToMain([onFail]() { onFail("no rollable mods. rng is rigged"); });
            return;
        }

        static std::mt19937 rng2{std::random_device{}()};
        std::uniform_int_distribution<size_t> dist(0, pool.size() - 1);
        auto picked = pool[dist(rng2)];
        bounceToMain([onSuccess, picked]() { onSuccess(picked); });
    }).detach();
}
