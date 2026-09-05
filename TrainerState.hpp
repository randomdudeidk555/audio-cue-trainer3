#pragma once
#include "CueScheduler.hpp"
#include <string>

class GJGameLevel;

namespace act {

// ---------------------------------------------------------------------------
// TrainerState
//
// Single shared instance tying together: which level is active, which
// pattern file was loaded for it, and the CueScheduler driving playback.
// PlayLayer hooks (main.cpp) call into this; it never plays sound itself
// or reads files itself - it just coordinates PatternManager,
// CueScheduler and AudioCueBank so those pieces don't need to know about
// each other.
// ---------------------------------------------------------------------------
class TrainerState {
public:
    static TrainerState& get();

    CueScheduler scheduler;

    std::string currentLevelKey;
    std::string currentLevelDisplayName;
    std::string currentPatternPath;
    int currentPatternCount = 0;
    int currentPatternRejected = 0;

    // Call when a level is entered (PlayLayer::init). Loads the right
    // pattern file for this level and arms the scheduler at t=0.
    void setCurrentLevel(GJGameLevel* level);

    // Re-reads the pattern file from disk for whichever level is current
    // (used by both level-start and the "Reload Pattern" button).
    void reloadPatternForCurrentLevel();
};

} // namespace act
