#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/GJGameLevel.hpp>

#include "TrainerState.hpp"
#include "AudioCueBank.hpp"
#include "TrainerPopup.hpp"

using namespace geode::prelude;
using namespace act;

namespace {

// ---------------------------------------------------------------------------
// currentLevelTime()
//
// The single point in this whole mod that reads "what time is it in the
// level right now". Everything else (CueScheduler) is fed this value and
// never reads a clock on its own.
//
// `PlayLayer::m_time` is GD's own level-elapsed-time field (used
// internally for e.g. the progress bar / "time" stat). RobTop's engine
// keeps this synced to the currently playing song whenever the level has
// one, and simply accumulates delta-time when it doesn't - either way, it
// is NOT a naive "frames elapsed * expected frame duration" counter, so
// unlike a hand-rolled frame counter it does not drift when FPS changes
// or stutters. That makes it the best "song/game clock" available to a
// mod without reaching into FMOD's channel/DSP clock directly.
//
// COMPATIBILITY NOTE: `m_time`'s exact name/offset comes from Geode's
// generated bindings (Geode/binding/PlayLayer.hpp or GJBaseGameLayer.hpp)
// for whichever GD version your SDK targets. If a future GD update
// renames this field, this is the only function that needs to change -
// look in that binding header for the level-time-in-seconds field
// (search for "time") and update the cast below.
double currentLevelTime(PlayLayer* pl) {
    if (!pl) return 0.0;
    return static_cast<double>(pl->m_time);
}

} // namespace

// ---------------------------------------------------------------------------
// PlayLayer hooks: level start / restart / per-frame check / leaving level
// ---------------------------------------------------------------------------
class $modify(ACT_PlayLayer, PlayLayer) {
    bool init(GJGameLevel* level) {
        if (!PlayLayer::init(level)) return false;

        // Pre-warm the synthesized cue sound as soon as the level starts,
        // so the very first cue of the attempt doesn't pay a one-time
        // FMOD::createSound cost right when precise timing matters most.
        AudioCueBank::get().ensureLoaded();

        // Loads the right pattern for this level (per-level file if it
        // exists, else the shared clicks.txt fallback) and arms the
        // scheduler.
        TrainerState::get().setCurrentLevel(level);

        // A brand new level attempt always starts at level-time 0.
        TrainerState::get().scheduler.resyncToTime(0.0);

        return true;
    }

    // Called every time the player dies-and-restarts, AND on manual
    // restart (R key / menu restart). This is the critical resync point:
    // without it, cues already fired in a previous attempt would stay
    // "used up" (CueScheduler::m_nextIndex wouldn't reset), so the next
    // attempt would get no cues at all - or, when resuming from a
    // practice-mode checkpoint partway through the level, cues would be
    // completely misaligned with the resumed position.
    void resetLevel() {
        PlayLayer::resetLevel();

        const double t = currentLevelTime(this);
        TrainerState::get().scheduler.resyncToTime(t);
    }

    // Runs every rendered frame. Note that `dt` is only forwarded to the
    // base game update - it is NOT used to decide when a cue fires. The
    // actual fire/no-fire decision inside CueScheduler::update() is made
    // purely by comparing currentLevelTime() (the audio-synced clock)
    // against each click's precomputed fire time, so cue timing does not
    // drift at low, high, or unstable FPS. Calling update() every frame
    // just means "check the audio-synced clock as often as we possibly
    // can" - it is a frequent *poll* of an accurate clock, not itself the
    // clock.
    void update(float dt) {
        PlayLayer::update(dt);
        TrainerState::get().scheduler.update(currentLevelTime(this));
    }

    // Called when the player leaves the level (back to the level select /
    // menu). Drops the pattern so nothing can fire during the exit
    // transition or after the layer is gone.
    void onQuit() {
        PlayLayer::onQuit();
        TrainerState::get().scheduler.clear();
    }
};

// ---------------------------------------------------------------------------
// PauseLayer hook: adds a button that opens the Audio Cue Trainer panel
// ---------------------------------------------------------------------------
class $modify(ACT_PauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        // "right-button-menu" is one of the stable node IDs Geode assigns
        // to PauseLayer's own layout (alongside "left-button-menu" and
        // "center-button-menu"). If a future GD/Geode update changes this
        // layout's IDs, use Geode's node ID inspector (in-game dev tools)
        // on the pause screen to find the current name and update this
        // one string.
        auto menu = this->getChildByID("right-button-menu");
        if (!menu) return; // fail safe rather than crash if layout differs

        auto sprite = CircleButtonSprite::createWithSpriteFrameName(
            "GJ_infoIcon_001.png",
            1.0f,
            CircleBaseColor::Green,
            CircleBaseSize::Small
        );

        auto btn = CCMenuItemSpriteExtra::create(
            sprite, this, menu_selector(ACT_PauseLayer::onOpenTrainerPanel)
        );
        btn->setID("audio-cue-trainer-button"_spr);
        menu->addChild(btn);
        menu->updateLayout();
    }

    void onOpenTrainerPanel(CCObject*) {
        TrainerPopup::create()->show();
    }
};
