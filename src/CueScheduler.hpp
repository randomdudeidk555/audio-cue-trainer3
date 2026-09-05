#pragma once
#include <vector>
#include <cstddef>

namespace act {

// ---------------------------------------------------------------------------
// CueScheduler
//
// Pure timing logic - no FMOD, no Geode, no cocos2d. This is deliberate:
// it makes the actual "when does a cue fire" decision trivially easy to
// reason about (and unit-test outside the game if you want to) and keeps
// audio/engine concerns entirely out of the timing math.
//
// The scheduler is fed the current, most-accurate game/song time once per
// frame from outside (see main.cpp -> currentLevelTime()). It never reads
// a clock itself and never counts frames - "how many frames since the
// last cue" is exactly the kind of frame-based timer this mod avoids,
// because that drifts as FPS changes. Instead every decision is made by
// comparing the *actual elapsed level time* against each click's
// precomputed fire time, so cue timing stays correct whether the game is
// running at 30, 60, 144 or 240 FPS.
// ---------------------------------------------------------------------------
class CueScheduler {
public:
    // Called once per fired cue. Kept as a plain function pointer (rather
    // than std::function) so this header stays trivially cheap to include
    // and has no hidden allocations on the hot path.
    using FireCallback = void (*)(double scheduledClickTime, double actualFireTime);

    // Installs a new pattern. Timestamps MUST already be sorted ascending
    // (PatternManager guarantees this). Resets internal progress to the
    // start - call resyncToTime() afterwards if the level isn't actually
    // starting from t=0 (e.g. hot-reloading mid-attempt).
    void setPattern(std::vector<double> sortedTimestampsSeconds);

    // Drops the current pattern entirely. Safe to call at any time; after
    // this, update() becomes a no-op until setPattern() is called again.
    void clear();

    // Must be called every frame (or as often as practical) with the
    // current level/song time in seconds. Fires every cue whose fire-time
    // has been reached or passed since the last call.
    void update(double currentGameTimeSeconds);

    // Re-aligns "which cue comes next" to match currentGameTimeSeconds.
    // MUST be called whenever the level (re)starts, the player respawns
    // to a checkpoint, or the pattern is reloaded - otherwise cues from a
    // previous attempt could be skipped entirely or fire all at once.
    void resyncToTime(double currentGameTimeSeconds);

    void setLeadTimeSeconds(double seconds) { m_leadTimeSeconds = seconds; }
    void setLatencyCompSeconds(double seconds) { m_latencyCompSeconds = seconds; }
    void setFireCallback(FireCallback cb) { m_onFire = cb; }

    size_t patternSize() const { return m_timestamps.size(); }
    size_t nextIndex() const { return m_nextIndex; }

private:
    std::vector<double> m_timestamps;   // ascending seconds, one per intended click
    size_t m_nextIndex = 0;             // index of the next cue not yet fired
    double m_leadTimeSeconds = 0.170;   // default 170ms, overridden from settings
    double m_latencyCompSeconds = 0.0;  // overridden from settings
    FireCallback m_onFire = nullptr;
};

} // namespace act
