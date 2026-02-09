# Plan: Weight → Mass and Heat Capacity → Specific Heat

This document is the migration plan for:
1. **Weight** → meaning **mass** (kg) per particle, with known volume so density = mass/volume is defined.
2. **Heat capacity** → **specific heat** (J/(kg·K)); heat capacity computed as `C = m × c` on the fly.

Design decision (see `SIMULATION_UNITS.md` → **Design: Heat capacity vs specific heat**): we do **not** mix the two approaches. Once mass is defined, we store **specific heat** and compute **C** everywhere.

---

## Pre-implementation: decisions locked in

Before touching code, these are fixed so we don’t block implementation:

| Topic | Decision |
|-------|----------|
| **Mass type** | `float` (was `int Weight`). Store mass in **kg**. |
| **Mass scale** | m = ρ × V_pixel (V_pixel = 1e-4 m³). Water 0.1 kg, tungsten ~1.93 kg, gases ~0.0001 kg. |
| **Per-particle mass** | Use element `Mass` only (no particle.mass override for now). |
| **Specific heat defaults** | Gas: **717.5** J/(kg·K) (air c_v). Solid/liquid: **500.0** J/(kg·K). |
| **Element default Mass** | **0.1f** kg (e.g. water-like). Replace in each element with real mass from density. |
| **NoWeightSwitching** | Keep name and UI text (“weight-based”); meaning is mass-based. No rename to NoMassSwitching. |
| **Legacy saves** | **Optional**: On load, if element “Weight” is int 1–1000, treat as old scale and convert to kg (e.g. mass_kg = old_value / 1000.0f) so old saves still load. New saves store Mass (float). If we don’t need old-save compat, we can break and document. |

**Open choice (pick one and go):**

- **Element Mass values**: (A) Bulk convert with a formula (e.g. mass_kg = old_Weight/1000.0f), then set water=0.1, tungsten=1.93, iron=0.787, gases=0.0001 etc. in key element files. (B) Assign real mass (kg) from density in every element file in one pass. Recommendation: **(A)** for speed; we can refine more elements later.

Once (A) or (B) is chosen, we’re ready to implement.

---

## Next steps (concrete order)

1. **Rename Weight → Mass**  
   - Same fixed volume per particle: **0.1 L = 1.0×10⁻⁴ m³** (from `SIMULATION_UNITS.md`).  
   - **Denser = more mass** (same volume). Store **mass in kg**: `m = ρ × V_pixel` with `V_pixel = 1e-4 m³`.  
   - **Scale**: Water (ρ ≈ 1000 kg/m³) → **0.1 kg**. Tungsten (ρ ≈ 19,250 kg/m³) → **~1.93 kg**. Gases (ρ ~ 1–2 kg/m³) → **~0.0001–0.0002 kg**.  
   - Rename the property from `Weight` to `Mass` in `Element` (and any UI/Lua) so “more mass = denser material” is clear.

2. **Heat capacity → Specific heat**  
   - Add/rename to **SpecificHeat** (J/(kg·K)) on Element (and optionally per-particle override).  
   - Remove stored heat capacity; use **C = mass × specificHeat** everywhere (heat transfer, wall loss, HUD).

3. **What else (same plan, in order)**  
   - **Constants**: Default specific heat (gas ~700–1000, solid/liquid ~400–500 J/(kg·K)); document mass scale in `SIMULATION_UNITS.md`.  
   - **Element struct**: `Mass` (float, kg), `SpecificHeat` (float, J/(kg·K)); convert all element `Weight` values to mass (kg) and all `HeatCapacity` to specific heat.  
   - **Particle**: `specificHeat` (from element or override); `get_particle_heat_capacity(i) = mass(i) * specificHeat(i)`.  
   - **Gameplay**: `can_move` (heavier mass can push lighter — same comparison with mass in kg); HCL/BLOD/STKM/WEB formulas rescaled or redefined for mass (kg).  
   - **Saves/Lua**: Legacy `heatCapacity` → convert or ignore; document Mass and SpecificHeat.

---

## 1. Goals

