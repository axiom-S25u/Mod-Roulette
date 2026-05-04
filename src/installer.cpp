#include "installer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/loader/Loader.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/loader/Dirs.hpp>
#include <Geode/utils/web.hpp>
#include <thread>

using namespace geode::prelude;

bool isAlreadyInstalled(std::string const& id) {
    return Loader::get()->getInstalledMod(id) != nullptr;
}

static void mainThread(std::function<void()> fn) {
    Loader::get()->queueInMainThread(std::move(fn));
}

void autoInstallCursedMod(
    std::string const& id,
    std::function<void(bool, std::string)> onDone
) {
    // already installed? just flip enable if needed and bail
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

    auto url = fmt::format(
        "https://api.geode-sdk.org/v1/mods/{}/versions/latest/download", id
    );
    auto target = dirs::getModsDir() / fmt::format("{}.geode", id);

    std::thread([url, target, id, onDone]() {
        web::WebRequest req;
        req.userAgent("Mod Roulette/1.0 (Axiom)");
        req.timeout(std::chrono::seconds(60));
        req.followRedirects(true);

        auto res = req.getSync(url);
        if (!res.ok()) {
            int code = res.code();
            mainThread([onDone, code]() {
                onDone(false, fmt::format("download failed (http {})", code));
            });
            return;
        }

        auto wrote = res.into(target);
        if (wrote.isErr()) {
            auto err = wrote.unwrapErr();
            mainThread([onDone, err]() {
                onDone(false, fmt::format("couldnt save file: {}", err));
            });
            return;
        }

        log::info("saved", id);
        mainThread([onDone]() {
            onDone(true, "installed! restart GD to activate");
        });
    }).detach();
}
