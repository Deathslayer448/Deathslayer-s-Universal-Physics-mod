# Pressure-difference break for walls (design and implementation)

## Problem

In TPT, elements like TUNG used to break when the **absolute** pressure in their cell reached a threshold (or when pressure **change over time** exceeded a threshold). In reality, containment fails due to **pressure difference** across the wall: 1 atm inside and 1 atm outside cancel out; vacuum inside with pressure outside (or vice versa) causes breakage. We want similar behaviour in TPT.

## Goals

- Break walls (e.g. TUNG) when the **pressure difference** across the wall exceeds a threshold, not when absolute pressure or temporal change does.
- Feed **velocity** into the criterion so that fast air or a fast particle can break the wall based on kinetic energy / momentum flux.
- Use **energy** (Joules), consistent with the solver (E = ρe + ½ρv², P, ρ, v in SI), so that in the future a particle thrown at TUNG can break it depending on velocity/energy.

## Scope

- **Per cell** (solver air cells, 4×4), not per particle. One evaluation per air cell along the wall.
- **Only cells that matter**: cells that contain at least one particle of a **breakable solid** (e.g. TUNG). Other solids can be added later.
- Particles are 1×1; air data (pv, rho, vx, vy) is per 4×4 cell.

---

## Implementation (as built)

### Option and gating

- **UI**: Solver and air → checkbox **"Pressure-difference break (TUNG)"**. Description: *"Experimental; can reduce frame rate. When on, TUNG can break from sustained pressure difference across the wall."*
- **Default**: Off (`Air::enablePressureBreak = false`). When off, `UpdatePressureBreakEnergy` is not called and TUNG does not check pressure-break energy.
- **Storage**: `Air::enablePressureBreak`; OptionsModel/OptionsController Get/Set; checkbox state synced in NotifySettingsChanged.

### Which cells

- Only air cells (iy, ix) that contain at least one TUNG particle. A single pass over particles marks `has_tung[cy*XCELLS+cx]`; accumulation runs only for those cells.

### Pressure-difference gate

- For each such cell, compute **ΔP = max(neighbor P) − min(neighbor P)** over the cell and its 8 neighbors.
- **Accumulate only when ΔP ≥ 80 kPa** (`MIN_PRESSURE_DIFFERENCE_PA = 80000` in Air.cpp). Smaller differentials (e.g. 101 vs 101.2 kPa) do not accumulate, so they do not cause break.

### Energy flux and accumulation

- **Flux** (W/m²) into the wall cell: from each of the 8 neighbors, compute **P·v_n + ½ρ v²·v_n** where v_n is the velocity component **from that neighbor toward this cell** (unit vector from (iy,ix) to (ny,nx)). Sum over neighbors; use `max(0, net_flux)` so only flux into the wall counts.
- **Units**: P in Pa, ρ in kg/m³, v in m/s (game velocity × 0.6 to match solver). Flux in W/m²; accumulation in J/m².
- **Accumulation**: `pressure_break_energy[iy][ix] += max(0, net_flux) * dt`. Run only when `enablePressureBreak` is true, after `sync_to_sim` in `update_air()`.
- **Diffusion**: A fraction (0.4) of each cell’s flux increment is added to **neighboring cells that also have TUNG** (`has_tung[ny][nx]`). This evens out accumulation along the wall and avoids checkerboard breaking; air-only cells are not written.

### Break condition (TUNG)

- When **`pressure_break_energy[cy][cx] ≥ 2 MJ/m²`** (`PRESSURE_BREAK_THRESHOLD_J_M2 = 2000000` in TUNG.cpp):
  1. Zero that cell’s `pressure_break_energy[cy][cx]`.
  2. Break **all TUNG in that single 4×4 air cell**: scan the 16 pixels in the cell with `pmap[py][px]`; for each pixel where `TYP(pmap)==PT_TUNG`, call `part_change_type` to PT_BRMT with `ctype = PT_TUNG`. This keeps the break aligned to the air grid and avoids scanning all particles (O(16) per break).

