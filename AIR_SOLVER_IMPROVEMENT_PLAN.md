# Air Solver Improvement Plan

Based on the current Rusanov + heat integration and the issues you reported. References are to the TPT-integrated code in `src/simulation/air_solver/` and `AirSolverWrapper`.

---

## Two temperatures (independent but coupled)

There are **two different temperatures** in the sim:

1. **Air temperature** (`hv[y][x]`) — the **ideal-gas temperature** of the air in each cell. This is the `T` in the gas EOS: `p = ρ R T`, and in the solver it comes from internal energy: `T = e/c_v`. The solver owns this during its step; we sync it to/from TPT as `hv`.
2. **Particle temperature** (`parts[i].temp`) — the temperature of solid/liquid/gas **particles**. Stored per particle, independent of air.

They are **independent** (separate state) but **coupled**: particle–air heat transfer in `Simulation.cpp` exchanges heat between `parts[i].temp` and `hv[y/CELL][x/CELL]`, so hot particles heat the air and hot air heats particles. In the improvement plan below, “temperature” in the solver context means **air temperature** (ideal-gas `hv`); “particle temperature” is always explicit.

### Heat capacity (don't forget) — not yet implemented

**Status**: We have **not** implemented heat-capacity-aware coupling or custom heat transfer yet. The solver uses air heat capacity (c_v, c_p) internally; TPT's existing particle–air heat transfer in `Simulation.cpp` uses per-element **HeatCapacity** for the particle side only and ignores the heat capacity of the air cell. A proper **custom heat transfer** that respects both air and particle heat capacities (and feeds into the solver) is planned but not done.

- **Air**: The solver already uses **ideal-gas heat capacities** `c_v` and `c_p` (e.g. `c_v = R/(γ−1)`, `c_p = γ*R/(γ−1)`). Internal energy per unit mass is `e = c_v * T`; the EOS is `p = (γ−1) ρ e`. Thermal diffusion in `heat.cpp` uses `c_p` and `k_thermal`. So air heat capacity is in the solver; it is **not** yet used in particle–air coupling.
- **Particles**: Today, each element can have a **HeatCapacity** in the Element struct (used in `Simulation.cpp` for the existing particle–air and particle–particle heat transfer). When we implement our **custom** coupling:
  - Use a **default heat capacity for particles** (e.g. 1.0f) so every particle has a value and the system works before per-type values are set. Any particle (or element) without an explicit value should fall back to this default.
  - **Later**: assign **per-element (per-particle-type) heat capacity** so each particle type has its actual heat capacity. Many elements already define `HeatCapacity` in their element definition; ensure a sensible default wherever it's missing.
- **Custom heat transfer** (planned): Replace or extend the current particle–air transfer with one that (1) uses **air heat capacity** (ρ * c_v per cell) and **particle heat capacity** (default or per-element) so heat exchange is consistent, and (2) feeds the updated air state (e.g. `hv`) into the solver so particle heating/cooling affects pressure and air temp. Not implemented yet.

### Advanced HUD (F3): air energy and particle energy

In **advanced view** (F3), when hovering a particle or air, the HUD shows **two energy values** (in addition to temperature, air temp, pressure, etc.):

1. **Air energy** (`E_air`): Total energy of the air cell in **J** (or mJ): internal energy + kinetic energy, i.e. `(p/(γ−1) + 0.5*ρ*v²) × volume`. This is the actual energy content of the cell in SI units.
2. **Particle energy** (`E_part`): **HeatCapacity × temperature** (HC×T). Temperature alone doesn’t tell the whole story for particles — a high‑HeatCapacity particle at the same temperature holds more thermal energy than a low‑HeatCapacity one. So we show **particle thermal energy** as `E_part = HeatCapacity × T` (in the same HC×T units the sim uses for heat transfer). When hovering a particle, both `E_air` (for the cell) and `E_part` (for that particle) are shown so you can see air energy and particle energy side by side.

---

## 1. Temperature ↔ Pressure Coupling

