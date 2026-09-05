#include "TrainerState.hpp"
#include "PatternManager.hpp"
#include "AudioCueBank.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/GJGameLevel.hpp>

using namespace geode::prelude;

namespace act {

TrainerState& TrainerState::get() {
    static TrainerState instance;
    return instance;
}

namespace {
    // Bridges CueScheduler's plain-function callback to actual sound
    // playback. Kept free-standing (matching CueScheduler::FireCallback's
    // signature) since CueScheduler itself must stay free of FMOD/Geode
    // includes.
    void onCueFire(double scheduledClickTime, double actualFireTime) {
        if (!Mod::get()->getSettingValue<bool>("enabled")) return;

        const float volume = static_cast<float>(Mod::get()->getSettingValue<double>("cue-volume"));
        AudioCueBank::get().play(volume);

        // actualFireTime vs scheduledClickTime is NOT drift - the cue is
        // *supposed* to fire (lead + latency comp) before the click time,
        // this log line is just for debugging/tuning lead time by hand.
        geode::log::debug(
            "AudioCueTrainer: cue for click@{:.3f}s fired at t={:.3f}s",
            scheduledClickTime, actualFireTime
        );
    }

    // COMPATIBILITY NOTE: integer settings in current Geode versions are
    // read back as int64_t via getSettingValue<int64_t>. If your SDK
    // version uses a different underlying type for "int" settings (some
    // older/newer versions use plain `int` or `double`), adjust the two
    // calls below accordingly - this is the only place that needs to
    // change.
    double leadTimeSecondsFromSettings() {
        return static_cast<double>(Mod::get()->getSettingValue<int64_t>("lead-time-ms")) / 1000.0;
    }
    double latencyCompSecondsFromSettings() {
        return static_cast<double>(Mod::get()->getSettingValue<int64_t>("latency-comp-ms")) / 1000.0;
    }
}

void TrainerState::setCurrentLevel(GJGameLevel* level) {
    currentLevelKey = makeLevelKey(level);
    currentLevelDisplayName = level ? std::string(level->m_levelName) : "Unknown Level";
    reloadPatternForCurrentLevel();
}

void TrainerState::reloadPatternForCurrentLevel() {
    auto loaded = PatternManager::get().loadForLevel(currentLevelKey);
    currentPatternPath = loaded.sourcePath;
    currentPatternCount = static_cast<int>(loaded.timestampsSeconds.size());
    currentPatternRejected = loaded.rejectedLineCount;

    scheduler.setPattern(std::move(loaded.timestampsSeconds));
    scheduler.setFireCallback(&onCueFire);
    scheduler.setLeadTimeSeconds(leadTimeSecondsFromSettings());
    scheduler.setLatencyCompSeconds(latencyCompSecondsFromSettings());

    // A freshly (re)loaded pattern during an active attempt should still
    // line up with wherever the level currently is, not always assume 0 -
    // callers that know it's a hard level restart also call
    // resyncToTime(0.0) themselves right after, which is harmless/cheap.
    scheduler.resyncToTime(0.0);
}

} // namespace act
