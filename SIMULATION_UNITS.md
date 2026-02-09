# Simulation Units and Measurements Reference

This document defines all units, scales, and physical measurements used in the simulation. Use this as a reference when tuning physics, adding new features, or debugging.

---

## Time

- **Frame rate**: 60 FPS
- **Frame time**: `dt_frame = 1/60 s ≈ 0.0167 s` (16.67 ms per frame)
- **Time units**: Seconds (s) in solver, frames in game logic

---

## Space / Geometry

### Map Dimensions

From `SimulationConfig.h`:
- **Air grid**: `XCELLS = 153`, `YCELLS = 96` cells
- **Total air cells**: `NCELL = 153 × 96 = 14,688` cells
- **Pixel resolution**: `XRES = 612`, `YRES = 384` pixels
  - `XRES = XCELLS × CELL = 153 × 4 = 612`
  - `YRES = YCELLS × CELL = 96 × 4 = 384`
- **Total pixels**: `NPART = 612 × 384 = 234,048` pixels

**Map size** (assuming 1 pixel = 0.01 m = 1 cm):
- **Width**: `612 pixels = 6.12 m = 612 cm`
- **Height**: `384 pixels = 3.84 m = 384 cm`
- **Total area**: `234,048 pixels² = 23.4048 m² ≈ 23.5 m²` = **234,048 cm²** (1 pixel = 1 cm → 1 px² = 1 cm²)
- **Aspect ratio**: `612:384 = 1.6:1` (16:10)

**Air grid size**:
- **Width**: `153 cells × 0.04 m/cell = 6.12 m` (matches pixel width)
- **Height**: `96 cells × 0.04 m/cell = 3.84 m` (matches pixel height)
- **Total air volume**: `14,688 cells × 1.6 L/cell = 23,500 L = 23.5 m³` (at unit depth)

### Pixel and Cell Sizes

- **1 pixel**: Base unit of space
- **CELL**: `CELL = 4` pixels (air cell size)
- **Air cell**: `4×4 pixels = 16 pixels`

### Air Solver Cell Size

From `Air.cpp` (`update_air`):
- **Cell size**: `cell_size_m = CELL * 0.01 = 4 * 0.01 = 0.04 m = 4 cm`
- **Air cell volume**: `V_cell = (cell_size_m)² = (0.04 m)² = 1.6×10⁻³ m³ = 1.6 L`

### Volume per Pixel

From air cell volume:
- **Air cell**: 16 pixels = 1.6 L = 1.6×10⁻³ m³
- **Volume per pixel**: `V_pixel = 1.6 L / 16 = 0.1 L = 100 mL = 1.0×10⁻⁴ m³`

**Note**: This assumes 2D simulation (per unit depth). The air solver treats cells as 2D (area), so volume is per unit depth.

### Pixel Dimensions (if cubic)

If we assume 1 pixel represents a cubic volume:
- `V_pixel = 1.0×10⁻⁴ m³`
- Side length: `∛(1.0×10⁻⁴) ≈ 0.0464 m ≈ 4.64 cm ≈ 46.4 mm`

But air solver uses `cell_size_m = 0.04 m = 4 cm`, so:
- **1 pixel ≈ 1 cm** (if CELL = 4 pixels = 4 cm)

---

## Air / Atmosphere

### Pressure → Mass (Air Solver)

From ideal gas law: `ρ = p/(R·T)` where:
- `R = 287 J/(kg·K)` (gas constant for air)
- `T ≈ 300 K` (ambient temperature)

**Mass per 4×4 air cell** (at 300 K):

| Pressure | Mass per cell | Notes |
|----------|---------------|-------|
| 101 kPa (1 atm) | ~1.9 g | Standard atmosphere |
| 10 kPa (0.1 atm) | ~0.19 g | Low pressure |
| 1 kPa | ~19 mg | Very low pressure |
| 0.1 kPa | ~1.9 mg | Near vacuum |
| Vacuum floor (~1e-6 atm ≈ 0.1 Pa) | ~1.6 µg | Solver minimum |

