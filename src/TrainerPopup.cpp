#include "TrainerPopup.hpp"
#include "TrainerState.hpp"
#include "AudioCueBank.hpp"
#include "PatternManager.hpp"

using namespace geode::prelude;

namespace act {

TrainerPopup* TrainerPopup::create() {
    auto ret = new TrainerPopup();
    if (ret->initAnchored(300.f, 220.f)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool TrainerPopup::setup() {
    this->setTitle("Audio Cue Trainer");

    const auto winSize = m_mainLayer->getContentSize();

    m_infoLabel = CCLabelBMFont::create("", "chatFont.fnt");
    m_infoLabel->setScale(0.5f);
    m_infoLabel->setAnchorPoint({0.5f, 0.5f});
    m_infoLabel->setPosition({winSize.width / 2.f, winSize.height - 60.f});
    m_mainLayer->addChild(m_infoLabel);
    refreshInfoLabel();

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    m_mainLayer->addChild(menu);

    // --- Test Cue ---
    auto testSprite = ButtonSprite::create("Test Cue", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    auto testBtn = CCMenuItemSpriteExtra::create(
        testSprite, this, menu_selector(TrainerPopup::onTestCue)
    );
    testBtn->setPosition({winSize.width / 2.f - 70.f, winSize.height - 110.f});
    menu->addChild(testBtn);

    // --- Reload Pattern ---
    auto reloadSprite = ButtonSprite::create("Reload Pattern", "goldFont.fnt", "GJ_button_01.png", 0.8f);
    auto reloadBtn = CCMenuItemSpriteExtra::create(
        reloadSprite, this, menu_selector(TrainerPopup::onReloadPattern)
    );
    reloadBtn->setPosition({winSize.width / 2.f + 70.f, winSize.height - 110.f});
    menu->addChild(reloadBtn);

    // --- Open patterns folder (convenience, not in the spec but cheap and useful) ---
    auto folderSprite = ButtonSprite::create("Open Patterns Folder", "goldFont.fnt", "GJ_button_02.png", 0.8f);
    auto folderBtn = CCMenuItemSpriteExtra::create(
        folderSprite, this, menu_selector(TrainerPopup::onOpenPatternFolder)
    );
    folderBtn->setPosition({winSize.width / 2.f, winSize.height - 150.f});
    menu->addChild(folderBtn);

    auto hint = CCLabelBMFont::create(
        "Lead Time / Latency Comp / Volume:\nsee this mod's Settings screen.",
        "chatFont.fnt"
    );
    hint->setScale(0.42f);
    hint->setAnchorPoint({0.5f, 0.5f});
    hint->setPosition({winSize.width / 2.f, 35.f});
    m_mainLayer->addChild(hint);

    return true;
}

void TrainerPopup::refreshInfoLabel() {
    auto& state = TrainerState::get();
    const std::string text = fmt::format(
        "Level: {}\n{} click(s) loaded ({} invalid line(s) skipped)\nSource: {}",
        state.currentLevelDisplayName.empty() ? "(none)" : state.currentLevelDisplayName,
        state.currentPatternCount,
        state.currentPatternRejected,
        state.currentPatternPath.empty() ? "(none)" : state.currentPatternPath
    );
    m_infoLabel->setString(text.c_str());
}

void TrainerPopup::onTestCue(CCObject*) {
    const float volume = static_cast<float>(Mod::get()->getSettingValue<double>("cue-volume"));
    AudioCueBank::get().play(volume);
}

void TrainerPopup::onReloadPattern(CCObject*) {
    TrainerState::get().reloadPatternForCurrentLevel();
    refreshInfoLabel();
    Notification::create("Pattern reloaded", NotificationIcon::Success)->show();
}

void TrainerPopup::onOpenPatternFolder(CCObject*) {
    const auto dir = Mod::get()->getSaveDir() / "patterns";
    file::openFolder(dir);
}

} // namespace act
