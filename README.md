# GroundEffect — Vector Arcade Flight Simulator

An authentic, high-fidelity recreation and evolution of the classic vector arcade flight game *GroundEffect*, written in modern C++ (C++17) with SDL2, featuring **Xbox 360 controller** support with true proportional analog throttle, haptic rumble feedback, dynamic zoom camera, vector stroke typography, aerodynamic ground-effect physics, and real-time procedural audio synthesis.

---

## Key Features

- **Twin Main Thrusters with Differential Steering**:
  - The craft is equipped with **two independent main thrusters** laterally displaced on either side of the descent stage.
  - Squeezing the **Left Trigger (`LT`)** ignites the left engine, pushing the left side up and causing the battleship to tilt and steer to the right.
  - Squeezing the **Right Trigger (`RT`)** ignites the right engine, pushing the right side up and causing the battleship to tilt and steer to the left.
  - Squeezing **both triggers simultaneously** delivers balanced vertical lift.
  - **Slightly Exponential Thrust Curve ($t^{1.6}$)**: Provides fine, delicate feathering control at low throttle for soft hover adjustments and touchdown precision, while smoothly ramping up to full engine punch at deep trigger pulls.
  - Pilots can steer using pure differential engine thrust alone or in combination with the attitude reaction control system (RCS)!
  - **Unlimited Fuel Testing Mode**: Enabled by default for convenient development and testing; toggle anytime on the fly.
  - **Velocity-Proportional Damage & Rebound System**: Instead of breaking on first contact, rough touchdowns and terrain impacts bounce the battleship off the surface and accrue structural damage proportional to the impact velocity ($v_{impact} \times 0.75\%$). The vehicle only breaks apart when cumulative damage reaches 100%!
  - **Impact Haptic Vibration**: Controller rumbles with intensity and duration scaled directly to impact speed on every collision, accompanied by a procedural metallic crunch sound and visual hull flash.

- **Xbox 360 Controller Support**:
  - **Left Trigger (`LT`) [Analog]**: Proportional 0% to 100% Left Engine Throttle.
  - **Right Trigger (`RT`) [Analog]**: Proportional 0% to 100% Right Engine Throttle.
  - **Left Stick (`LS`) & D-Pad**: High-precision yaw rotation with deadzone filtering and attitude thruster puffs.
  - **`B` Button (ABORT)**: Emergency abort system! Fires retro rockets vertically, uprights the battleship, and burns fuel to escape an imminent crash.
  - **`X` Button (INSERT COIN / +227 KG FUEL)**: Recreates the original arcade mechanic allowing pilots to pump in more propellant mid-flight (+226.8 kg / +500 lb).
  - **`Y` Button (CHANGE MISSION SITE)**: Cycles through 4 terrain zones (Tranquility Base, Tycho Crater, Ocean of Storms, Hadley Rille).
  - **`A` Button (RESTART / BOOST)**: Restarts flight after touchdown or crash (or provides simultaneous dual-engine thrust).
  - **`START` Button**: Pause and unpause.
  - **`BACK` Button**: Toggle CRT Phosphor vector glow and scanline effect.
  - **Dual Haptic Rumble**: Independent motor feedback for dual throttles plus violent velocity-proportional impact shock upon collision!

- **Vector Display & Aesthetics**:
  - Wireframe vector craft with octagonal main body, cockpit windows, RCS thruster quads, **twin engine nozzles**, and 4 landing gear legs with dish footpads.
  - Dual-mode dynamic camera: Automatically scales from wide strategic horizon view down into a high-magnification close-up view as the craft nears the terrain surface.
  - Dual dynamic vector flame plumes with throttle-proportional length and independent flicker phases.
  - Dynamic visual damage feedback: Flash of orange/red phosphor upon impact and damaged hull flicker when structural integrity drops below 35%.
  - Breakup crash physics: Craft shatters into tumbling vector debris upon reaching 100% damage.
  - Built-in 1979 stroke-based vector typography (no external TTF files required).