**In practice**: The coupling is **one-way**: **pressure → heat** works (pressure creates or removes heat), but **air temperature → pressure** does not (air temp isn’t creating pressure). So pressure affects temperature; temperature does not affect pressure. On top of that, **particle temperature is not coupled to air** — hot/cold particles don’t properly change air temp (and thus pressure), and the solver overwrites `hv` each frame so particle–air heat transfer is lost.

### Current behavior

- **Solver EOS**: The solver uses ideal gas: `p = (γ−1) ρ e`, and **air temperature** is `T = e/c_v`. So inside the solver, **air pressure and air temperature are coupled** via internal energy `e`; that part works.
- **sync_from_tpt** (`air_solver_rusanov.cpp`): Builds solver state from `pv`, `vx`, `vy`, `rho` only. It does **not** use `hv` (air temperature). Internal energy is set as:
  - `e = p / ((γ−1) * r)`  
  So the solver’s temperature is determined only by `p` and `rho`, not by TPT’s `hv`.
- **sync_to_tpt**: Writes back `hv[i] = temperature_from_e(e)`, so the solver overwrites TPT’s **air temperature** (`hv`) every frame.
- **Particle–air heat** (`Simulation.cpp`): Particles exchange heat with `hv`; that only affects TPT’s air temp. On the next frame, the solver runs and overwrites `hv` from its own state (which was built **without** the previous `hv` or any particle heating). So **particle temperature is not coupled to air** — heating/cooling from particles is lost for the air side.
- **Particle temperature → pressure**: The old “HotAir” pressure addition from **particle temperature** is commented out in `Simulation.cpp`; particle temp does not add pressure anywhere.

So (one-way coupling):

- **Air temperature → pressure**: **Uncoupled** — air temperature is **not** creating pressure. Changing air temp doesn’t change pressure the way ideal gas (p = ρ R T) would suggest; that direction is broken or missing.
- **Pressure → air temperature (heat)**: **Coupled** — pressure **does** create or remove heat. So pressure affects temperature; that direction works. The coupling is **only one-way** for some reason (pressure → heat works; air temp → pressure does not).
- **Particle temperature ↔ air** is **not** coupled: particle–air heat transfer updates `hv`, but the solver never sees it and overwrites `hv` each frame, so particle temp doesn’t affect air temp or pressure.
- Heat conductivity in the solver is **constant** (`k_thermal` in `heat.cpp`); there is no pressure- or temperature-dependent conductivity.

### Improvements (main goal: couple particle temperature to air)

1. **Use TPT air temperature when syncing into the solver**  
   In `air_solver_sync_from_tpt`, use **air temperature** `hv` to set internal energy. That way, **particle–air heat transfer** (which updates `hv` in `Simulation.cpp`) is fed into the solver: hot particles heat the air → `hv` goes up → solver gets higher `e` → pressure and air temp stay consistent. Options:
   - Option A: Set `e = c_v * hv[i]` (with a clamp) so that air temp is the ideal-gas T and `p = (γ−1) ρ e = ρ R T`. Then `pv` and `rho` must be consistent with `hv` (e.g. `rho = p/(R*T)` if we want ideal gas).
   - Option B: Keep `e = p/((γ−1)*r)` but blend in a term from `hv` so that particle–air heating (which updates `hv`) affects the solver state.
   - Ensure after this change that `rho` and `pv` are still consistent with **air temperature** (e.g. if we set `e` from `hv`, we may need `rho = p/(R*hv)` or keep `pv` and adjust `e` so that pressure and air temp are consistent).

2. **Particle temperature affecting pressure (via air)**  
   Once the solver uses `hv` (item 1), particle–air heat transfer will **indirectly** couple particle temp to pressure: hot particles heat air → `hv` rises → solver sees it → air temp and pressure rise. Optionally, also re-enable and tune the HotAir-style **direct** pressure source in `Simulation.cpp` (add pressure to `pv` where hot particles are) for a stronger effect.

3. **Pressure-dependent heat conductivity (optional)**  
   In `heat.cpp`, replace constant `k_thermal` with a function of local pressure (and optionally temperature), e.g. `k(p, T)`, and use it in the diffusion step so that conductivity depends on pressure/temperature.

