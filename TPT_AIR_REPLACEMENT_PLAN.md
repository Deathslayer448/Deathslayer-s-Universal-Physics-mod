# TPT Air / Pressure / Temperature Replacement Plan

Replace TPT’s current air, pressure, and temperature systems with the AirSolverRusanov-based system. This document inventories what TPT has, how it interacts, and a phased plan to replace it without breaking the sim.

---

## 0. Progress summary (what's done)

- **Phase 1 – Prep:** Done. Solver in `src/simulation/air_solver/` (air_solver_rusanov.cpp, heat.cpp, heat.hpp, air_solver_api.h); wrapper in `AirSolverWrapper.cpp/h`; sync_from_sim / sync_to_sim / step / set_boundary_mode / set_uniform; build wired.
- **Phase 2 – Replace air evolution:** Done. `Air::update_air()` uses Rusanov path: ensure_created → set_boundary_mode(edgeMode, 1 kPa for void) → sync_from_sim → step(frame_dt, airSolverStepsPerFrame) → sync_to_sim; wall cells get v=0 and p from neighbor after sync. `Air::update_airh()` no-op when solver has state. Clear() uses useAtmosphericPressure (101325 vs 1000 Pa baseline).
- **Boundary:** Void = open (ghost 1 kPa so pressure leaks); Loop = periodic; Solid = reflective only where bmap_blockair (from sync). No per-frame edge overwrite to P_atm.
- **Solver stability / vacuum:** No pressure floor; p_max cap only (clamp_p_max_only). Vacuum interface: Rusanov with s_max floor (c_dense); large density ratio: pressure term min(p_L,p_R), donor-state flux when flux into low-ρ (rho_receiver_donor); cap donor velocity into very low-ρ (rho_receiver_cap_v, donor_v_max_over_c); no viscous momentum into low-ρ; sync_from_tpt soft cap on velocity for low-p cells.
- **Options / display:** Pressure unit default kPa; "Pressure in kPa" and atmospheric in Solver and air; Shift+2 = normalized pressure, Shift+3 = high focus, Shift+4 = low focus.
- **Remaining:** Phase 4 cleanup (remove dead legacy air code, repurpose constants); optional: gravity in solver, particle→solver source terms, open BC tuning.

---

## 1. What TPT Has Today (Inventory)

### 1.1 Data (where it lives)

| System | Storage | Location | Units / notes |
|--------|---------|----------|----------------|
| **Air pressure** | `sim.pv[YCELLS][XCELLS]` | `RenderableSimulation` (Simulation.h) | Pascals (already 101325 = 1 atm) |
| **Air velocity** | `sim.vx`, `sim.vy` | same | Game units; used with CELL for gradients |
| **Air temperature** | `sim.hv[YCELLS][XCELLS]` | same | Kelvin (ambient = `ambientAirTemp`) |
| **Air density** | `air.rho[YCELLS][XCELLS]` | Air.h/cpp | kg/m³ (P_atm/(R*T) in Clear()) |
| **Wall (block air)** | `air.bmap_blockair`, `air.bmap_blockairh` | Air.h | 0 = fluid, 0x8 = block air/heat |
| **Particle temperature** | `parts[i].temp` | Particle | Kelvin; per-particle |
| **Particle velocity** | `parts[i].vx`, `parts[i].vy` | Particle | Game units |
| **Ambient** | `air.ambientAirTemp`, P_atm = 101325 | Air.cpp, Clear(), config | 1 atm in Pa; T in K |
| **Temporary buffers** | `air.ovx`, `air.ovy`, `air.opv`, `air.ohv` | Air.h | Used in update_air / update_airh |

Grid: `CELL = 4`, `XCELLS = 153`, `YCELLS = 96`, `NCELL = 153*96`.  
Constants: `AIR_TSTEPP`, `AIR_TSTEPV`, `AIR_VADV`, etc. in SimulationConfig.h.

### 1.2 Update order and call sites

- **Simulation.cpp** (one sim step):
  1. `air->update_air();`   // pressure + velocity (current “physics”)
  2. `if (aheat_enable) air->update_airh();`   // air temperature (advection + kernel blend)
  3. Gravity, then particle updates.

- **Air.cpp**
  - `update_air()`: pressure from div(v), velocity from ∇P/ρ, walls, rho update; uses magic constants, no conservation.
  - `update_airh()`: hv advection (kernel + “far away” sampling), edge hv = ambientAirTemp; air convection block is **disabled** (commented) so it doesn’t overwrite velocity.