- **Weight**: Becomes **mass per particle** in **kg** (or a consistent scale, e.g. grams stored as kg: 0.001 = 1 g).
- **Volume**: Already defined: `V_pixel = 1.0×10⁻⁴ m³` (see `SIMULATION_UNITS.md`). Density: `ρ = m / V_pixel`.
- **Specific heat**: New stored quantity **per particle** (and/or per element as default): **J/(kg·K)**.
- **Heat capacity**: No longer stored; use `C = mass × specificHeat` everywhere heat capacity is needed.
- **Legacy**: Support loading old saves (optional: convert old `heatCapacity` to `specificHeat` using a default mass, or keep a one-time migration path).

---

## 2. Data Structure Changes

### 2.1 Particle (`Particle.h`, `Particle.cpp`)

| Current | After |
|--------|--------|
| `float heatCapacity` (J/K) | **Remove** or keep only for legacy read; add `float specificHeat` (J/(kg·K)) |
| — | Mass: use **element** `Weight` (repurposed as mass in kg) or add `float mass` if per-particle mass is needed |

**Decision**: Prefer mass from **element** `Weight` (in kg) so one source of truth. Optionally allow per-particle override (e.g. `mass > 0` on particle) later.

### 2.2 Element (`Element.h`, `Element.cpp`)

| Current | After |
|--------|--------|
| `int Weight` (dimensionless, relative) | **Rename to `Mass`** (float, kg). Fixed volume 0.1 L → m = ρ×V_pixel. Water ≈ 0.1 kg, tungsten ≈ 1.93 kg, gases ~0.0001 kg. |
| `float HeatCapacity` (J/K per particle) | **Rename to `SpecificHeat`** (J/(kg·K)). Heat capacity computed as C = Mass × SpecificHeat. |

**Naming**: Rename `Weight` → `Mass` (mass in kg); rename `HeatCapacity` → `SpecificHeat` (J/(kg·K)). More mass = denser material (same volume).

### 2.3 Constants (`ElementDefs.h`)

- Remove or repurpose `DEFAULT_GAS_HEAT_CAPACITY_J_PER_K`, `DEFAULT_SOLID_LIQUID_HEAT_CAPACITY_J_PER_K`, `DEFAULT_PARTICLE_HEAT_CAPACITY`.
- Add defaults for **specific heat** (J/(kg·K)), e.g.:
  - Gas: ~700–1000 J/(kg·K)
  - Solid/liquid: ~400–500 J/(kg·K)
- Define **mass scale** for `Weight`: e.g. 1 = 1 g → `0.001f` kg, or 100 = 100 g → `0.1f` kg. Document in `SIMULATION_UNITS.md`.

---

## 3. Codebase Checklist (everything that must be touched)

Search the repo for: `Weight`, `HeatCapacity`, `heatCapacity`, `HeatConduct` (only where heat capacity is used), and `can_move` (Weight-based).

### 3.1 Core simulation

| File | What to change |
|------|----------------|
| `src/simulation/Particle.h` | Replace or add: `heatCapacity` → `specificHeat` (and/or remove stored C). |
| `src/simulation/Particle.cpp` | Property list: expose `specificHeat` (and mass if per-particle); handle legacy `heatCapacity` in load if needed. |
| `src/simulation/Element.h` | Document `Weight` as mass (kg); add or rename `SpecificHeat` (J/(kg·K)). |
| `src/simulation/Element.cpp` | Defaults: `Weight` in kg, `SpecificHeat`; property list. |
| `src/simulation/ElementDefs.h` | Default specific heat constants; remove/repurpose heat capacity defaults. |
| `src/simulation/Simulation.cpp` | **create_part**: set `specificHeat` from element (or default); stop setting `heatCapacity`. All heat transfer: get mass (element Weight or particle), get specific heat, compute `C = m * c`; use C in Fourier/energy updates. **get_particle_heat_capacity** (or equivalent): return `mass * specificHeat`. |
| `src/simulation/Air.cpp` | Wall heat loss and any particle heat: use `C = m * specificHeat` instead of stored `heatCapacity`. |
| `src/simulation/SimulationData.cpp` | **init_can_move**: uses `elements[movingType].Weight` and `elements[destinationType].Weight`. After Weight = mass (kg), comparison (heavier can push lighter) still works if all masses use same scale. Ensure no code assumes Weight is 1–100; if so, rescale or adapt. |