**Calculation**: `m = ρ·V_cell = (p/(R·T)) · 1.6×10⁻³ m³`

### Air Heat Capacity

- **Specific heat (constant volume)**: `c_v = 717.5 J/(kg·K)` (from solver: `R/(γ-1)`)
- **Specific heat (constant pressure)**: `c_p = 1004.5 J/(kg·K)` (from solver: `γ·R/(γ-1)`)
- **Heat capacity per cell**: `C_air = ρ·c_v·V_cell` (J/K)
  - At 1 atm, 300 K: `C_air ≈ 1.9 g × 717.5 J/(kg·K) ≈ 1.36 J/K`

### Air Thermal Conductivity

- `k_air = 0.026 W/(m·K)` (at 300 K)

---

## Particles

### Mass (Weight Property)

**Status**: `Weight` property exists but is currently **not physically meaningful**. It's used for some gravity/physics but doesn't represent actual mass.

**Future**: Should represent **mass per particle** in **kg** or **g**.

**Current usage**: Used in some element interactions, but not for:
- Heat capacity calculations (we use per-particle `heatCapacity` in J/K)
- Momentum/force calculations (particles use velocity directly)

### Heat Capacity

**Units**: **J/K (joules per kelvin) per particle**

**Defaults**:
- **Gas particles**: `DEFAULT_GAS_HEAT_CAPACITY_J_PER_K = 0.001 J/K` (1 mJ/K)
- **Solid/Liquid particles**: `DEFAULT_SOLID_LIQUID_HEAT_CAPACITY_J_PER_K = 0.5 J/K`

**Element-specific** (examples):
- **TTAN (Titanium)**: `0.52 J/K` (based on ~520 J/(kg·K) for ~1 g equivalent)
- **IRON**: `0.45 J/K` (based on ~450 J/(kg·K) for ~1 g equivalent)

**Note**: These assume ~1 g per particle. **Long-term**: we will store **specific heat** (J/(kg·K)) and compute `C = m × c` once mass is defined; see **Design: Heat capacity vs specific heat**.

### Particle Volume

- **Volume per particle**: `V_particle = 0.1 L = 1.0×10⁻⁴ m³` (same as pixel volume)
- **Mass per particle** (if we assume density):
  - Water (1000 kg/m³): `m = 1000 × 1.0×10⁻⁴ = 0.1 kg = 100 g`
  - Iron (~7870 kg/m³): `m = 7870 × 1.0×10⁻⁴ = 0.787 kg = 787 g`
  - Air (1.2 kg/m³ at 1 atm): `m = 1.2 × 1.0×10⁻⁴ = 0.00012 kg = 0.12 g`

**But**: Current `Weight` values don't match these. Need to define what `Weight` actually represents.

---

## Measures at a Glance (gravity, pressure, velocity, heat)

Quick reference for tuning and debugging.

### Gravity

| Where | Units | Storage | Conversion / typical |
|-------|--------|---------|------------------------|
| Game | pixels/frame² | `pGravX`, `pGravY` | `a_game` (e.g. 0.25) |
| Solver | m/s² | `gx`, `gy` | `a_mps2 = a_game × 144` |
| Earth ref | m/s² | — | 9.8 m/s²; sim default ≈ 36 m/s² (0.25 px/f²) |

- **Conversion**: `a_mps2 = a_game × (0.04 / (1/60)²) = a_game × 144`.

### Pressure

| Where | Units | Storage | Conversion / typical |
|-------|--------|---------|------------------------|
| Solver / air | Pa | `pv[y][x]` | 1 atm = 101325 Pa |
| Reference | atm | — | 1 atm, 0.1 atm, 1 kPa, vacuum floor ~0.1 Pa |

- **To density**: `ρ = p/(R·T)` with `R = 287 J/(kg·K)`.

### Velocity