- **Particle–air coupling** (Simulation.cpp heat transfer):
  - `hv[y/CELL][x/CELL]` and `parts[i].temp` exchange heat when `aheat_enable && !PROP_NOAMBHEAT`:  
    `dtemp = hv - parts[i].temp`, then both updated with alpha/HeatCapacity. So **air temp and particle temp are two systems that interact** at particle positions. TPT air temp has **no heat capacity** (just simple diffusion/advection); particle–air exchange **ignores** air heat capacity. Our system replaces this with proper ρ·c_v and k∇²T.

- **Particle–air velocity**: Particles read `vx`, `vy` (e.g. drag, wind); some elements write forces to `fvx`, `fvy`. So **air velocity and particle velocity are separate but interact**.

### 1.3 What we’re replacing (and what stays)

| Replace | With | Notes |
|---------|------|--------|
| **Pressure evolution** (update_air pressure part) | AirSolverRusanov gas step | p from solver state (ρ,e,u,v → p) |
| **Air velocity evolution** (update_air velocity part) | Same gas step | vx, vy from solver u, v |
| **Air temperature evolution** (update_airh) | Solver’s heat diffusion + gas internal energy (with heat capacity c_v, c_p) | T from solver (e/c_v); hv = T; replaces “no heat capacity, just simple diffusion” |
| **Air density** | Solver state ρ | rho = U0[0]; no separate rho law in Clear |
| **Wall handling** | Solver `wall` mask | bmap_blockair → wall; reflective + adiabatic |

| Keep (for now) | Later / optional |
|-----------------|-------------------|
| **Particle temperature** `parts[i].temp` | Same; coupling to air becomes “particle ↔ solver hv” |
| **Particle velocity** `parts[i].vx/vy` | Same; coupling stays “particle ↔ solver vx, vy” |
| **Ambient 1 atm (101325 Pa)** | Used as initial/reference; solver uses same units (Pa, K, kg/m³) |
| **ambientAirTemp** | Initial T and boundary reference; solver T in Kelvin |
| **aheat_enable** | Can gate “use solver hv” vs legacy hv |
| **Grid (XCELLS, YCELLS, CELL)** | Solver grid = same; dx = CELL * (meter per pixel) |

So: **strip** the *evolution* of pv, vx, vy, hv, rho from Air.cpp and replace with one “solver step + copy back.” **Keep** particle systems and ambient constants; **reconnect** particle–air coupling to solver state.

---

## 2. How the new system fits

### 2.1 AirSolverRusanov provides

- **One state** per cell: ρ, ρu, ρv, E (mass, momentum, energy). From that we get u, v, p, T (and hv = T).
- **Convection + viscosity** (gas step); **heat diffusion** (heat module); **reflective walls** (wall mask).
- **Conservative** (mass, energy); **stable** (Rusanov + CFL + diffusion limit); **same units** (Pa, K, kg/m³, m/s if we choose dx in meters).

### 2.2 Mapping TPT ↔ solver

- **Copy in (each tick):**
  - `pv` → p, `vx`/`vy` → u,v, `rho` → ρ (or derive ρ from p, hv and ideal gas).
  - `bmap_blockair` → `wall` (true = solid).
  - Optionally use `ambientAirTemp` and 101325 for initial/BC reference.

- **Step:**  
  `dt = state.step(); if (dt > 0) state.apply_heat_diffusion(dt);`  
  (and optionally substep to match frame time.)

- **Copy out:**  
  From solver primitives: `pv = p`, `vx = u`, `vy = v`, `hv = T` (Kelvin), `rho = ρ`.

- **Particle–air coupling (unchanged conceptually):**
  - **Heat:** Keep current logic that uses `hv[y/CELL][x/CELL]` and `parts[i].temp`; after replacement, hv is solver T, so particle ↔ air heat exchange is particle ↔ solver temperature.
  - **Velocity:** Keep current logic that uses `vx`/`vy` for drag/wind; after replacement, vx/vy are solver velocity. Optionally add momentum transfer from particles to solver (source terms) later.

### 2.3 Ambient pressure (1 atm) and initial state

- **Already in TPT:** P_atm = 101325 Pa in Air.cpp; rho from P_atm/(R*ambientAirTemp) in Clear().
- **Solver:** Same units; `set_uniform(rho, 0, 0, P_atm)` with rho = P_atm/(R*T_ambient) gives a consistent rest state.
- **Plan:** When we “Clear” or init the solver, use `ambientAirTemp` and 101325 to set uniform (ρ, 0, 0, p). When we replace update_air/update_airh, **don’t** force edges to P_atm (that loses energy); let solver boundaries be reflective or use a proper open BC later. So “ambient 1 atm” = initial/reference value and optional BC target, not a per-frame overwrite.

