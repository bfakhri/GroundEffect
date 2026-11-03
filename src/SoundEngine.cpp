#include "SoundEngine.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

static const int SAMPLE_RATE = 44100; // [Hz] Audio sampling rate in Hertz
static const float PI = 3.14159265358979323846f; // [rad] Circular constant

SoundEngine::SoundEngine() {}

SoundEngine::~SoundEngine() {
    shutdown();
}

bool SoundEngine::init() {
    if (m_initialized) return true;

    SDL_AudioSpec wanted, obtained;
    SDL_zero(wanted);
    wanted.freq = SAMPLE_RATE; // [Hz]
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 1;
    wanted.samples = 512;
    wanted.callback = audioCallback;
    wanted.userdata = this;

    m_devId = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained, 0);
    if (m_devId == 0) {
        std::cerr << "Warning: Could not open audio device: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_PauseAudioDevice(m_devId, 0); // Unpause
    m_initialized = true;
    return true;
}

void SoundEngine::shutdown() {
    if (m_devId != 0) {
        SDL_CloseAudioDevice(m_devId);
        m_devId = 0;
    }
    m_initialized = false;
}

void SoundEngine::setThrust(float throttle) {
    m_thrustThrottle.store(std::clamp(throttle, 0.0f, 1.0f)); // [0.0 - 1.0, dimensionless]
}

void SoundEngine::playRCS() {
    m_rcsTimer.store(0.08f); // [s] 80ms puff
}

void SoundEngine::playBeep(float freq, float duration) {
    m_beepFreq.store(freq);       // [Hz]
    m_beepTimer.store(duration);  // [s]
}

void SoundEngine::playImpact(float intensity) {
    m_impactIntensity.store(std::clamp(intensity, 0.2f, 1.0f)); // [0.2 - 1.0, dimensionless]
    m_impactTimer.store(0.22f); // [s] 220ms metallic impact crunch
}

void SoundEngine::playExplosion() {
    m_explosionTimer.store(1.6f); // [s]
}

void SoundEngine::playLandFanfare() {
    m_fanfareTimer.store(1.2f); // [s]
}

void SoundEngine::playCoinSound() {
    m_coinTimer.store(0.35f); // [s]
}

void SoundEngine::playImpulseBoost() {
    m_boostTimer.store(0.28f); // [s] Punchy rocket boost
}

float SoundEngine::getNoise() {
    m_rngState = m_rngState * 1664525u + 1013904223u;
    return ((float)(m_rngState >> 16) / 32768.0f) - 1.0f;
}

void SoundEngine::audioCallback(void* userdata, Uint8* stream, int len) {
    SoundEngine* engine = static_cast<SoundEngine*>(userdata);
    float* output = reinterpret_cast<float*>(stream);
    int numSamples = len / sizeof(float);
    engine->generateAudio(output, numSamples);
}

