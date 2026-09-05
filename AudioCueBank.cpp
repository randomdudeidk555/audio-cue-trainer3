#include "AudioCueBank.hpp"
#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <cmath>
#include <algorithm>

using namespace geode::prelude;

namespace act {

AudioCueBank& AudioCueBank::get() {
    static AudioCueBank instance;
    return instance;
}

namespace {
    constexpr int kSampleRate = 44100;
    constexpr double kDurationSeconds = 0.045; // 45ms - short & crisp, negligible decode cost
    constexpr double kToneHz = 1400.0;         // bright/percussive, easy to distinguish from GD's own SFX

    template <typename T>
    void appendLE(std::vector<uint8_t>& buf, T value) {
        auto* p = reinterpret_cast<uint8_t*>(&value);
        buf.insert(buf.end(), p, p + sizeof(T));
    }
}

std::vector<uint8_t> AudioCueBank::buildClickWavBytes() {
    const int numSamples = static_cast<int>(kSampleRate * kDurationSeconds);
    const int byteRate = kSampleRate * 1 * 2; // mono, 16-bit
    const int dataSize = numSamples * 2;

    std::vector<uint8_t> wav;
    wav.reserve(44 + dataSize);

    // ---- Standard 44-byte canonical PCM WAV header ----
    wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
    appendLE<uint32_t>(wav, 36 + dataSize);
    wav.insert(wav.end(), {'W', 'A', 'V', 'E'});
    wav.insert(wav.end(), {'f', 'm', 't', ' '});
    appendLE<uint32_t>(wav, 16);          // PCM fmt chunk size
    appendLE<uint16_t>(wav, 1);           // format = PCM
    appendLE<uint16_t>(wav, 1);           // channels = mono
    appendLE<uint32_t>(wav, kSampleRate);
    appendLE<uint32_t>(wav, byteRate);
    appendLE<uint16_t>(wav, 2);           // block align
    appendLE<uint16_t>(wav, 16);          // bits per sample
    wav.insert(wav.end(), {'d', 'a', 't', 'a'});
    appendLE<uint32_t>(wav, dataSize);

    // ---- PCM data: enveloped sine burst ----
    // A hard-cut sine wave pops audibly at start/end, which both sounds
    // bad AND makes the perceived "onset" of the click fuzzy/inconsistent
    // between plays. A fast attack + short decay keeps the transient
    // sharp (good for precise timing perception) while eliminating pops.
    constexpr double attack = 0.003;   // 3ms fade-in
    constexpr double release = 0.020;  // 20ms fade-out
    for (int i = 0; i < numSamples; i++) {
        const double t = static_cast<double>(i) / kSampleRate;
        double envelope;
        if (t < attack) {
            envelope = t / attack;
        } else if (t > kDurationSeconds - release) {
            envelope = (kDurationSeconds - t) / release;
        } else {
            envelope = 1.0;
        }
        envelope = std::clamp(envelope, 0.0, 1.0);

        const double sample = std::sin(2.0 * M_PI * kToneHz * t) * envelope;
        const int16_t pcm = static_cast<int16_t>(sample * 32000.0);
        appendLE<int16_t>(wav, pcm);
    }

    return wav;
}

void AudioCueBank::ensureLoaded() {
    if (m_loaded) return;

    auto* fmod = FMODAudioEngine::sharedEngine();
    if (!fmod || !fmod->m_system) {
        // COMPATIBILITY NOTE: `FMODAudioEngine::m_system` (an FMOD::System*)
        // is a member exposed by Geode's generated bindings for this GD
        // version. If your installed Geode SDK's
        // Geode/binding/FMODAudioEngine.hpp doesn't have this field (name
        // changed / made private), check that file for the current way to
        // reach the FMOD::System instance and update this one call site.
        geode::log::error("AudioCueTrainer: FMOD system unavailable, cannot load cue sound.");
        return;
    }

    const auto wavBytes = buildClickWavBytes();

    FMOD_CREATESOUNDEXINFO exinfo{};
    exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
    exinfo.length = static_cast<unsigned int>(wavBytes.size());

    // FMOD_OPENMEMORY (as opposed to FMOD_OPENMEMORY_POINT) makes FMOD
    // copy the buffer internally, so `wavBytes` can safely be destroyed
    // once this call returns - no need to keep it alive ourselves.
    const auto result = fmod->m_system->createSound(
        reinterpret_cast<const char*>(wavBytes.data()),
        FMOD_OPENMEMORY | FMOD_CREATESAMPLE | FMOD_LOOP_OFF,
        &exinfo,
        &m_sound
    );

    if (result != FMOD_OK) {
        geode::log::error("AudioCueTrainer: failed to create cue sound (FMOD error {})", static_cast<int>(result));
        return;
    }

    m_loaded = true;
}

void AudioCueBank::play(float volume) {
    ensureLoaded();
    if (!m_loaded || !m_sound) return;

    auto* fmod = FMODAudioEngine::sharedEngine();
    if (!fmod || !fmod->m_system) return;

    FMOD::Channel* channel = nullptr;
    // Playing directly on the FMOD::System with an already-created,
    // already-resident FMOD::Sound is the lowest-latency route available:
    // there's no file path lookup, no on-demand decode, and no pass
    // through GD's own path-based playEffect() wrapper.
    const auto result = fmod->m_system->playSound(m_sound, nullptr, false, &channel);
    if (result == FMOD_OK && channel) {
        // COMPATIBILITY NOTE: `m_effectsVolume` is the SFX volume field
        // name in current bindings. If it's renamed in your SDK version,
        // check Geode/binding/FMODAudioEngine.hpp for the current name
        // (search for "effect" or "sfx").
        const float sfxVolume = fmod->m_effectsVolume;
        channel->setVolume(std::clamp(sfxVolume * volume, 0.f, 1.f));
    }
}

void AudioCueBank::unload() {
    if (m_sound) {
        m_sound->release();
        m_sound = nullptr;
    }
    m_loaded = false;
}

} // namespace act
