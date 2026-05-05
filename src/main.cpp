#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/utils/string.hpp>
#include "popup.hpp"

using namespace geode::prelude;

class $modify(ChaosMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto logoPath = Mod::get()->getResourcesDir() / "logo.png";
        auto logo = CCSprite::create(geode::utils::string::pathToString(logoPath).c_str());
        logo->setScale(2.5f);

        auto btn = CCMenuItemSpriteExtra::create(
            logo, this, menu_selector(ChaosMenuLayer::onChaos)
        );
        btn->setID("chaos-roll"_spr);

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onChaos(CCObject*) {
        showChaosRoll();
    }
};

$on_mod(Loaded) {
    log::info("download worked");
}