| Where | Units | Storage | Conversion / typical |
|-------|--------|---------|------------------------|
| Game (particles) | pixels/frame | `vx`, `vy` | `v_mps = v_game × 0.06` |
| Solver (air) | m/s | `ux`, `uy` | 1 m/s = 1/0.06 ≈ 16.7 px/frame |

- **Conversion**: `v_mps = v_game × game_vel_scale`, `game_vel_scale = 0.06`.

### Heat / temperature

| Quantity | Units | Storage | Conversion / typical |
|----------|--------|---------|------------------------|
| Temperature | K | `parts[i].temp`, `hv[y][x]` | Ambient ~295 K (22°C), range 0–99990 K |
| Heat capacity (particle) | J/K | `parts[i].heatCapacity` | Gas default 0.001, solid/liquid 0.5 |
| Heat capacity (air cell) | J/K | derived | `C_air = ρ·c_v·V_cell`, c_v = 717.5 J/(kg·K) |
| Thermal conductivity | W/(m·K) | `HeatConduct` 0–255 | `k = (HeatConduct/255) × 400` |
| Heat transfer rate | W | derived | `Q = k·A·ΔT/dx`; per frame: `Q×dt_frame` (J) |

---

## Velocity

### Game Units

- **Storage**: `vx`, `vy` in **pixels per frame**
- **Conversion**: `v_game = v_mps / game_vel_scale` where `game_vel_scale = 0.06`
- **Meaning**: `1 pixel/frame = 0.06 m/s` at 60 FPS

### Solver Units

- **Storage**: `ux`, `uy` in **m/s** (meters per second)
- **Conversion**: `v_mps = v_game * game_vel_scale`

### Example

- `vx = 10 pixels/frame` → `ux = 10 × 0.06 = 0.6 m/s`
- `ux = 10 m/s` → `vx = 10 / 0.06 ≈ 167 pixels/frame`

---

## Gravity

### Game Units

- **Storage**: `pGravX`, `pGravY` in **pixels per frame²** (acceleration)
- **Conversion**: `a_mps2 = a_game * (dx_m / dt_frame²)` where:
  - `dx_m = 0.04 m` (cell size)
  - `dt_frame = 1/60 s`
  - So: `a_mps2 = a_game * (0.04 / (1/60)²) = a_game * 144`

### Solver Units

- **Storage**: `gx`, `gy` in **m/s²** (meters per second squared)

### Example

