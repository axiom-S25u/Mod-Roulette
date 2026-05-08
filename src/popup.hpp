#pragma once

#include <Geode/ui/Popup.hpp>
#include "api.hpp"
#include <string>

using namespace geode::prelude;

class ChaosPopup : public Popup {
protected:
    void onDownload(CCObject*);
    void onReroll(CCObject*);
    void onDisableAndKeep(CCObject*);

public:
    bool initChaos(float w, float h, CursedMod mod);
    static ChaosPopup* create(CursedMod mod);

    CursedMod m_mod;
    std::string m_incompatibleMod;
};

void showChaosRoll();
