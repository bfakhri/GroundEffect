# GroundEffect 3D — Vector Arcade Flight Simulator

An authentic, high-fidelity 3D recreation and evolution of the classic vector arcade flight game *GroundEffect*, written in modern C++ (C++17) with pure SDL2 (zero external 3D/OpenGL package dependencies), featuring **6-DOF 3D flight physics**, **3D wireframe battleship & lunar terrain heightfields**, **Xbox 360 controller** support with true proportional analog throttle, haptic rumble feedback, multi-mode 3D perspective camera (`CHASE 3D`, `ORBIT 3D`, `ISOMETRIC 3D`, `CLASSIC 2D`), vector stroke typography, 3D aerodynamic ground-effect physics, automatic ground-effect hover thrusters, air brakes, real-time 3D force vector telemetry, and procedural audio synthesis.

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

- **Air Brakes & Ground Effect Hover Thrusters**:
  - **Air Brakes (`S` / `Down Arrow` to hold, `K` / `RS Click` to toggle)**: Deploys articulated vector spoiler flaps on the port and starboard sponsons, applying linear + quadratic aerodynamic drag opposing velocity $(v_x, v_y)$ and damping rotational spin.
  - **Ground Effect Hover Thrusters (`H` / `D-Pad Up` to toggle)**: Automatically fires and modulates ventral ground-effect thrusters near the terrain surface to keep the battleship from striking the ground when descending at a slow speed. When left on, the vehicle hovers and smoothly skates across uneven terrain contours!

- **Xbox 360 Controller Support**:
  - **Left Trigger (`LT`) [Analog]**: Proportional 0% to 100% Left Engine Throttle.
  - **Right Trigger (`RT`) [Analog]**: Proportional 0% to 100% Right Engine Throttle.
  - **Left Stick (`LS`) & D-Pad Left/Right**: High-precision yaw rotation with deadzone filtering and attitude thruster puffs.
  - **D-Pad Up**: Toggle automatic **Ground Effect Hover Thrusters**.
  - **D-Pad Down / Right Stick Down / `RS` Click**: Hold or toggle **Air Brakes**.
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
| **Left Stick (`LS`) X / Y** | **3D Attitude Control (RCS Roll & Pitch)** |
| **Right Stick (`RS`) X / Y** | **3D Camera Orbit (Yaw & Elevation Pitch)** |
| **D-Pad Left / Right** | Digital Roll Left / Right |
| **D-Pad Up** | **TOGGLE HOVER THRUSTERS** (Auto-modulating 3D ground-effect hover & terrain skating) |
| **D-Pad Down / `RS` Click** | **AIR BRAKES** (Hold D-Pad Down, or click RS to lock) |
| **`LS` Click** | **TOGGLE THROTTLE LOCK** (Link both engines to single trigger) |
| **`B` Button** | **EMERGENCY ABORT** (Emergency 3D upright dual-retro burn) |
| **`X` Button** | **INSERT COIN** (+227 kg Fuel / +500 lb) |
| **`Y` Button** | **CHANGE MISSION** (Cycle 3D landing terrain site) |
| **`A` Button / Bumpers (`LB`/`RB`)** | **IMPULSE THRUSTER BOOST** (One-frame retro impulse boost to halt fall cold; launches next flight if crashed) |
| **`START` Button** | **PAUSE / RESUME** |
| **`BACK` Button** | **TOGGLE CRT GLOW / SCANLINES** |

### Keyboard Controls

| Key | Function |
|---|---|
| **Q / E** | Fire Left / Right Thruster Independently |
| **Up Arrow / Space** | Fire Both Main Thrusters (Balanced vertical thrust) |
| **W / S** | **3D Pitch Forward (+Z) / Pitch Aft (-Z)** |
| **Left / Right Arrows or A / D** | **3D Roll Left (-X) / Roll Right (+X)** |
| **J / L (or `[` / `]`) & I / O** | **3D Camera Orbit (Yaw & Elevation Pitch)** |
| **Tab / M** | **Cycle Camera View Mode** (`3D CHASE`, `3D ORBIT`, `3D ISOMETRIC`, `CLASSIC 2D`) |
| **Down Arrow** | **Hold Air Brakes** (Aerodynamic drag & 3D spin damping) |
| **K** | **Toggle Air Brakes Lock** |
| **H** | **Toggle Ground Effect Hover Thrusters** (Auto-hover & 3D terrain skating) |
| **V** | **Toggle 3D Force Vector Overlay** |
| **Left Shift / Right Shift** | **Impulse Thruster Boost** (One-frame retro impulse boost to halt fall cold) |
| **T** | **Toggle Throttle Lock Mode** (Link both engines to either trigger) |
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

### Running the Automated Test & Simulation Suite

```bash
make test
```

Or via CMake / CTest:
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

---

## Safe Landing Checklist

To successfully touch down on a landing pad and receive multiplier points:
1. **Pad Alignment**: Both footpads must make contact within the bounds of a designated horizontal pad (`2X`, `3X`, `4X`, or `5X`).
2. **Vertical Speed**: Must be less than **6.1 m/s** (20.0 ft/s; HUD turns green).
3. **Horizontal Speed**: Must be less than **4.3 m/s** (14.0 ft/s; HUD turns green).
4. **Battleship Attitude / Tilt**: Must be upright within **±7.0 degrees** of vertical.