---

## 3. Phased replacement plan

### Phase 1 – Prep (no behavior change) — DONE

1. **Add solver to build**  
   - Copy or submodule AirSolverRusanov (air_solver_rusanov.cpp, heat.cpp, heat.hpp) into TPT (e.g. `src/simulation/` or a dedicated folder).  
   - Wire build (meson/cmake) so TPT compiles and links them.

2. **Solver wrapper type**  
   - Define a small wrapper (e.g. `AirSolver` or `RusanovAir`) that:
     - Holds `State` + optional heat; grid = XCELLS × YCELLS; dx = CELL * meter_per_pixel (e.g. 4e-3 m if CELL=4 and 1 pixel = 1 mm).
     - `sync_from_sim(sim, air)`: copy `pv`, `vx`, `vy`, `rho`, `bmap_blockair` into solver U and wall.
     - `sync_to_sim(sim, air)`: copy solver primitives to `pv`, `vx`, `vy`, `hv`, `rho`.
     - `step(dt_frame)`: call `state.step()` (and optionally `apply_heat_diffusion`) in a loop so total advance ≈ dt_frame, then sync_to_sim.
   - No call from Simulation yet; just compile and maybe unit-test sync in isolation.

3. **Document current call graph**  
   - List every read of `pv`, `vx`, `vy`, `hv`, `rho` outside Air.cpp (elements, tools, renderer, Lua). So we know what “contract” the new system must satisfy (same grid, same meaning of pv/vx/vy/hv/rho).

### Phase 2 – Replace air evolution only — DONE

4. **Switch Air.cpp to solver**  
   - In `Air::update_air()`:
     - Build or reuse the wrapper; `sync_from_sim(sim, *this)` (pv, vx, vy, rho, bmap_blockair → U, wall).
     - Call wrapper `step(frame_dt)` (solver + heat step, then copy out).
     - `sync_to_sim(sim, *this)` (pv, vx, vy, hv, rho).
   - Remove or stub the **old** pressure/velocity update logic (the big loops that update opv, ovx, ovy from div(v), ∇P, etc.). Keep edge/wall handling only if the solver doesn’t cover it (solver already has reflective walls; we only need to set wall from bmap_blockair).
   - In `Air::update_airh()`: either **remove** (solver already updates hv via heat diffusion) or make it a no-op when “solver air” is enabled, so we don’t overwrite hv after sync_to_sim.
   - Keep `Clear()` / `ClearAirH()` so they init `pv`, `vx`, `vy`, `hv`, `rho` to a consistent rest state (e.g. P_atm, 0, 0, ambientAirTemp, ρ_atm). When solver is used, either also init the solver state from that, or init solver and then copy out to sim once.

5. **Ambient and init**  
   - Ensure on Clear we set solver (if any) to uniform (ρ_atm, 0, 0, P_atm) with ρ_atm = 101325/(R*ambientAirTemp), so “1 atm + ambientAirTemp” remains the rest state.
   - Do **not** force pv/hv at domain edges to P_atm/ambient every frame (that would drain energy). Use solver’s reflective walls; open boundaries can be a later phase.

6. **Test**  
   - Run with solver only: no explosions, no NaNs; pressure and velocity look reasonable; hv (air temp) is updated by solver.  
   - Compare with old code in a side-by-side branch if useful, but goal is to fully switch.

### Phase 3 – Particle–air coupling (unchanged contract) — DONE

7. **Particle heat transfer**  
   - Simulation.cpp heat transfer already uses `hv[y/CELL][x/CELL]` and `parts[i].temp`. After Phase 2, hv is solver T, so this remains “particle temp ↔ air temp” with no API change.  
   - Optionally tune alpha/HeatCapacity so exchange rate feels right with the new hv behavior.

8. **Particle–air velocity**  
   - Any code that reads `sim.vx`/`sim.vy` for drag/wind now reads solver velocity; no change needed except to ensure sync_to_sim runs before particle updates.  
   - Optional later: add momentum/energy source terms from particles to solver (so particles push the air).

### Phase 4 – Cleanup and tuning — REMAINING

9. **Remove dead code**  
   - [ ] Delete or keep behind flag the `#if 0` legacy update_air block in Air.cpp.  
   - [ ] Remove or repurpose ovx, ovy, opv, ohv if unused when solver is always on.  
   - [ ] Keep vorticity, kernel, ApproximateBlockAirMaps only if still used.

10. **Constants**  
    - [ ] Repurpose or document AIR_TSTEPP, AIR_TSTEPV, etc. for solver path (solver uses CFL + diffusion limit).  
    - Keep ambientAirTemp, P_atm (101325), R, and any display/options (e.g. useAtmosphericPressure) as-is.