4. **Custom heat transfer and heat capacity (not yet implemented)**  
   Implement **custom heat transfer** between particles and air that:
   - Uses **air heat capacity** (ρ * c_v per cell) and **particle heat capacity** so heat exchange is physically consistent.
   - **Default particle heat capacity**: Use a default value (e.g. 1.0f) for every particle so the system works before per-type values are set; use this default when an element has no explicit HeatCapacity or for compatibility.
   - **Later**: Use **per-element (per-particle-type) heat capacity** so each particle type has its actual heat capacity (set in each element definition; ensure defaults where missing).
   - Feeds the updated air state (e.g. `hv`) into the solver (see item 1) so particle heating/cooling affects pressure and air temp.

---

## 2. Velocity Too Strong (e.g. 10 kPa → Huge Velocity)

### Current behavior

- **Units**: Solver works in SI: pressure in Pa, velocity in m/s. TPT stores velocity in “game units” (effectively per-frame displacement). `game_vel_scale` (default `0.06`) converts: `v_mps = v_game * game_vel_scale`, and on output `v_game = v_mps / game_vel_scale`.
- **Physics**: A 10 kPa pressure difference over one cell (e.g. `dx = 0.04` m) gives a large pressure gradient, so the solver correctly produces strong acceleration and thus large m/s velocities in a few steps. When converted back to game units (`v_mps / 0.06`), those m/s become very large on-screen.
- So the effect is a mix of: (1) **physical** — pressure gradients really do drive strong flows when cell size is small; (2) **scaling** — the same flow “feels” too strong in game units.

### Improvements

1. **Clarify and document units**  
   In `AirSolverWrapper.h` / `air_solver_api.h`, document exactly what TPT’s `vx`/`vy` represent (e.g. “pixels per frame” or “game velocity units”) and how `game_vel_scale` is derived (e.g. “1 game unit = 0.06 m/s so that 1 m/s = 1/0.06 game units”).

2. **Reduce effective strength of pressure-driven flow (gameplay)**  
   - **Option A**: Increase `game_vel_scale` so that the same m/s is written back as smaller game velocity (e.g. `game_vel_scale = 0.12` → half the on-screen velocity for the same solver m/s).  
   - **Option B**: Cap or damp velocity in the solver output: after `primitives()` / in `sync_to_tpt`, clamp `ux`/`uy` to a max magnitude (in m/s) before converting to game units, or apply a soft cap (e.g. tanh scaling).  
   - **Option C**: Reduce pressure gradient in the solver (e.g. artificial viscosity or a “drag” term that limits velocity magnitude). This changes physics; prefer A or B if the goal is “same physics, less strong on screen.”

3. **Tune `game_vel_scale` empirically**  
   For a 10 kPa difference, measure typical m/s from the solver and choose `game_vel_scale` so that the resulting game velocity looks right (e.g. not “huge”).

---

## 3. Single 0 kPa Cell → Whole Simulation Nearly Stops

### Current behavior

- **sync_from_tpt**: If `pv[i] == 0`, pressure is clamped to `1e-10`; if `rho[i]` is small, it’s clamped to `rho_min` (1e-6). So the solver never sees true vacuum; it sees a very low-pressure, low-density cell.
- **Convection**: The interface between a normal cell and this “vacuum” cell has a huge pressure jump. The Rusanov flux dumps a lot of mass and **momentum** into the vacuum cell in one step.
- **primitives**: Velocity is `ux = U[1]/rho`, `uy = U[2]/rho`. In that cell, `rho` is still only `rho_min` (or slightly higher after mass flux), but momentum `U[1]`, `U[2]` can be very large (from the neighboring high-pressure cell). So **velocity in that cell becomes huge** (e.g. `ux = large_momentum / rho_min`).
- **max_lambda**: CFL uses `lam = max over cells of (|ux| + |uy| + c)`. That one cell can have enormous `|ux|` and `|uy|`, so **lam explodes** and:
  - `dt_cfl = CFL * dx / lam` becomes **tiny** (e.g. 1e-6 or smaller).
  - The simulation effectively crawls (one step per frame with a microscopic dt), so pressure and flow appear “nearly static.”

