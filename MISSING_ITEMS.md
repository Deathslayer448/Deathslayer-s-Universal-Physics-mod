# Missing Items from Air Solver Plans

## ✅ Already Done

### AIR_SOLVER_IMPROVEMENT_PLAN.md - Phase 1 (Foundations)
1. ✅ **Vacuum / 0 kPa bug** - Velocity capping in `max_lambda()` implemented (v_max_mult = 5.0× sound speed)
2. ✅ **Default particle heat capacity** - `DEFAULT_PARTICLE_HEAT_CAPACITY = 1.0f` defined and used in Simulation.cpp
3. ✅ **Air temp → pressure coupling** - `sync_from_tpt` uses `hv` to compute `rho = p/(R*T)` and `e = c_v*T`, so air temperature affects pressure

### TPT_AIR_REPLACEMENT_PLAN.md - Phases 1-3
- ✅ Phase 1 (Prep): Solver integrated, wrapper exists
- ✅ Phase 2 (Replace air evolution): `update_air()` uses Rusanov solver
- ✅ Phase 3 (Particle-air coupling): Works with solver state

---

## ❌ Still Missing

### AIR_SOLVER_IMPROVEMENT_PLAN.md - Phase 1 (Foundations)

#### 1. Energy loss in enclosed system (Section 5)
**Status**: Partially done - heat diffusion uses flux form, but needs verification
- ✅ Heat diffusion uses conservative flux form (`heat.cpp`)
- ❓ **Needs verification**: Ensure total energy is conserved in closed system (no leaks)
- **Action**: Add total-energy monitoring/debug output to verify conservation

#### 2. Velocity strength tuning (Section 2)
**Status**: Not tuned yet
- Current `game_vel_scale = 0.06`
- **Action**: Test with 10 kPa pressure difference and tune `game_vel_scale` or add velocity cap in `sync_to_tpt` if needed

### AIR_SOLVER_IMPROVEMENT_PLAN.md - Phase 2 (Dependencies)

#### 3. Custom heat transfer and heat capacity (Section 1, item 4)
**Status**: Not implemented
- Current: Particle-air heat transfer uses `hv` but ignores air heat capacity (ρ·c_v)
- **Action**: Implement heat-capacity-aware transfer:
  - Use air heat capacity: `ρ·c_v·V_cell` per cell
  - Use particle heat capacity: `DEFAULT_PARTICLE_HEAT_CAPACITY` or per-element `HeatCapacity`
  - Feed updated `hv` into solver (already works since `sync_from_tpt` uses `hv`)

#### 4. Advanced HUD (F3): air energy and particle energy (Section 0)
**Status**: Not implemented
- **Action**: Add to F3 HUD:
  - `E_air`: Total energy of air cell (internal + kinetic) in J
  - `E_part`: Particle thermal energy = `HeatCapacity × temperature`

#### 5. Optional: Pressure-dependent heat conductivity (Section 1, item 3)
**Status**: Not implemented
- Current: Constant `k_thermal` in `heat.hpp`
- **Action**: Replace with `k(p, T)` function

### TPT_AIR_REPLACEMENT_PLAN.md - Phase 4 (Cleanup)

#### 6. Remove dead code (Section 9)
**Status**: Legacy code still present
- **Location**: `src/simulation/Air.cpp` lines 336-917 (`#if 0` block)
- **Action**: Delete the `#if 0 // USE_LEGACY_AIR_PHYSICS` block (580+ lines)

#### 7. Remove unused buffers (Section 9)
**Status**: May still be used
- **Buffers**: `ovx`, `ovy`, `opv`, `ohv` in `Air.h`
- **Action**: Check if still used; if not, remove or repurpose

#### 8. Constants repurpose/document (Section 10)
**Status**: Not done
- **Constants**: `AIR_TSTEPP`, `AIR_TSTEPV`, `AIR_VADV`, etc. in `SimulationConfig.h`
- **Action**: Document which are still used vs legacy, or repurpose for solver path

#### 9. Performance profiling (Section 11)
**Status**: Not done
- **Action**: Profile one frame; tune `airSolverStepsPerFrame` or CFL if needed

---

## Priority Order (Recommended)

1. **Remove dead code** (#6) - Cleanup, low risk
2. **Energy conservation verification** (#1) - Foundation, verify it works
3. **Custom heat transfer** (#3) - Important for proper physics
4. **Advanced HUD** (#4) - User-facing feature
5. **Velocity strength tuning** (#2) - Gameplay tuning
6. **Unused buffers** (#7) - Cleanup
7. **Constants documentation** (#8) - Documentation
8. **Performance profiling** (#9) - Optimization
9. **Pressure-dependent conductivity** (#5) - Optional enhancement