- **1979 Arcade Telemetry & Rules**:
  - Real-time HUD displaying `SCORE`, `TIME`, `HULL` (live structural integrity percentage), `FUEL` (shows `FUEL UNLTD` when unlimited fuel is enabled), `ALTITUDE`, `HORIZONTAL SPEED` (with `<` / `>` direction indicators), `VERTICAL SPEED` (with `^` / `v` direction indicators), `TILT ANGLE`, and dual `L.ENG [LT]` / `R.ENG [RT]` throttle meters.
  - Color-coded safety alerts: Green for safe landing parameters; flashing red when speed or tilt exceeds safe structural tolerances.
  - Multiplier landing sites: 2X, 3X, 4X, and 5X pads located on ledges, crater bottoms, and mountain pinnacles.

- **Real-Time Procedural Audio Synthesizer**:
  - Continuous engine roar with rumble frequency and noise spectrum proportional to thruster fire.
  - Procedural metallic crunch/thud audio on collision impacts.
  - RCS attitude puff sounds.
  - Low-altitude descent warning alarm beeps.
  - White-noise explosive crash blast.
  - Ascending 4-tone landing fanfare.
  - Dual-tone arcade coin chime.

---

## Xbox 360 Controller Mapping

| Xbox 360 Input | Action / Arcade Function |
|---|---|
| **Left Trigger (`LT`)** | **Left Main Thruster** (Continuous analog 0% - 100%, induces clockwise tilt) |
| **Right Trigger (`RT`)** | **Right Main Thruster** (Continuous analog 0% - 100%, induces counter-clockwise tilt) |
| **Left Stick (`LS`) X-Axis** | **Attitude Control (RCS)** (Smooth analog yaw steering) |
| **D-Pad Left / Right** | Digital Rotation Left / Right (Classic arcade feel) |
| **`B` Button** | **EMERGENCY ABORT** (Emergency upright dual-retro burn) |
| **`X` Button** | **INSERT COIN** (+227 kg Fuel / +500 lb) |
| **`Y` Button** | **CHANGE MISSION** (Cycle landing terrain site) |
| **`A` Button / Bumpers (`LB`/`RB`)** | **IMPULSE THRUSTER BOOST** (One-frame retro impulse boost to halt fall cold; launches next flight if crashed) |
| **`START` Button** | **PAUSE / RESUME** |
| **`BACK` Button** | **TOGGLE CRT GLOW / SCANLINES** |

### Keyboard Controls

| Key | Function |
|---|---|
| **Q** | Fire Left Thruster (Induces rightward tilt) |
| **E** | Fire Right Thruster (Induces leftward tilt) |
| **Up Arrow / W / Space** | Fire Both Main Thrusters (Balanced vertical thrust) |
| **Left Shift / Right Shift** | **Impulse Thruster Boost** (One-frame retro impulse boost to halt fall cold) |
| **Left / Right Arrows or A / D** | Attitude Thrusters (RCS Yaw) |
| **U** | **Toggle Unlimited Fuel** (Testing mode) |
| **B** | Emergency Abort |
| **C / F** | Insert Coin / +227 kg Fuel (+500 lb) |
| **N** | Next Mission / Terrain Site |
| **R** | Restart Flight |
| **P** | Pause / Resume |
| **G** | Toggle CRT Glow & Scanlines |
| **Escape** | Exit Game |

---

## Building and Running

### Prerequisites

- A C++17 compiler (`g++` or `clang++`)
- SDL2 (`libsdl2-dev`)

On Ubuntu / Debian:
```bash
sudo apt-get install libsdl2-dev g++ make
```

### Compiling with Make

```bash
cd ~/Projects/GroundEffect
make -j$(nproc)
./GroundEffect
```

### Compiling with CMake (Alternative)

```bash
cd ~/Projects/GroundEffect
mkdir -p build && cd build
cmake ..
cmake --build .
./GroundEffect
```

---

## Safe Landing Checklist

To successfully touch down on a landing pad and receive multiplier points:
1. **Pad Alignment**: Both footpads must make contact within the bounds of a designated horizontal pad (`2X`, `3X`, `4X`, or `5X`).
2. **Vertical Speed**: Must be less than **6.1 m/s** (20.0 ft/s; HUD turns green).
3. **Horizontal Speed**: Must be less than **4.3 m/s** (14.0 ft/s; HUD turns green).
4. **Battleship Attitude / Tilt**: Must be upright within **±7.0 degrees** of vertical.
