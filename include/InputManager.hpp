#pragma once

#include <SDL2/SDL.h>
#include <string>

struct InputState {
    float throttleLeft = 0.0f;   // [0.0 - 1.0, dimensionless] Analog Left Trigger or Q/Up/Space throttle fraction
    float throttleRight = 0.0f;  // [0.0 - 1.0, dimensionless] Analog Right Trigger or E/Up/Space throttle fraction
    float throttle = 0.0f;       // [0.0 - 1.0, dimensionless] Combined average throttle: (throttleLeft + throttleRight) * 0.5f
    float rotateAxis = 0.0f;     // [-1.0 to +1.0, dimensionless] Yaw rotation input (-1.0 left to +1.0 right)
    bool abortPressed = false;   // Emergency abort (B button or B key)
    bool boostPressed = false;   // Impulse Thruster boost (A button on controller or Left Shift on keyboard)
    bool restartPressed = false; // Restart game (A button or R key)
    bool manualRestartPressed = false; // Manual restart (R key specifically)
    bool addFuelPressed = false; // Insert coin / add fuel (X button or C/F key)
    bool nextSitePressed = false;// Switch mission (Y button or N key)
    bool pausePressed = false;   // Pause/resume (Start button or P key)
    bool toggleGlowPressed = false; // Toggle CRT glow (Back button or G key)
    bool toggleUnlimitedFuelPressed = false; // Toggle unlimited fuel (U key)
    bool toggleThrottleLockPressed = false;  // Toggle independent/locked throttle mode (T key / LS click)
    bool airBrakeHeld = false;               // Momentary air brake hold (Down/S key or D-Pad Down)
    bool toggleAirBrakesPressed = false;     // Toggle air brakes lock (K key or RS click)
    bool toggleHoverPressed = false;         // Toggle ground effect hover thrusters (H key or D-Pad Up)
    bool quitRequested = false;              // Quit game (ESC or window close)
};

class InputManager {
public:
    InputManager();
    ~InputManager();

    bool init();
    void shutdown();
    void update();

    const InputState& getState() const { return m_state; }
    void setStateForTesting(const InputState& state) { m_state = state; }
    bool isControllerConnected() const { return m_controller != nullptr; }
    std::string getControllerName() const;

    // Haptic rumble
    void setRumble(float lowFreq, float highFreq, uint32_t durationMs = 100); // lowFreq, highFreq in [0.0 - 1.0, dimensionless], durationMs in [ms]

private:
    void openController(int joystickIndex);
    void closeController();

    SDL_GameController* m_controller = nullptr;
    SDL_JoystickID m_joystickId = -1;
    InputState m_state;

    // Button edge detection
    bool m_prevAbortBtn = false;
    bool m_prevBoostBtn = false;
    bool m_prevRestartBtn = false;
    bool m_prevAddFuelBtn = false;
    bool m_prevNextSiteBtn = false;
    bool m_prevPauseBtn = false;
    bool m_prevGlowBtn = false;
    bool m_prevThrottleLockBtn = false;
    bool m_prevAirBrakeToggleBtn = false;
    bool m_prevHoverBtn = false;

    // Keyboard states
    bool m_keyUp = false;
    bool m_keyDown = false;
    bool m_keyLeft = false;
    bool m_keyRight = false;
    bool m_keySpace = false;
    bool m_keyQ = false;
    bool m_keyE = false;
    bool m_keyU = false;
    bool m_keyT = false;
    bool m_keyK = false;
    bool m_keyH = false;
    bool m_keyAbort = false;
    bool m_keyRestart = false;
    bool m_keyAddFuel = false;
    bool m_keyNextSite = false;
    bool m_keyPause = false;
    bool m_keyGlow = false;
    bool m_keyShift = false;

    bool m_prevKeyU = false;
    bool m_prevKeyT = false;
    bool m_prevKeyK = false;
    bool m_prevKeyH = false;
    bool m_prevKeyAbort = false;
    bool m_prevKeyRestart = false;
    bool m_prevKeyAddFuel = false;
    bool m_prevKeyNextSite = false;
    bool m_prevKeyPause = false;
    bool m_prevKeyGlow = false;
    bool m_prevKeyShift = false;
};