So the bug is: **one vacuum (or near-vacuum) cell gets a huge momentum from its neighbor and then a huge velocity; that dominates the CFL condition and collapses dt.**

### Improvements

1. **Cap velocity when computing the CFL lambda**  
   In `max_lambda()`, instead of using raw `|ux| + |uy| + c`, use a capped velocity, e.g. `lam = max(lam, min(|ux| + |uy|, v_max) + c)` with a reasonable `v_max` (e.g. 2–3× sound speed at ambient). So a single crazy cell does not dictate dt.  
   - Optionally, also cap velocity in `primitives_from` / `primitives_from_U4` when used **only for** CFL (or everywhere) so that the rest of the step doesn’t see unbounded velocities.

2. **Cap velocity in primitives used for flux and time step**  
   After computing `ux`, `uy` from `U[1]/r`, `U[2]/r`, clamp them to a maximum magnitude (e.g. multiple of local sound speed or a fixed m/s limit). Use these capped values for:
   - `max_lambda()` (so dt doesn’t collapse),
   - and optionally for the flux and viscous update (so the vacuum cell doesn’t inject unrealistic momentum back).

3. **Vacuum / low-pressure handling**  
   - **Option A**: In cells with `p < p_floor` (e.g. 1 Pa), treat as “vacuum”: set velocity to zero (or very small) for CFL and for flux neighbor state, so they don’t produce huge velocities.  
   - **Option B**: Enforce a **pressure floor** in the solver (e.g. after each step, set `p = max(p, p_floor)` and adjust `e` accordingly) so we never get true vacuum and momentum doesn’t pile up in near-zero-density cells.  
   - **Option C**: In `sync_from_tpt`, when `pv[i] <= 0` (or very small), don’t use `rho_min` with a tiny pressure; instead set a small but non-zero pressure (e.g. 1 Pa) and a consistent `rho` (e.g. `rho = p/(R*T_ambient)`), and zero velocity, so the solver never sees “zero pressure + rho_min” which leads to huge u.

4. **State validity**  
   Keep or tighten `state_valid()` so that if a cell has unreasonable velocity (e.g. > some multiple of sound speed), we reject the step or clamp before accepting.

---

## 5. Energy loss in enclosed system

### Observed behaviour

- **Setup**: Solid map (walls all around), enclosed system, no leaks.
- **Initial**: 1 atm, 22°C (air temp).
- **Action**: Add some pressure (so added energy).
- **Result**: Pressure and air temperature go up, then down; they equalize over time, but the **final** pressure and temperature are **lower** than the initial state (1 atm, 22°C) even though energy was added and there are no leaks.

So total energy (and/or mass) is **decreasing** in a closed system, which violates conservation.

### Possible causes

1. **Heat diffusion at walls** (`heat.cpp`): Adiabatic walls are implemented by using `T_self` for the neighbor when the neighbor is a wall, so `lap_T = (Tp + Tm + Tpy + Tmy - 4*T)/dx²` with one of the four neighbours equal to `T`. The discrete Laplacian is not written in conservative flux form at the boundary, so **the sum over all fluid cells of `k_thermal * lap_T` may not be zero**. That can introduce a small net gain or loss of energy each step; over many steps it can noticeably lower (or raise) total energy.

2. **Viscous boundary treatment** (`air_solver_rusanov.cpp`, `apply_viscous`): At wall-adjacent cells, velocity gradients use reflective ghosts. If the viscous stress or the energy update `div(τ·v)` is not exactly conservative at walls (e.g. one-sided differences or missing terms), total energy could drift.

3. **Sync round-trip**: Every frame we replace the solver state with TPT state (`sync_from_sim`) then step then write back (`sync_to_sim`). Solver state is built from `pv`, `vx`, `vy`, `rho` only; `e` is set as `e = p/((γ−1)*r)`. If elsewhere in TPT something modifies `pv` or `rho` (e.g. wall copy, or particle–air heat indirectly), we could be feeding a state with less energy. Unlikely if the user only adds pressure and runs with no particles, but worth checking whether any code path changes `pv`/`rho` between `sync_to_sim` and the next `sync_from_sim`.