### Block air and undo

- **TUNG blocks air**: Element property **PROP_BLOCKAIR** (ElementDefs.h). TUNG, TTAN, ALMN, RSSS set it; `Air::ApproximateBlockAirMaps()` builds `bmap_blockair` from `elements[type].Properties & PROP_BLOCKAIR` (no per-type logic in Air). So pressure can differ on either side of a TUNG wall until it breaks.
- **Undo (Ctrl+Z)**: On `Simulation::Restore`, `pressure_break_energy` is cleared (`std::fill(..., 0)` in Editing.cpp) so restored TUNG does not immediately re-break.

### Constants (tunable)

| Constant | Value | Location | Purpose |
|----------|--------|----------|---------|
| MIN_PRESSURE_DIFFERENCE_PA | 80 000 | Air.cpp | Only accumulate when ΔP ≥ 80 kPa |
| diffusion_frac | 0.4 | Air.cpp | Fraction of flux increment bled to neighboring TUNG cells |
| PRESSURE_BREAK_THRESHOLD_J_M2 | 2 000 000 | TUNG.cpp | Break when accumulated energy ≥ 2 MJ/m² |
| vel_scale | 0.6f | Air.cpp | Game velocity → m/s (must match AirSolverWrapper) |

---

## Design notes (original ideas, some simplified in implementation)

### Normals and “inside” vs “outside”

The current implementation does **not** walk away from the wall to sample “bulk” P on two sides. It uses the **wall cell and its 8 neighbors** directly:

- **ΔP**: max and min of P over the cell and 8 neighbors; accumulation gated by ΔP ≥ 80 kPa.
- **Flux**: net energy flux from neighbors toward the wall cell (P·v_n + ½ρ v²·v_n), so flow into the wall from the high-pressure side naturally dominates.

So “sides” are implicit in the neighbor pressures and velocities; explicit walk/labelling was dropped for simplicity.

### Velocity in the criterion

- **Static**: pressure difference → flow toward the wall → positive v_n and flux → energy accumulates.
- **Dynamic**: fast air or impact → large v and v² → large flux in one frame → can reach threshold quickly.

So both sustained ΔP and fast impacts can break TUNG via the same energy-based rule.

### Optimisation and edge cases

- **Cost**: Only cells with at least one TUNG particle are updated. When the option is off, no accumulation and no TUNG break check.
- **Break front**: Breaking all TUNG in the single 4×4 cell (and zeroing only that cell’s energy) keeps the front aligned to the air grid and avoids single-particle teeth. Diffusion between TUNG cells smooths accumulation.
- **Particles vs air**: Currently only air flux (solver P, ρ, v) feeds accumulation. Particle–wall impact could be added later as an extra contribution to `pressure_break_energy`.

---

## Summary

| Item | Choice |
|------|--------|
| Resolution | Per air cell (4×4), not per particle |
| Which cells | Only those containing at least one TUNG particle |
| ΔP gate | Accumulate only when max(neighbor P) − min(neighbor P) ≥ 80 kPa |
| Criterion | Energy-based: accumulate flux (W/m²) × dt → J/m²; break when ≥ 2 MJ/m² |
| Velocity | Via v_n and v² in flux (P·v_n + ½ρ v²·v_n) |
| Diffusion | 0.4 × increment to neighboring TUNG cells only |
| Break action | All TUNG in that 4×4 cell → BRMT (ctype TUNG); zero that cell’s energy |
| Option | “Pressure-difference break (TUNG)” in Solver and air; default off; experimental, may reduce frame rate |
| Units | J, J/m², Pa, m/s, kg/m³ — consistent with solver |

This gives pressure-difference-driven break, velocity-dependent impact, and an energy-based rule so that both static overpressure and fast flows can break TUNG in a principled way, with a single toggle and clear performance caveat.
