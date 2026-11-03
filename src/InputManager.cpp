#include "InputManager.hpp"
#include <iostream>
#include <cmath>
#include <algorithm>

const int STICK_DEADZONE = 5000;   // [raw counts, out of 32767, dimensionless]
const int TRIGGER_DEADZONE = 1000; // [raw counts, out of 32767, dimensionless]

InputManager::InputManager() {}

InputManager::~InputManager() {
    shutdown();
}

bool InputManager::init() {
    // Scan for existing connected game controllers
    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            openController(i);
            break;
        }
    }
    return true;
}

void InputManager::shutdown() {
    closeController();
}

void InputManager::openController(int joystickIndex) {
    if (m_controller) {
        closeController();
    }
    m_controller = SDL_GameControllerOpen(joystickIndex);
    if (m_controller) {
        SDL_Joystick* j = SDL_GameControllerGetJoystick(m_controller);
        m_joystickId = SDL_JoystickInstanceID(j);
        std::cout << "[Controller] Connected: " << SDL_GameControllerName(m_controller) << std::endl;
    }
}

void InputManager::closeController() {
    if (m_controller) {
        std::cout << "[Controller] Disconnected" << std::endl;
        SDL_GameControllerClose(m_controller);
        m_controller = nullptr;
        m_joystickId = -1;
    }
}

std::string InputManager::getControllerName() const {
    if (m_controller) {
        return SDL_GameControllerName(m_controller);
    }
    return "None (Keyboard Fallback)";
}

void InputManager::setRumble(float lowFreq, float highFreq, uint32_t durationMs) {
    if (m_controller) {
        Uint16 low = (Uint16)(std::clamp(lowFreq, 0.0f, 1.0f) * 65535.0f);
        Uint16 high = (Uint16)(std::clamp(highFreq, 0.0f, 1.0f) * 65535.0f);
        SDL_GameControllerRumble(m_controller, low, high, durationMs);
    }
}