4. **Rusanov numerical dissipation**: The scheme is conservative in principle; numerical diffusion should not reduce total energy, only redistribute it (KE → internal energy). So this is unlikely to explain a net drop below initial.

5. **Order of operations / heat diffusion vs convection**: Heat diffusion updates `E` in place with `E += dt * k_thermal * lap_T`. If the heat step is not exactly conservative (see 1), running it every frame after the convective step will accumulate a drift.

### Improvements

1. **Make heat diffusion conservative with adiabatic walls**: Rewrite the thermal diffusion in `heat.cpp` in **conservative flux form**: for each face, compute flux `F = k * (T_R - T_L)/dx` (zero at wall faces by using `T_self` for the wall side), then update cells with `dE/dt = (F_in - F_out)/dx` (or 2D equivalent). Then the sum over all cells of `dE/dt` is zero by construction (each interior face counted twice with opposite sign; wall faces zero).

2. **Audit viscous energy update at walls**: In `apply_viscous`, verify that the energy source `div(τ·v)` at wall-adjacent cells uses the same ghosting as the stress so that total mechanical work is consistent and total energy is conserved.

3. **Optional: monitor total energy**: In debug, compute total mass and total energy (solver side) after each step and after heat diffusion; log or assert that they are non-decreasing in a closed system (up to a tiny tolerance). That will pinpoint whether the loss is in convection, viscosity, or heat diffusion.

4. **Check TPT-side modifications of pv/rho**: Ensure nothing in the sim loop (e.g. wall pressure copy, or other air logic) changes `pv` or `rho` in a way that reduces energy between solver syncs.

---

## 6. Implementation Order: Foundations First, Then Dependencies

**Phase 1 — Foundations** (no dependencies; do first so the rest can build on them)

1. **Vacuum / 0 kPa bug (Section 3)**  
   Fix so simulation doesn’t freeze when any cell hits 0 kPa. Cap velocity in `max_lambda()` (and optionally in primitives) so one cell can’t force dt to zero. *Foundation: solver must be stable before we add coupling.*

2. **Energy loss in enclosed system (Section 5)**  
   Make heat diffusion conservative (flux form, zero flux at walls). *Foundation: conservation must hold before we trust temp/pressure coupling.*

3. **Default particle heat capacity**  
   Define a default value (e.g. 1.0f) and use it as fallback when an element has no or zero HeatCapacity. *Foundation: custom heat transfer (Phase 2) will need a particle heat capacity for every particle; define the default once.*

**Phase 2 — Dependencies** (depend on Phase 1)

4. **Air temp → pressure (Section 1)**  
   Use `hv` in `sync_from_tpt` to set `e` so air temperature affects pressure (ideal gas). Keeps `p`/`rho`/`T` consistent. *Depends on: stable solver (1), conservation (2).*

5. **Velocity strength (Section 2)**  
   Tune `game_vel_scale` or cap output velocity so 10 kPa difference doesn’t look “huge.” *Can do after (1) so sim is stable.*

6. **Custom heat transfer and heat capacity (Section 1 item 4)**  
   Implement heat-capacity-aware particle–air transfer; use default particle heat capacity (3), then per-element where set. Feed updated `hv` into solver via (4). *Depends on: air temp in sync (4), default HC (3).*

7. **Optional later**: Pressure-dependent heat conductivity; HotAir-style direct pressure from particle temp; total-energy monitoring.

---

## 7. Code References (Current)