### 3.2 Element definitions (all set `Weight` and some set `HeatCapacity`)

- **Weight**: Every element sets `Weight = N`. Migration: define conversion from current scale to kg (e.g. current 100 → 0.1 kg). Bulk replace or script: e.g. `Weight = 100` → `Weight = 0.1f` (if 100 = 100 g), or keep integer and interpret as grams (then mass_kg = Weight/1000.0f).
- **HeatCapacity** → **SpecificHeat**: Elements that set `HeatCapacity` (e.g. TTAN 0.52f, IRON 0.45f, BASE 1.5f) must get **specific heat** values. Since C was per particle, and we had an implicit mass (e.g. 1 g): c = C/m = 0.52/0.001 = 520 J/(kg·K) for TTAN. So:
  - TTAN: `HeatCapacity = 0.52f` → `SpecificHeat = 520.0f` (J/(kg·K))
  - IRON: `HeatCapacity = 0.45f` → `SpecificHeat = 450.0f`
  - BASE: `HeatCapacity = 1.5f` → choose mass for BASE or use 500 J/(kg·K) as default.
- **Files to update** (grep found these and more): `IRON.cpp`, `TTAN.cpp`, `BASE.cpp`, `Element.cpp` (default), and every other element that sets `Weight` (all of them). For HeatCapacity → SpecificHeat: only elements with explicit HeatCapacity need numeric conversion; others use default specific heat.

### 3.3 Heat transfer and energy

| File | What to change |
|------|----------------|
| `Simulation.cpp` | Particle–air heat transfer: use `C_part = mass(i) * specificHeat(i)`, same for air. Particle–particle: same. LAVA/ctype: use ctype’s specific heat and (if available) mass for that particle. |
| `Air.cpp` | Any use of `sim.parts[k].heatCapacity` or element `HeatCapacity`: replace with `C = mass * specificHeat`. |
| `src/simulation/elements/PIPE.cpp` | Uses `elements[pipe->type].HeatCapacity` and `elements[part->type].HeatCapacity` for heat balance. Replace with specific heat and mass: C = m*c for each. |

### 3.4 GUI and HUD

| File | What to change |
|------|----------------|
| `src/gui/game/GameView.cpp` | `sd.elements[type].HeatCapacity` (e.g. for HUD or tooltip): show or use specific heat; if showing energy, use `C = m*c` and show E = C*T. |
| `src/gui/game/GameModel.cpp` | NoWeightSwitching: no change (option is about weight-based **switching**, not the unit of Weight). |
| Any HUD that shows “heat capacity” or “energy” | Use computed C = m*c; display specific heat (J/(kg·K)) if needed. |

### 3.5 Gameplay logic that uses Weight

| File | What to change |
|------|----------------|
| `SimulationData.cpp` | `init_can_move`: `elements[movingType].Weight <= elements[destinationType].Weight` → still works if Weight is mass in kg (heavier mass can push lighter). Ensure no integer overflow; if Weight is float, comparison is fine. |
| `src/simulation/elements/HCL.cpp` | Weight switching: `elements[parts[i].type].Weight - pow(elements[TYP(r)].Weight, 2) / 10.0f` — formula is in “relative” units. After migration, either keep a **relative weight** for this formula (e.g. derived from mass) or redefine formula in mass (kg). Same for BLOD. |
| `src/simulation/elements/BLOD.cpp` | Same as HCL: weight-based swap chance. |
| `src/simulation/elements/STKM.cpp` | `parts[i].vx -= (elements[(int)playerp->elem].Weight*parts[np].vx)/1000` — momentum-like. After Weight = mass, this becomes mass in kg; adjust divisor (1000) so behaviour is preserved or tuned. |
| `src/simulation/elements/WEB.cpp` | `elements[rt].Weight < 50` — threshold. After migration, use mass threshold in kg (e.g. 0.05 kg). |

