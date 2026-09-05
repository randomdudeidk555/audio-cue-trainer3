#pragma once
#include <fmod.hpp>
#include <vector>
#include <cstdint>

namespace act {

// ---------------------------------------------------------------------------
// AudioCueBank
//
// Synthesizes a very short "click" sound entirely in memory (a tiny
// enveloped sine burst rendered to a WAV byte buffer at runtime), so the
// mod ships with zero external audio assets. The sound is created once
// via FMOD::System::createSound(..., FMOD_OPENMEMORY | FMOD_CREATESAMPLE)
// and cached; playback goes straight through FMOD::System::playSound on
// the already-loaded FMOD::Sound, which is the lowest-latency path
// available (no file I/O, no decode-on-play, no extra GD-side effect
// wrapper indirection).
// ---------------------------------------------------------------------------
class AudioCueBank {
public:
    static AudioCueBank& get();

    // Creates the FMOD sound if it hasn't been created yet. Cheap to call
    // repeatedly - it's a no-op after the first successful call.
    void ensureLoaded();

    // Plays the cue immediately, ignoring any schedule. Used both by the
    // real cue-fire path and by the "Test Cue" button.
    void play(float volume);

    void unload();

private:
    AudioCueBank() = default;
    std::vector<uint8_t> buildClickWavBytes();

    FMOD::Sound* m_sound = nullptr;
    bool m_loaded = false;
};

} // namespace act