void SoundEngine::generateAudio(float* output, int numSamples) {
    const float dt = 1.0f / (float)SAMPLE_RATE;

    float throttle = m_thrustThrottle.load();
    float rcsTime = m_rcsTimer.load();
    float impTime = m_impactTimer.load();
    float impIntensity = m_impactIntensity.load();
    float expTime = m_explosionTimer.load();
    float beepTime = m_beepTimer.load();
    float beepFreq = m_beepFreq.load();
    float fanfareTime = m_fanfareTimer.load();
    float coinTime = m_coinTimer.load();
    float boostTime = m_boostTimer.load();

    for (int i = 0; i < numSamples; ++i) {
        float sample = 0.0f;

        // 1. Rocket engine thrust rumble
        if (throttle > 0.01f) {
            float rumbleFreq = 45.0f + throttle * 65.0f; // [Hz] Modulation frequency (45 Hz to 110 Hz)
            m_thrustPhase += 2.0f * PI * rumbleFreq * dt; // [rad]
            if (m_thrustPhase > 2.0f * PI) m_thrustPhase -= 2.0f * PI;

            float rumbleOsc = std::sin(m_thrustPhase);
            float noise = getNoise();
            // Blended noise with low rumble
            float thrustAudio = (rumbleOsc * 0.4f + noise * 0.6f) * (0.25f + 0.55f * throttle);
            sample += thrustAudio * throttle * 0.45f;
        }

        // 2. RCS puff
        if (rcsTime > 0.0f) {
            float rcsNoise = getNoise() * 0.18f;
            sample += rcsNoise;
            rcsTime -= dt; // [s]
        }

        // 3. Proximity / Warning Beep
        if (beepTime > 0.0f) {
            m_beepPhase += 2.0f * PI * beepFreq * dt; // [rad]
            if (m_beepPhase > 2.0f * PI) m_beepPhase -= 2.0f * PI;
            // Square wave for authentic arcade beep
            float tone = (m_beepPhase < PI) ? 0.22f : -0.22f;
            sample += tone;
            beepTime -= dt; // [s]
        }

        // 4. Impact crunch / thud sound
        if (impTime > 0.0f) {
            float env = impTime / 0.22f; // [0.0 - 1.0, dimensionless]
            float impNoise = getNoise();
            sample += impNoise * env * impIntensity * 0.55f;
            impTime -= dt; // [s]
        }

        // 5. Explosion sound
        if (expTime > 0.0f) {
            float env = expTime / 1.6f; // [0.0 - 1.0, dimensionless]
            float expNoise = getNoise();
            sample += expNoise * env * env * 0.6f;
            expTime -= dt; // [s]
        }

        // 6. Landing Fanfare (4 ascending tones: C5, E5, G5, C6)
        if (fanfareTime > 0.0f) {
            float progress = 1.0f - (fanfareTime / 1.2f); // [0.0 - 1.0, dimensionless]
            float fFreq = 523.25f; // [Hz] C5
            if (progress > 0.75f) fFreq = 1046.50f; // [Hz] C6
            else if (progress > 0.50f) fFreq = 783.99f; // [Hz] G5
            else if (progress > 0.25f) fFreq = 659.25f; // [Hz] E5

            m_fanfarePhase += 2.0f * PI * fFreq * dt; // [rad]
            if (m_fanfarePhase > 2.0f * PI) m_fanfarePhase -= 2.0f * PI;
            float note = std::sin(m_fanfarePhase) * 0.25f;
            sample += note * (fanfareTime / 1.2f);
            fanfareTime -= dt; // [s]
        }

        // 7. Coin Insert Chime (two-tone: B5 -> E6)
        if (coinTime > 0.0f) {
            float cFreq = (coinTime > 0.18f) ? 987.77f : 1318.51f; // [Hz]
            m_coinPhase += 2.0f * PI * cFreq * dt;                 // [rad]
            if (m_coinPhase > 2.0f * PI) m_coinPhase -= 2.0f * PI;
            sample += std::sin(m_coinPhase) * 0.28f;
            coinTime -= dt; // [s]
        }

        // 8. Impulse Thruster Boost blast (low-frequency punch + noisy rocket whoosh)
        if (boostTime > 0.0f) {
            float env = boostTime / 0.28f;
            float boostNoise = getNoise() * 0.45f;
            sample += boostNoise * env;
            boostTime -= dt; // [s]
        }

        // Clip to avoid distortion
        output[i] = std::clamp(sample, -1.0f, 1.0f);
    }

    m_rcsTimer.store(std::max(0.0f, rcsTime));
    m_impactTimer.store(std::max(0.0f, impTime));
    m_explosionTimer.store(std::max(0.0f, expTime));
    m_beepTimer.store(std::max(0.0f, beepTime));
    m_fanfareTimer.store(std::max(0.0f, fanfareTime));
    m_coinTimer.store(std::max(0.0f, coinTime));
    m_boostTimer.store(std::max(0.0f, boostTime));
}