### 3.6 Save format and Lua

- **Save**: If particle stores `heatCapacity`, either drop it and rely on element default specific heat, or add a one-time conversion when loading (old heatCapacity + default mass → specificHeat = heatCapacity/mass).
- **Lua**: Any API that exposes `Weight` or `HeatCapacity`/`heatCapacity`: document new meaning (mass in kg, specific heat in J/(kg·K)); add `specificHeat` and optionally `mass` if per-particle.

### 3.7 Documentation

- `SIMULATION_UNITS.md`: Already updated with design (Path B, specific heat). Add final mass scale and specific heat defaults after decisions.
- `AIR_SOLVER_IMPROVEMENT_PLAN.md`, `TPT_AIR_REPLACEMENT_PLAN.md`, `MISSING_ITEMS.md`: Where they mention HeatCapacity or particle heat capacity, point to specific heat and C = m*c.

---

## 4. Order of Work (phases)

1. **Define scale and constants**  
   - Decide mass scale for `Weight` (e.g. 1 = 1 g).  
   - Add default specific heat constants (gas, solid/liquid).  
   - Document in `SIMULATION_UNITS.md`.

2. **Element struct and defaults**  
   - Add `SpecificHeat` (or rename `HeatCapacity` and change units).  
   - In `Element.cpp`, set default specific heat.  
   - Convert existing `HeatCapacity` values to `SpecificHeat` in element files (TTAN, IRON, BASE, etc.).

3. **Particle struct**  
   - Add `specificHeat` (or remove `heatCapacity` and use element only; optionally keep per-particle override).  
   - In `create_part`, set `specificHeat` from element default.  
   - Property tool: expose specific heat (and mass if per-particle).

4. **Heat capacity usage**  
   - Introduce helper: `get_particle_heat_capacity(i) = mass(i) * specificHeat(i)`.  
   - Replace every use of stored `heatCapacity` and element `HeatCapacity` in heat transfer with this (Simulation.cpp, Air.cpp, PIPE.cpp, GameView, etc.).

5. **Weight → mass**  
   - Change semantic of `Weight` to mass (kg); rescale all element `Weight` values to the new scale (bulk script or search/replace).  
   - Fix `can_move` and any gameplay formula that assumed old Weight range (HCL, BLOD, STKM, WEB) to use new mass scale or a derived “relative weight” for chances.

6. **Legacy saves**  
   - If saves store `heatCapacity`: on load, either ignore and use element specific heat, or set `specificHeat = heatCapacity / default_mass` for that type.

7. **Cleanup**  
   - Remove or deprecate `heatCapacity` from Particle and `HeatCapacity` from Element (if replaced by SpecificHeat).  
   - Remove default heat capacity constants that are no longer used.

---

## 5. Summary Table (where Weight and heat capacity appear)

| Area | Files | Action |
|------|--------|--------|
| Particle | Particle.h, Particle.cpp | heatCapacity → specificHeat (or remove); property list |
| Element | Element.h, Element.cpp | Weight = mass (kg); HeatCapacity → SpecificHeat (J/(kg·K)) |
| Constants | ElementDefs.h | Default specific heat; mass scale |
| Heat transfer | Simulation.cpp, Air.cpp | C = m*c everywhere |
| PIPE/contents | PIPE.cpp | Heat balance using mass and specific heat |
| can_move | SimulationData.cpp | Keep comparison; ensure scale consistent |
| Weight-based gameplay | HCL.cpp, BLOD.cpp, STKM.cpp, WEB.cpp | Rescale or redefine formulas for mass (kg) |
| GUI | GameView.cpp | Show/use specific heat and C = m*c |
| Saves / Lua | Save format, Lua API | Optional conversion; document new fields |
| Docs | SIMULATION_UNITS.md, etc. | Final scale and defaults |

This plan covers the full codebase impact for Weight → mass and heat capacity → specific heat; use it as a checklist when implementing.
