#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include "popup.hpp"

using namespace geode::prelude;

class $modify(ChaosMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto sprBg = CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png"); // id rather not use modlogo till dotdotdot makes me one
        auto label = CCLabelBMFont::create("?", "bigFont.fnt");
        label->setScale(0.5f);
        label->setPosition(sprBg->getContentSize() / 2.f);
        label->setColor({255, 100, 100});
        sprBg->addChild(label);

        auto btn = CCMenuItemSpriteExtra::create(
            sprBg, this, menu_selector(ChaosMenuLayer::onChaos)
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
