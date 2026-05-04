#include "popup.hpp"
#include "api.hpp"
#include "installer.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/FLAlertLayer.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

static std::string clip(std::string s, size_t max) {
    if (s.size() <= max) return s;
    return s.substr(0, max - 3) + "...";
}

ChaosPopup* ChaosPopup::create(CursedMod mod) {
    auto ret = new ChaosPopup();
    if (ret->initChaos(360.f, 180.f, mod)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ChaosPopup::initChaos(float w, float h, CursedMod mod) {
    m_mod = mod;
    if (!Popup::init(w, h)) return false;

    this->setTitle("ROULETTE WINNER");

    auto winSize = m_mainLayer->getContentSize();
    auto cx = winSize.width / 2.f;

    auto nameLbl = CCLabelBMFont::create(clip(m_mod.name, 28).c_str(), "bigFont.fnt");
    nameLbl->setPosition({cx, winSize.height - 58.f});
    nameLbl->limitLabelWidth(300.f, 0.9f, 0.4f);
    nameLbl->setColor({255, 200, 80});
    m_mainLayer->addChild(nameLbl);

    auto devLbl = CCLabelBMFont::create(
        fmt::format("by {}", clip(m_mod.developer, 32)).c_str(),
        "goldFont.fnt"
    );
    devLbl->setPosition({cx, winSize.height - 82.f});
    devLbl->setScale(0.55f);
    m_mainLayer->addChild(devLbl);

    auto menu = CCMenu::create();
    menu->setPosition({cx, 30.f});
    m_mainLayer->addChild(menu);

    auto downloadSpr = ButtonSprite::create("Download", "bigFont.fnt", "GJ_button_01.png", 0.7f);
    auto downloadBtn = CCMenuItemSpriteExtra::create(
        downloadSpr, this, menu_selector(ChaosPopup::onDownload)
    );
    downloadBtn->setPositionX(-65.f);
    menu->addChild(downloadBtn);

    auto rerollSpr = ButtonSprite::create("Reroll", "goldFont.fnt", "GJ_button_04.png", 0.7f);
    auto rerollBtn = CCMenuItemSpriteExtra::create(
        rerollSpr, this, menu_selector(ChaosPopup::onReroll)
    );
    rerollBtn->setPositionX(65.f);
    menu->addChild(rerollBtn);

    return true;
}

void ChaosPopup::onDownload(CCObject*) {
    this->onClose(nullptr);
    autoInstallCursedMod(m_mod.id, [](bool ok, std::string msg) {
        auto notif = Notification::create(
            fmt::format("{}: {}", ok ? "downloaded" : "failed", msg),
            ok ? NotificationIcon::Success : NotificationIcon::Error,
            3.f
        );
        notif->show();
    });
}

void ChaosPopup::onReroll(CCObject*) {
    this->onClose(nullptr);
    showChaosRoll();
}

static bool startsWith(std::string const& s, std::string const& prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

void showChaosRoll() {
    auto notif = Notification::create(
        "rolling the cursed dice...", NotificationIcon::Loading, 15.f
    );
    notif->show();
    auto notifRef = Ref<Notification>(notif);

    rollCursedMod(
        [notifRef](CursedMod mod) {
            if (notifRef) notifRef->hide();
            log::info("[chaos] rolled: {} ({})", mod.name, mod.id);
            
            if (startsWith(mod.id, "axiom.")) {
                FLAlertLayer::create(
                    "ayo",
                    "<cy>oh damn i made that mod</c>",
                    "OK"
                )->show();
                showChaosRoll();
                return;
            }

            auto popup = ChaosPopup::create(mod);
            if (!popup) return;
            popup->show();
        },
        [notifRef](std::string err) {
            if (notifRef) notifRef->hide();
            FLAlertLayer::create(
                "L",
                fmt::format("<cr>roll failed:</c> {}", err),
                "fine"
            )->show();
        }
    );
}