| Topic              | File / location                                                                 |
|--------------------|----------------------------------------------------------------------------------|
| Sync TPT → solver  | `air_solver_rusanov.cpp`: `air_solver_sync_from_tpt` (pv, vx, vy, rho; no hv)   |
| Sync solver → TPT  | `air_solver_rusanov.cpp`: `air_solver_sync_to_tpt` (writes hv from e)           |
| CFL / dt           | `air_solver_rusanov.cpp`: `max_lambda()`, `step()` (dt_cfl = CFL*dx/lam)        |
| Primitives         | `air_solver_rusanov.cpp`: `primitives_from`, `primitives_from_U4`, `sound_speed`|
| Velocity scale     | `AirSolverWrapper.h`: `game_vel_scale`; used in sync_from/sync_to               |
| Heat diffusion     | `heat.cpp`: `heat_diffusion_step`; `heat.hpp`: `k_thermal` constant             |
| Particle–air heat | `Simulation.cpp`: heat transfer with `hv`; HotAir block commented out           |
| Air update         | `Air.cpp`: `update_air()` — ensure_created, sync_from_sim, step, sync_to_sim   |

---

## 8. Summary Table

| Issue                         | Cause (current code)                                                                 | Direction for fix                                                                 |
|------------------------------|----------------------------------------------------------------------------------------|------------------------------------------------------------------------------------|
| One-way p–T: air temp doesn’t create pressure; particle temp not coupled | **Pressure → heat** is coupled (pressure creates/removes heat). **Air temp → pressure** is uncoupled (air temp isn’t creating pressure). `sync_from_tpt` ignores `hv`, so particle–air heat is overwritten; particle temp doesn’t affect air or pressure. | Use **air temp** `hv` in `sync_from_tpt` so air temp affects pressure (ideal gas) and particle–air heat feeds into solver. |
| Pressure → air T in solver  | One-way works: pressure affects heat; TPT sees it via `sync_to_tpt` (hv from e).       | No change needed for that direction; optional: pressure-dependent conductivity.   |
| Heat conductivity            | Constant `k_thermal`                                                                   | Optional: k(p, T) in heat diffusion.                                              |
| Velocity too strong          | Large pressure gradient → large m/s; same m/s → large game units via `game_vel_scale`  | Tune `game_vel_scale` and/or cap output (or solver) velocity.                     |
| 0 kPa → simulation halts     | Vacuum cell gets huge momentum → huge u → max_lambda explodes → dt → 0                | Cap velocity in `max_lambda()` (and optionally in primitives); vacuum handling.    |
| Energy loss in enclosed box  | Heat diffusion Laplacian at walls likely non-conservative; viscous BC or sync could contribute | Heat diffusion in conservative flux form (zero flux at walls); audit viscous energy at walls; optional total-energy monitor. |

This plan is based on the current codebase and is intended as a living document to be updated as changes are made.

---

## 9. Debugging 0-pressure / stuck solver

Debug prints go to **stderr** and are **always on** (no build option). You **must run the game from a terminal** to see them; if you launch from a desktop icon or file manager, stderr is not visible.

```bash
cd /path/to/universal-physics-mod
./build/powder
# or capture to a file:
./build/powder 2> air_debug.log
# then: tail -f air_debug.log
```

On first air update you should see: `[AIR] update_air: Rusanov path active (run from terminal to see logs)` and `[AIR-WRAP] air solver step() active (stderr is working)`. If you see nothing, you're not running from a terminal or air mode is off.

When the solver gets stuck, you will see:

- **`[AIR] dt tiny: lam=... dt_cfl=...`** — CFL time step is very small because `max_lambda()` is huge (one cell has huge velocity). So the sim advances almost no time per frame and appears stuck.
- **`[AIR] state INVALID at iy=... ix=... rho=... e_int=...`** — After a step, a cell failed `state_valid()` (rho &lt; rho_min or internal energy &lt; e_min). The step is rejected and we retry; if it keeps failing, we break after 5 rejects.
- **`[AIR-WRAP] step returned dt=0 (reject #n)`** — The solver rejected the step (state invalid).
- **`[AIR-WRAP] too many rejects, breaking`** — We gave up after 5 rejects in a row.
- **`[AIR-WRAP] stuck: advance=...`** — We did max_steps but advanced almost no time (dt was tiny every step).

From that you can tell whether the stuck is from **tiny dt** (lambda too large) or **rejected steps** (invalid state), and which cell (iy, ix) and which quantity (rho vs e_int) is bad.