void InputManager::update() {
    // Reset one-shot edge triggers
    m_state.abortPressed = false;
    m_state.boostPressed = false;
    m_state.restartPressed = false;
    m_state.manualRestartPressed = false;
    m_state.addFuelPressed = false;
    m_state.nextSitePressed = false;
    m_state.pausePressed = false;
    m_state.toggleGlowPressed = false;
    m_state.toggleUnlimitedFuelPressed = false;
    m_state.toggleThrottleLockPressed = false;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_state.quitRequested = true;
                break;

            case SDL_CONTROLLERDEVICEADDED:
                if (!m_controller) {
                    openController(event.cdevice.which);
                }
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
                if (m_controller && event.cdevice.which == m_joystickId) {
                    closeController();
                }
                break;

            case SDL_KEYDOWN:
                if (event.key.repeat == 0) {
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE: m_state.quitRequested = true; break;
                        case SDLK_UP:
                        case SDLK_w:      m_keyUp = true; break;
                        case SDLK_DOWN:
                        case SDLK_s:      m_keyDown = true; break;
                        case SDLK_LEFT:
                        case SDLK_a:      m_keyLeft = true; break;
                        case SDLK_RIGHT:
                        case SDLK_d:      m_keyRight = true; break;
                        case SDLK_SPACE:  m_keySpace = true; break;
                        case SDLK_q:      m_keyQ = true; break;
                        case SDLK_e:      m_keyE = true; break;
                        case SDLK_u:      m_keyU = true; break;
                        case SDLK_t:      m_keyT = true; break;
                        case SDLK_b:      m_keyAbort = true; break;
                        case SDLK_r:      m_keyRestart = true; break;
                        case SDLK_c:
                        case SDLK_f:      m_keyAddFuel = true; break;
                        case SDLK_n:      m_keyNextSite = true; break;
                        case SDLK_p:      m_keyPause = true; break;
                        case SDLK_g:      m_keyGlow = true; break;
                        case SDLK_LSHIFT:
                        case SDLK_RSHIFT: m_keyShift = true; break;
                    }
                }
                break;

            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                    case SDLK_w:      m_keyUp = false; break;
                    case SDLK_DOWN:
                    case SDLK_s:      m_keyDown = false; break;
                    case SDLK_LEFT:
                    case SDLK_a:      m_keyLeft = false; break;
                    case SDLK_RIGHT:
                    case SDLK_d:      m_keyRight = false; break;
                    case SDLK_SPACE:  m_keySpace = false; break;
                    case SDLK_q:      m_keyQ = false; break;
                    case SDLK_e:      m_keyE = false; break;
                    case SDLK_u:      m_keyU = false; break;
                    case SDLK_t:      m_keyT = false; break;
                    case SDLK_b:      m_keyAbort = false; break;
                    case SDLK_r:      m_keyRestart = false; break;
                    case SDLK_c:
                    case SDLK_f:      m_keyAddFuel = false; break;
                    case SDLK_n:      m_keyNextSite = false; break;
                    case SDLK_p:      m_keyPause = false; break;
                    case SDLK_g:      m_keyGlow = false; break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT: m_keyShift = false; break;
                }
                break;
        }
    }

    // Process Xbox 360 Controller State if connected
    float ctrlThrottleLeft = 0.0f;
    float ctrlThrottleRight = 0.0f;
    float ctrlRotate = 0.0f;
    bool btnAbort = false;
    bool btnBoost = false;
    bool btnRestart = false;
    bool btnAddFuel = false;
    bool btnNextSite = false;
    bool btnPause = false;
    bool btnGlow = false;
    bool btnThrottleLock = false;

    if (m_controller) {
        // Left Trigger: Left Thruster proportional throttle (0 to 32767)
        Sint16 ltValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        if (ltValue > TRIGGER_DEADZONE) {
            ctrlThrottleLeft = (float)(ltValue - TRIGGER_DEADZONE) / (32767.0f - TRIGGER_DEADZONE);
        }

        // Right Trigger: Right Thruster proportional throttle (0 to 32767)
        Sint16 rtValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (rtValue > TRIGGER_DEADZONE) {
            ctrlThrottleRight = (float)(rtValue - TRIGGER_DEADZONE) / (32767.0f - TRIGGER_DEADZONE);
        }

        // 'A' Button or Bumpers: Impulse Thruster boost (A also acts as restart after crash)
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A) ||
            SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER) ||
            SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            btnBoost = true;
        }
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A)) {
            btnRestart = true;
        }

        // Left Thumbstick X: Rotation (-32768 to 32767)
        Sint16 lxValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
        if (std::abs(lxValue) > STICK_DEADZONE) {
            float sign = (lxValue > 0) ? 1.0f : -1.0f;
            ctrlRotate = sign * (float)(std::abs(lxValue) - STICK_DEADZONE) / (32767.0f - STICK_DEADZONE);
        }

        // D-Pad override for crisp digital rotation
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT) ||
            SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
            ctrlRotate = -1.0f;
        } else if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) ||
                   SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            ctrlRotate = 1.0f;
        }

        // Face Buttons:
        // 'B' = ABORT
        btnAbort = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B) != 0;
        // 'X' = Insert Coin / Add Fuel
        btnAddFuel = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_X) != 0;
        // 'Y' = Next Site / Mission
        btnNextSite = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_Y) != 0;
        // Start = Pause
        btnPause = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START) != 0;
        // Back = Toggle Glow / CRT
        btnGlow = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_BACK) != 0;
        // Left Stick Click = Toggle throttle lock mode
        btnThrottleLock = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSTICK) != 0;
    }

    // Blend with Keyboard
    float kbThrottleLeft = 0.0f;
    float kbThrottleRight = 0.0f;

    // Up / W / Space fires both thrusters together
    if (m_keyUp || m_keySpace) {
        kbThrottleLeft = std::max(kbThrottleLeft, 1.0f);
        kbThrottleRight = std::max(kbThrottleRight, 1.0f);
    }
    // Q controls Left Thruster, E controls Right Thruster
    if (m_keyQ) {
        kbThrottleLeft = std::max(kbThrottleLeft, 1.0f);
    }
    if (m_keyE) {
        kbThrottleRight = std::max(kbThrottleRight, 1.0f);
    }

    float kbRotate = 0.0f;
    if (m_keyLeft) kbRotate -= 1.0f;
    if (m_keyRight) kbRotate += 1.0f;

    m_state.throttleLeft = std::clamp(std::max(ctrlThrottleLeft, kbThrottleLeft), 0.0f, 1.0f);
    m_state.throttleRight = std::clamp(std::max(ctrlThrottleRight, kbThrottleRight), 0.0f, 1.0f);
    m_state.throttle = (m_state.throttleLeft + m_state.throttleRight) * 0.5f;
    m_state.rotateAxis = std::clamp(ctrlRotate + kbRotate, -1.0f, 1.0f);

    // Edge triggers (press on rising edge)
    if ((btnBoost && !m_prevBoostBtn) || (m_keyShift && !m_prevKeyShift)) {
        m_state.boostPressed = true;
    }
    if ((btnAbort && !m_prevAbortBtn) || (m_keyAbort && !m_prevKeyAbort)) {
        m_state.abortPressed = true;
    }
    if ((btnRestart && !m_prevRestartBtn) || (m_keyRestart && !m_prevKeyRestart)) {
        m_state.restartPressed = true;
    }
    if (m_keyRestart && !m_prevKeyRestart) {
        m_state.manualRestartPressed = true;
    }
    if ((btnAddFuel && !m_prevAddFuelBtn) || (m_keyAddFuel && !m_prevKeyAddFuel)) {
        m_state.addFuelPressed = true;
    }
    if ((btnNextSite && !m_prevNextSiteBtn) || (m_keyNextSite && !m_prevKeyNextSite)) {
        m_state.nextSitePressed = true;
    }
    if ((btnPause && !m_prevPauseBtn) || (m_keyPause && !m_prevKeyPause)) {
        m_state.pausePressed = true;
    }
    if ((btnGlow && !m_prevGlowBtn) || (m_keyGlow && !m_prevKeyGlow)) {
        m_state.toggleGlowPressed = true;
    }
    if (m_keyU && !m_prevKeyU) {
        m_state.toggleUnlimitedFuelPressed = true;
    }
    if ((btnThrottleLock && !m_prevThrottleLockBtn) || (m_keyT && !m_prevKeyT)) {
        m_state.toggleThrottleLockPressed = true;
    }

    m_prevAbortBtn = btnAbort;
    m_prevBoostBtn = btnBoost;
    m_prevRestartBtn = btnRestart;
    m_prevAddFuelBtn = btnAddFuel;
    m_prevNextSiteBtn = btnNextSite;
    m_prevPauseBtn = btnPause;
    m_prevGlowBtn = btnGlow;
    m_prevThrottleLockBtn = btnThrottleLock;

    m_prevKeyU = m_keyU;
    m_prevKeyT = m_keyT;
    m_prevKeyAbort = m_keyAbort;
    m_prevKeyRestart = m_keyRestart;
    m_prevKeyAddFuel = m_keyAddFuel;
    m_prevKeyNextSite = m_keyNextSite;
    m_prevKeyPause = m_keyPause;
    m_prevKeyGlow = m_keyGlow;
    m_prevKeyShift = m_keyShift;
}
