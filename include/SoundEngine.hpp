#pragma once

#include <SDL2/SDL.h>
#include <atomic>
#include <mutex>
#include <vector>

class SoundEngine {
public:
    SoundEngine();
    ~SoundEngine();

    bool init();
    void shutdown();

    // Sound triggers
    void setThrust(float throttle); // throttle in [0.0 - 1.0, dimensionless] (continuous engine sound)
    void playRCS();                 // Short attitude puff
    void playBeep(float freq = 880.0f, float duration = 0.08f); // freq in [Hz], duration in [s]
    void playImpact(float intensity = 1.0f); // intensity in [0.0 - 1.0, dimensionless] (collision crunch/thud)
    void playExplosion();
    void playLandFanfare();
    void playCoinSound();
    void playImpulseBoost();

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void generateAudio(float* output, int numSamples);

    SDL_AudioDeviceID m_devId = 0;
    bool m_initialized = false;

    // Synth states
    std::atomic<float> m_thrustThrottle{0.0f};  // [0.0 - 1.0, dimensionless] Current engine throttle
    std::atomic<float> m_rcsTimer{0.0f};        // [s] Remaining RCS puff audio duration
    std::atomic<float> m_impactTimer{0.0f};     // [s] Remaining impact sound duration
    std::atomic<float> m_impactIntensity{1.0f}; // [0.0 - 1.0, dimensionless] Impact volume scale
    std::atomic<float> m_explosionTimer{0.0f};  // [s] Explosion blast sound duration
    std::atomic<float> m_boostTimer{0.0f};      // [s] Impulse thruster boost audio duration
    std::atomic<float> m_beepTimer{0.0f};       // [s] Warning tone duration
    std::atomic<float> m_beepFreq{880.0f};      // [Hz] Warning tone frequency
    std::atomic<float> m_fanfareTimer{0.0f};    // [s] Landing fanfare sound duration
    std::atomic<float> m_coinTimer{0.0f};       // [s] Coin chime sound duration

    // Phase accumulators and RNG
    float m_thrustPhase = 0.0f;  // [rad] Phase accumulator for engine rumble
    float m_beepPhase = 0.0f;    // [rad] Phase accumulator for beeps
    float m_fanfarePhase = 0.0f; // [rad] Phase accumulator for fanfare
    float m_coinPhase = 0.0f;    // [rad] Phase accumulator for coin sound
    uint32_t m_rngState = 123456789; // [dimensionless] LCG PRNG state

    float getNoise();
};
