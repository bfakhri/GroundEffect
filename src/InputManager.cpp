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
    m_state.toggleAirBrakesPressed = false;
    m_state.toggleHoverPressed = false;
    m_state.toggleForceVectorsPressed = false;
    m_state.cycleCameraPressed = false;

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
                        case SDLK_UP:     m_keyUp = true; break;
                        case SDLK_DOWN:   m_keyDown = true; break;
                        case SDLK_w:      m_keyW = true; break;
                        case SDLK_s:      m_keyS = true; break;
                        case SDLK_LEFT:
                        case SDLK_a:      m_keyLeft = true; break;
                        case SDLK_RIGHT:
                        case SDLK_d:      m_keyRight = true; break;
                        case SDLK_j:
                        case SDLK_LEFTBRACKET:  m_keyJ = true; break;
                        case SDLK_l:
                        case SDLK_RIGHTBRACKET: m_keyL = true; break;
                        case SDLK_i:      m_keyI = true; break;
                        case SDLK_o:      m_keyO = true; break;
                        case SDLK_z:      m_keyZ = true; break;
                        case SDLK_x:      m_keyX = true; break;
                        case SDLK_SPACE:  m_keySpace = true; break;
                        case SDLK_q:      m_keyQ = true; break;
                        case SDLK_e:      m_keyE = true; break;
                        case SDLK_u:      m_keyU = true; break;
                        case SDLK_t:      m_keyT = true; break;
                        case SDLK_k:      m_keyK = true; break;
                        case SDLK_h:      m_keyH = true; break;
                        case SDLK_v:      m_keyV = true; break;
                        case SDLK_TAB:
                        case SDLK_m:      m_keyM = true; break;
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
                    case SDLK_UP:     m_keyUp = false; break;
                    case SDLK_DOWN:   m_keyDown = false; break;
                    case SDLK_w:      m_keyW = false; break;
                    case SDLK_s:      m_keyS = false; break;
                    case SDLK_LEFT:
                    case SDLK_a:      m_keyLeft = false; break;
                    case SDLK_RIGHT:
                    case SDLK_d:      m_keyRight = false; break;
                    case SDLK_j:
                    case SDLK_LEFTBRACKET:  m_keyJ = false; break;
                    case SDLK_l:
                    case SDLK_RIGHTBRACKET: m_keyL = false; break;
                    case SDLK_i:      m_keyI = false; break;
                    case SDLK_o:      m_keyO = false; break;
                    case SDLK_z:      m_keyZ = false; break;
                    case SDLK_x:      m_keyX = false; break;
                    case SDLK_SPACE:  m_keySpace = false; break;
                    case SDLK_q:      m_keyQ = false; break;
                    case SDLK_e:      m_keyE = false; break;
                    case SDLK_u:      m_keyU = false; break;
                    case SDLK_t:      m_keyT = false; break;
                    case SDLK_k:      m_keyK = false; break;
                    case SDLK_h:      m_keyH = false; break;
                    case SDLK_v:      m_keyV = false; break;
                    case SDLK_TAB:
                    case SDLK_m:      m_keyM = false; break;
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
    float ctrlPitch = 0.0f;
    float ctrlYaw = 0.0f;
    float ctrlCamYaw = 0.0f;
    float ctrlCamPitch = 0.0f;
    bool btnAbort = false;
    bool btnBoost = false;
    bool btnRestart = false;
    bool btnAddFuel = false;
    bool btnNextSite = false;
    bool btnPause = false;
    bool btnGlow = false;
    bool btnThrottleLock = false;
    bool btnAirBrakeHeld = false;
    bool btnAirBrakeToggle = false;
    bool btnHoverToggle = false;

    if (m_controller) {
        Sint16 ltValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
        if (ltValue > TRIGGER_DEADZONE) {
            ctrlThrottleLeft = (float)(ltValue - TRIGGER_DEADZONE) / (32767.0f - TRIGGER_DEADZONE);
        }

        Sint16 rtValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        if (rtValue > TRIGGER_DEADZONE) {
            ctrlThrottleRight = (float)(rtValue - TRIGGER_DEADZONE) / (32767.0f - TRIGGER_DEADZONE);
        }

        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_A)) {
            btnBoost = true;
            btnRestart = true;
        }

        // Left Thumbstick X: Roll Rotation (-32768 to 32767)
        Sint16 lxValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTX);
        if (std::abs(lxValue) > STICK_DEADZONE) {
            float sign = (lxValue > 0) ? 1.0f : -1.0f;
            ctrlRotate = sign * (float)(std::abs(lxValue) - STICK_DEADZONE) / (32767.0f - STICK_DEADZONE);
        }

        // Left Thumbstick Y: 3D Pitch Forward/Backward (pushing stick up = -Y raw = pitch forward +Z)
        Sint16 lyValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (std::abs(lyValue) > STICK_DEADZONE) {
            float sign = (lyValue < 0) ? 1.0f : -1.0f;
            ctrlPitch = sign * (float)(std::abs(lyValue) - STICK_DEADZONE) / (32767.0f - STICK_DEADZONE);
        }

        // Right Thumbstick X/Y: 3D Camera Orbit Yaw & Elevation Pitch
        Sint16 rxValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTX);
        if (std::abs(rxValue) > STICK_DEADZONE) {
            float sign = (rxValue > 0) ? 1.0f : -1.0f;
            ctrlCamYaw = sign * (float)(std::abs(rxValue) - STICK_DEADZONE) / (32767.0f - STICK_DEADZONE);
        }
        Sint16 ryValue = SDL_GameControllerGetAxis(m_controller, SDL_CONTROLLER_AXIS_RIGHTY);
        if (std::abs(ryValue) > STICK_DEADZONE) {
            float sign = (ryValue < 0) ? 1.0f : -1.0f;
            ctrlCamPitch = sign * (float)(std::abs(ryValue) - STICK_DEADZONE) / (32767.0f - STICK_DEADZONE);
        }

        // D-Pad override for crisp digital roll rotation
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) {
            ctrlRotate = -1.0f;
        } else if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) {
            ctrlRotate = 1.0f;
        }

        // Shoulder Bumpers (LB / RB) for rudder yaw control
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
            ctrlYaw -= 1.0f;
        }
        if (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            ctrlYaw += 1.0f;
        }

        btnAirBrakeHeld = (SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) != 0);
        btnAirBrakeToggle = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_RIGHTSTICK) != 0;
        btnHoverToggle = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_DPAD_UP) != 0;

        btnAbort = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_B) != 0;
        btnAddFuel = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_X) != 0;
        btnNextSite = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_Y) != 0;
        btnPause = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_START) != 0;
        btnGlow = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_BACK) != 0;
        btnThrottleLock = SDL_GameControllerGetButton(m_controller, SDL_CONTROLLER_BUTTON_LEFTSTICK) != 0;
    }

    // Blend with Keyboard
    float kbThrottleLeft = 0.0f;
    float kbThrottleRight = 0.0f;

    // Up Arrow / Space fires both thrusters together
    if (m_keyUp || m_keySpace) {
        kbThrottleLeft = std::max(kbThrottleLeft, 1.0f);
        kbThrottleRight = std::max(kbThrottleRight, 1.0f);
    }
    if (m_keyQ) {
        kbThrottleLeft = std::max(kbThrottleLeft, 1.0f);
    }
    if (m_keyE) {
        kbThrottleRight = std::max(kbThrottleRight, 1.0f);
    }

    float kbRotate = 0.0f;
    if (m_keyLeft)  kbRotate -= 1.0f;
    if (m_keyRight) kbRotate += 1.0f;

    // W / S controls 3D Pitch Forward (+Z) / Aft (-Z)
    float kbPitch = 0.0f;
    if (m_keyW) kbPitch += 1.0f;
    if (m_keyS) kbPitch -= 1.0f;

    // Z / X controls 3D Rudder Yaw Left (-1) / Right (+1)
    float kbYaw = 0.0f;
    if (m_keyZ) kbYaw -= 1.0f;
    if (m_keyX) kbYaw += 1.0f;

    // J / L and I / O control 3D Camera Orbit
    float kbCamYaw = 0.0f;
    if (m_keyJ) kbCamYaw -= 1.0f;
    if (m_keyL) kbCamYaw += 1.0f;
    float kbCamPitch = 0.0f;
    if (m_keyI) kbCamPitch += 1.0f;
    if (m_keyO) kbCamPitch -= 1.0f;

    m_state.throttleLeft  = std::clamp(std::max(ctrlThrottleLeft, kbThrottleLeft), 0.0f, 1.0f);
    m_state.throttleRight = std::clamp(std::max(ctrlThrottleRight, kbThrottleRight), 0.0f, 1.0f);
    m_state.throttle      = (m_state.throttleLeft + m_state.throttleRight) * 0.5f;
    m_state.rotateAxis    = std::clamp(ctrlRotate + kbRotate, -1.0f, 1.0f);
    m_state.pitchAxis     = std::clamp(ctrlPitch + kbPitch, -1.0f, 1.0f);
    m_state.yawAxis       = std::clamp(ctrlYaw + kbYaw, -1.0f, 1.0f);
    m_state.camYawAxis    = std::clamp(ctrlCamYaw + kbCamYaw, -1.0f, 1.0f);
    m_state.camPitchAxis  = std::clamp(ctrlCamPitch + kbCamPitch, -1.0f, 1.0f);
    m_state.airBrakeHeld  = m_keyDown || btnAirBrakeHeld;

    // Edge triggers
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
    if ((btnAirBrakeToggle && !m_prevAirBrakeToggleBtn) || (m_keyK && !m_prevKeyK)) {
        m_state.toggleAirBrakesPressed = true;
    }
    if ((btnHoverToggle && !m_prevHoverBtn) || (m_keyH && !m_prevKeyH)) {
        m_state.toggleHoverPressed = true;
    }
    if (m_keyV && !m_prevKeyV) {
        m_state.toggleForceVectorsPressed = true;
    }
    if (m_keyM && !m_prevKeyM) {
        m_state.cycleCameraPressed = true;
    }

    m_prevAbortBtn = btnAbort;
    m_prevBoostBtn = btnBoost;
    m_prevRestartBtn = btnRestart;
    m_prevAddFuelBtn = btnAddFuel;
    m_prevNextSiteBtn = btnNextSite;
    m_prevPauseBtn = btnPause;
    m_prevGlowBtn = btnGlow;
    m_prevThrottleLockBtn = btnThrottleLock;
    m_prevAirBrakeToggleBtn = btnAirBrakeToggle;
    m_prevHoverBtn = btnHoverToggle;

    m_prevKeyU = m_keyU;
    m_prevKeyT = m_keyT;
    m_prevKeyK = m_keyK;
    m_prevKeyH = m_keyH;
    m_prevKeyV = m_keyV;
    m_prevKeyM = m_keyM;
    m_prevKeyAbort = m_keyAbort;
    m_prevKeyRestart = m_keyRestart;
    m_prevKeyAddFuel = m_keyAddFuel;
    m_prevKeyNextSite = m_keyNextSite;
    m_prevKeyPause = m_keyPause;
    m_prevKeyGlow = m_keyGlow;
    m_prevKeyShift = m_keyShift;
}
