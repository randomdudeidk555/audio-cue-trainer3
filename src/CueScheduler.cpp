#include "CueScheduler.hpp"
#include <algorithm>

namespace act {

void CueScheduler::setPattern(std::vector<double> sortedTimestampsSeconds) {
    m_timestamps = std::move(sortedTimestampsSeconds);
    m_nextIndex = 0;
}

void CueScheduler::clear() {
    m_timestamps.clear();
    m_nextIndex = 0;
}

void CueScheduler::update(double currentGameTimeSeconds) {
    if (m_timestamps.empty()) return;

    // NOTE: this is a `while`, not an `if`. If a lag spike causes several
    // frames' worth of time to pass between two update() calls, more than
    // one cue's fire-time window can be crossed at once - a `while` loop
    // makes sure none of them get silently skipped (each still fires,
    // just back-to-back instead of evenly spaced, which is the correct
    // behavior for a missed-frame situation).
    while (m_nextIndex < m_timestamps.size()) {
        const double clickTime = m_timestamps[m_nextIndex];
        const double fireTime = clickTime - m_leadTimeSeconds - m_latencyCompSeconds;

        // Small epsilon avoids a cue being pushed one frame later purely
        // due to floating point rounding of the game clock.
        if (currentGameTimeSeconds + 1e-9 >= fireTime) {
            if (m_onFire) {
                m_onFire(clickTime, currentGameTimeSeconds);
            }
            m_nextIndex++;
        } else {
            break;
        }
    }
}

void CueScheduler::resyncToTime(double currentGameTimeSeconds) {
    // We want the first index whose *fire time* (clickTime - lead - comp)
    // is still >= currentGameTimeSeconds, i.e. hasn't been "missed" yet.
    // Equivalently: the first clickTime >= currentGameTimeSeconds + lead + comp.
    // std::lower_bound gives us exactly that in O(log n), which matters
    // for patterns with many hundreds of timestamps.
    const double threshold = currentGameTimeSeconds + m_leadTimeSeconds + m_latencyCompSeconds;
    auto it = std::lower_bound(m_timestamps.begin(), m_timestamps.end(), threshold);
    m_nextIndex = static_cast<size_t>(it - m_timestamps.begin());
}

} // namespace act