- Standard gravity: `a_game = 0.25 pixels/frame²` → `a_mps2 = 0.25 × 144 = 36 m/s²` (≈ 3.7× Earth's 9.8 m/s²)

---

## Temperature

- **Units**: **Kelvin (K)**
- **Storage**: `parts[i].temp` and `hv[y][x]` (air temperature)
- **Ambient**: `ambientAirTemp ≈ 295 K` (22°C)
- **Range**: `MIN_TEMP = 0 K` to `MAX_TEMP = 99990 K`

---

## Energy

### Air Energy

- **Internal energy**: `E_int = ρ·e·V_cell` where `e = c_v·T` (J)
- **Kinetic energy**: `E_kin = 0.5·ρ·(u²+v²)·V_cell` (J)
- **Total energy**: `E_total = E_int + E_kin` (J)

### Particle Energy

- **Thermal energy**: `E_thermal = heatCapacity × T` (J)
  - Example: TTAN at 1000 K with `heatCapacity = 0.52 J/K` → `E = 520 J`

---

## Force / Pressure

- **Pressure**: **Pascals (Pa)**
  - `1 atm = 101325 Pa = 101.325 kPa`
  - Storage: `pv[y][x]` in Pa
- **Force**: **Newtons (N)** (when needed)
- **Stress**: **Pa** (pressure, same units)

---

## Thermal Conductivity

### Particles

- **Storage**: `HeatConduct` (0-255 scale)
- **Conversion**: `k = (HeatConduct / 255) × 400 W/(m·K)`
  - Range: `0.01 W/(m·K)` (insulator) to `~400 W/(m·K)` (metal)
- **Example**: `HeatConduct = 251` → `k ≈ 394 W/(m·K)` (good conductor)

### Air

- **Constant**: `k_air = 0.026 W/(m·K)` (at 300 K)

---

## Density

### Air

- **Units**: **kg/m³**
- **Storage**: `rho[y][x]` in `Air` class
- **From ideal gas**: `ρ = p/(R·T)` where `R = 287 J/(kg·K)`
- **At 1 atm, 300 K**: `ρ ≈ 1.2 kg/m³`

### Particles

- **Not explicitly stored** (would need `Weight` to represent mass, then `ρ = m/V_particle`)
- **Would be**: `ρ_particle = Weight / V_pixel` (kg/m³) if `Weight` is in kg

---

## Momentum

- **Units**: **kg·m/s** (or **kg·pixels/frame** in game units)
- **Not explicitly stored** (would be `p = m × v` if we had particle mass)
- **Air momentum**: `ρ·u·V_cell` (kg·m/s per cell) stored as `rhou`, `rhov` in solver

---

## Force

- **Units**: **Newtons (N)** = kg·m/s²
- **Gravity force**: `F = m × g` (N) — needs particle mass
- **Pressure force**: `F = p × A` (N) where `A` is area
- **Drag force**: `F_drag = 0.5 × ρ × v² × C_d × A` (N) — needs drag coefficient `C_d`

---

## Power / Heat Transfer Rate

- **Units**: **Watts (W)** = J/s
- **Heat flux**: `Q = k × A × (T1 - T2) / dx` (W) — used in particle heat transfer
- **Heat transfer per frame**: `Q_frame = Q × dt_frame` (J) — energy transferred

---

## Area

- **Contact area (particle-particle)**: `A_contact = 0.001 m × 0.001 m = 1.0×10⁻⁶ m²` (1 mm²)
- **Contact area (particle-air)**: Same, `1.0×10⁻⁶ m²`
- **Air cell face area**: `A_face = cell_size_m = 0.04 m` (per unit depth in 2D)

---

## Viscosity

### Air

- **Dynamic viscosity**: `μ = 1.8×10⁻⁵ Pa·s` (at 300 K, Sutherland's law: `μ(T) = μ_ref × (T/T_ref)^1.5 × (T_ref+S)/(T+S)`)
- **Kinematic viscosity**: `ν = μ/ρ` (m²/s)
  - At 1 atm, 300 K: `ν ≈ 1.5×10⁻⁵ m²/s`

### Particles

- **Not explicitly modeled** (particles use `Loss` property for velocity damping, not true viscosity)

---

## Surface Tension

- **Units**: **N/m** (force per unit length)
- **Not currently modeled** (would affect liquid behavior, droplet formation)

---

## Elastic Modulus / Stiffness

- **Units**: **Pa** (pressure, same as stress)
- **Not currently modeled** (would affect solid deformation, collisions)

---

## Friction / Drag Coefficients

### Air Drag

- **Drag coefficient**: `C_d` (dimensionless)
- **Current**: Uses `Advection` property and `AIR_TO_PARTICLE_ALPHA = 0.002` (relaxation rate)
- **Drag force**: `F_drag = α × (v_air - v_particle)` where `α` is relaxation coefficient

### Particle-Particle Friction

- **Not explicitly modeled** (uses `Loss` property for velocity damping)

---

## Frequency / Rate

- **Update rate**: 60 Hz (60 updates per second)
- **Heat transfer chance**: `HeatConduct/250` per frame (probability-based, not rate-based)
- **Diffusion rate**: `Diffusion` property (random velocity addition per frame)

---

## Summary Table

| Quantity | Units | Storage | Notes |
|----------|-------|---------|-------|
| **Time** | seconds (s) | frames (1/60 s) | 60 FPS |
| **Map width** | m / pixels | `XRES = 612` | `6.12 m` (612 pixels) |
| **Map height** | m / pixels | `YRES = 384` | `3.84 m` (384 pixels) |
| **Map area** | m² / pixels² / cm² | `XRES × YRES` | `23.5 m²` (234,048 pixels² = 234,048 cm²) |
| **Length** | meters (m) | pixels | 1 pixel ≈ 1 cm (from air solver) |
| **Volume (pixel)** | m³ | derived | `1.0×10⁻⁴ m³ = 0.1 L` |
| **Volume (air cell)** | m³ | derived | `1.6×10⁻³ m³ = 1.6 L` (4×4 pixels) |
| **Area (contact)** | m² | derived | `1.0×10⁻⁶ m²` (1 mm²) |
| **Pressure** | Pa | `pv[y][x]` | 1 atm = 101325 Pa |
| **Mass (air cell)** | kg | derived | `ρ·V_cell` from pressure |
| **Mass (particle)** | kg | `Weight` (not meaningful yet) | **Needs definition** |
| **Temperature** | K | `parts[i].temp`, `hv[y][x]` | Kelvin |
| **Heat capacity** | J/K | `parts[i].heatCapacity` | Per particle |
| **Velocity (game)** | pixels/frame | `vx`, `vy` | `game_vel_scale = 0.06` |
| **Velocity (solver)** | m/s | `ux`, `uy` | Meters per second |
| **Gravity (game)** | pixels/frame² | `pGravX/Y` | Acceleration |
| **Gravity (solver)** | m/s² | `gx`, `gy` | Meters per second squared |
| **Thermal conductivity** | W/(m·K) | `HeatConduct` (0-255) | Converted to SI |
| **Density (air)** | kg/m³ | `rho[y][x]` | From ideal gas law |
| **Viscosity (air)** | Pa·s | derived | `μ(T)` via Sutherland's law |
| **Momentum** | kg·m/s | derived | `m × v` (needs mass) |
| **Force** | N | derived | `m × a` or `p × A` (needs mass/area) |
| **Power** | W | derived | `J/s` (heat transfer rate) |

---

## Conversion Factors

- **Frame to second**: `1 frame = 1/60 s`
- **Pixel to meter**: `1 pixel ≈ 0.01 m` (from air solver: `cell_size_m = 4 pixels = 0.04 m`)
- **Game velocity to m/s**: `v_mps = v_game × 0.06`
- **Game acceleration to m/s²**: `a_mps2 = a_game × 144` (for `dx_m = 0.04 m`, `dt = 1/60 s`)
- **Pressure to air density**: `ρ = p/(287 × T)` kg/m³
- **HeatConduct to k**: `k = (HeatConduct/255) × 400` W/(m·K)

---

## Undefined / To Be Decided

### Particle Mass (planned: rename `Weight` → `Mass`)

**Current status**: Exists as `Weight` but **not physically meaningful**. Used for:
- Movement rules (`can_move` checks: lighter particles can't push heavier ones)
- Some gravity calculations (but not consistent)

**Chosen plan** (see `WEIGHT_AND_SPECIFIC_HEAT_PLAN.md`): Rename **Weight → Mass** (float, kg). Fixed volume 0.1 L per particle → **m = ρ × V_pixel** (denser = more mass, same volume).
- **Water** (ρ ≈ 1000 kg/m³): `m = 0.1 kg`
- **Tungsten** (ρ ≈ 19,250 kg/m³): `m ≈ 1.93 kg`
- **Iron** (ρ ≈ 7870 kg/m³): `m ≈ 0.787 kg`
- **Gases** (ρ ~ 1–2 kg/m³): `m ~ 0.0001–0.0002 kg`

**Impact**: Once mass is defined (and we switch to **specific heat**, see below):
- Heat capacity: `C = m × c` where `c` = specific heat (J/(kg·K)); we will store `c`, not `C`.
- Density: `ρ = m / V_pixel` (known volume).
- Momentum: `p = m × v` (kg·m/s)
- Force: `F = m × a` (N)
- Kinetic energy: `E_kin = 0.5 × m × v²` (J)
- Buoyancy: `F_buoy = ρ_fluid × g × V_displaced` (N)

### Particle Density

**Current**: Not stored. Would be `ρ = m/V_pixel` once mass is defined.

**Use cases**:
- Buoyancy calculations (heavier particles sink)
- Pressure from particle weight (hydrostatic pressure)
- Collision momentum exchange

### Surface Tension

**Current**: Not modeled.

**Would affect**:
- Liquid droplet formation
- Liquid-liquid interfaces
- Capillary action

**Units**: N/m (force per unit length)

### Elastic Modulus / Stiffness

**Current**: Not modeled.

**Would affect**:
- Solid deformation
- Collision response (bounciness)
- Stress/strain relationships

**Units**: Pa (same as pressure)

### Drag Coefficients

**Current**: Uses `Advection` property and relaxation rate `α`.

**Could use**:
- Drag coefficient `C_d` (dimensionless)
- Drag force: `F_drag = 0.5 × ρ × v² × C_d × A` (N)

---

## Design: Heat capacity vs specific heat

**Decision**: Once **Weight** means **mass** (kg) and we have a known **volume** (hence density), we will treat thermal properties via **specific heat**, not stored heat capacity. Do **not** mix the two approaches.

| Approach | What we store | Heat capacity used in code |
|----------|----------------|----------------------------|
| **Path A** (current / short-term) | `heatCapacity` (J/K) per particle | Use stored `C` directly. Particles are “thermally abstract” (no need for mass for heat). |
| **Path B** (long-term, chosen) | **mass** (kg) + **specificHeat** (J/(kg·K)) | Compute `C = m × c` on the fly wherever heat capacity is needed. |

**Why Path B once mass exists**:
- **Consistent**: Mass, volume, density, and specific heat are standard physics; `C` is then derived.
- **No conceptual mismatch**: We avoid “mass means something but heat capacity is unrelated.”
- **Density from mass + volume**: `ρ = m / V_pixel` is well-defined; specific heat stays per unit mass.

**Migration plan**:
1. Define **Weight** as mass (kg) and document volume → density.
2. Introduce **specificHeat** (J/(kg·K)) per particle (and/or per element as default).
3. Replace all use of stored `heatCapacity` with `C = mass × specificHeat` (with fallbacks for legacy saves if needed).
4. Remove or deprecate the `heatCapacity` field; do not keep both `heatCapacity` and `specificHeat` as independent inputs.

**Do not mix**: Either we use stored `C` and do not derive it from mass (Path A), or we use mass + `c` and always compute `C` (Path B). We do not store both `C` and `c` and use them interchangeably.

---

## Notes for Future Tuning

1. **Particle mass**: Define `Weight` as actual mass in **kg** or **g**. Then heat capacity can be computed from specific heat: `C = m × c` where `c` is J/(kg·K).

2. **Consistency**: Ensure all physics uses consistent units (SI preferred for solver, game units for display/storage).

3. **Volume**: Current pixel volume (`0.1 L`) assumes 2D simulation. If we add 3D depth, volume scales linearly.

4. **Air cell size**: Currently `0.04 m` (4 cm). This affects CFL time step and pressure gradients. Can be tuned for gameplay vs realism.

5. **Velocity scale**: `game_vel_scale = 0.06` converts m/s to pixels/frame. May need tuning if velocities feel too fast/slow.

6. **Heat capacity vs specific heat**: See **Design: Heat capacity vs specific heat** below. We will use **specific heat** (Path B) once mass is defined; do not mix the two approaches.

7. **Contact area**: Currently assumes `1 mm²` for particle-particle and particle-air contact. Could be tuned based on particle size/material properties.

---

## References

- Air solver: `src/simulation/Air.cpp` (`update_air`, `cell_size_m = CELL * 0.01`)
- Heat capacity: `src/simulation/ElementDefs.h` (defaults), `src/simulation/elements/*.cpp` (element values)
- Velocity conversion: `src/simulation/AirSolverWrapper.cpp` (`game_vel_scale`)
- Gravity conversion: `src/simulation/AirSolverWrapper.cpp` (gravity scale calculation)