11. **Performance**  
    - [ ] Profile one frame; tune airSolverStepsPerFrame or CFL if needed; simple-air mode later.

---

## 4. Summary table (what replaces what)

| TPT current | Replaced by | When |
|-------------|-------------|------|
| update_air() pressure + velocity | Solver gas step (+ heat) | Phase 2 |
| update_airh() hv advection (no heat capacity) | Solver heat diffusion + internal energy (ρ·c_v, k∇²T); hv = T | Phase 2 |
| TPT temperature system (simple diffusion, no air heat capacity) | Our system: air T from e/c_v, heat capacity, k∇²T; particle–air coupling can use ρ·c_v·V_cell | Phase 2/3 |
| rho from P/(RT) in update | Solver state ρ | Phase 2 |
| Wall handling in update_air | Solver wall mask (bmap_blockair) | Phase 2 |
| pv, vx, vy, hv, rho arrays | Same arrays; **filled by** solver | Phase 2 |
| parts[i].temp | Unchanged | — |
| Particle ↔ hv heat transfer | Same code; hv now from solver | Phase 3 |
| Particle ↔ vx, vy (drag/wind) | Same code; vx, vy from solver | Phase 3 |
| Ambient 1 atm, ambientAirTemp | Init and reference only; no per-frame overwrite | Phase 2 |

---

## 5. Performance — can we do all this and keep it?

**Yes.** Rough numbers:

- **Grid:** 153×96 ≈ 14.7k cells. Solver is O(N) per step: one pass for convection (Rusanov), one for viscosity, one for heat diffusion. No heavy allocs per step, no Eigen, no external deps.
- **Substeps:** CFL-limited dt; we may need several steps per frame (e.g. 5–20) to advance one frame’s worth of time. That’s still on the order of 10⁵–10⁶ ops per frame. Modern CPU handles that easily in 16–33 ms (60–30 FPS).
- **Evidence:** The SDL demo (80×120, similar size) ran 260k+ frames with good responsiveness. TPT’s grid is 153×96; same ballpark.
- **Tuning if needed:** Cap substeps per frame (e.g. max 10); accept slightly less sim time per frame when flow is fast. Or lower CFL a bit to take fewer, larger steps. Profile once integrated and adjust.

So: **doable.** Integrate, profile, then tune (substeps per frame, CFL) only if a specific platform or scene needs it.

---

## 6. Risks and mitigations

- **Elements assume old pv/vx/vy/hv behavior:** Mitigation: keep same grid and meaning (pv = pressure in Pa, hv = air temp in K). If something assumed “hv is only advected,” we may need one-off fixes for that element.
- **Save compatibility:** Saves store pv, vx, vy, hv, rho (and options). Keep format; only the *evolution* changes. Old saves should load and then evolve with the new solver.
- **Performance:** Solver is O(cells) per step; TPT has 153×96 cells. Mitigation: profile; substep only as needed; optional “fast air” later if required.
- **Gravity/buoyancy:** Current air code has disabled gravity-in-velocity. Solver doesn’t have gravity yet. We can add ρ*g as a source term in the solver later; Phase 2 can be gravity-off for air.

---

## 7. File-level checklist

- [x] **Add:** AirSolverRusanov sources + wrapper (AirSolverWrapper).
- [x] **Modify:** Air.cpp — update_air uses solver sync → step → sync; update_airh no-op when solver active.
- [ ] **Modify:** Air.h — keep pv, vx, vy, hv, rho, bmap_blockair; remove or repurpose ovx, ovy, opv, ohv if unused (Phase 4).
- [x] **Modify:** Simulation.cpp — call order (update_air then update_airh) unchanged; particle heat transfer unchanged.
- [x] **Keep:** SimulationConfig.h CELL, XCELLS, YCELLS; ambientAirTemp, 101325, useAtmosphericPressure in Air and options.
- [x] **Keep:** All reads of pv, vx, vy, hv, rho in elements/tools/renderer; they get solver output.
- [ ] **Later:** Gravity in solver; particle → solver momentum/energy source terms; open boundaries.

This plan strips TPT’s custom air/pressure/temperature evolution and replaces it with the single AirSolverRusanov-based system while keeping particle systems and their coupling points unchanged. TPT temperature (no heat capacity, simple diffusion) is replaced by our system with heat capacity and k∇²T; particle–air coupling can use air heat capacity (ρ·c_v·V_cell) for energy-conserving exchange. Performance: doable (O(N), ~15k cells, a few substeps per frame); profile and tune if needed.
