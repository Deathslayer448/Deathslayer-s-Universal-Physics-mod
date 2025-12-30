# Integrated Physics System Redesign - Real Physics Implementation

## Overview
This document outlines the plan to replace the current simplified, disconnected physics systems (pressure, velocity, gravity, temperature) with a unified, physically accurate implementation based on real fluid dynamics equations. All systems will be properly coupled as they are in real physics, while maintaining 60 FPS performance and adding configurability for cell size and atmospheric pressure.

**Key Insight**: In real physics, pressure, velocity, gravity, temperature, and density are all interconnected. The current implementation treats them as separate systems, which leads to unrealistic behavior.

## What Will Be Redesigned

### Systems to Integrate:
1. **Pressure System** - Currently simplified, will use ideal gas law
2. **Velocity System** - Currently decoupled, will use Navier-Stokes momentum equation
3. **Gravity System** - Currently only affects particles, will affect fluid (buoyancy)
4. **Temperature System** - Currently loosely coupled, will be fully integrated
5. **Mass Conservation** - Currently missing, will track particle contributions
6. **Particle Stacking & Mapping** - Currently broken (only one particle accessible per position), will use proper multi-particle mapping

### Key Improvements:
- **Proper Coupling**: All systems affect each other as in real physics
- **Buoyancy**: Denser regions fall, lighter regions rise (via gravity + density)
- **Temperature-Pressure Coupling**: Hot air expands (ideal gas law)
- **Particle-Fluid Interaction**: Particles contribute to and are affected by fluid
- **Mass Conservation**: Particle creation/destruction properly affects fluid density
- **Configurable Cell Size**: Runtime adjustable for performance/accuracy tradeoff
- **Atmospheric Pressure**: Optional baseline pressure for realistic behavior

---

## Current System Analysis

### Current Implementation (Disconnected Systems)

#### Pressure/Velocity System
- **Location**: `src/simulation/Air.cpp` and `src/simulation/Air.h`
- **Grid Resolution**: Fixed `CELL = 4` (defined in `Config.template.h`)
- **Grid Size**: `(XRES/CELL) × (YRES/CELL)` = `153 × 96` cells (for 612×384 resolution)
- **Current Approach**: Simplified pressure-velocity coupling with magic constants:
  - `AIR_TSTEPP = 0.3f` - Pressure time step
  - `AIR_TSTEPV = 0.4f` - Velocity time step
  - `AIR_VADV = 0.3f` - Velocity advection
  - `AIR_VLOSS = 0.999f` - Velocity loss
  - `AIR_PLOSS = 0.9999f` - Pressure loss

#### Gravity System
- **Location**: `src/simulation/Gravity.cpp` and `src/simulation/Gravity.h`
- **Current Approach**: 
  - Separate gravity field calculation (Newtonian gravity)
  - Applied directly to particles, not integrated with fluid dynamics
  - Multiple modes (vertical, radial, off) but not physically consistent
  - Gravity affects particles but not the fluid field itself

#### Temperature System
- **Location**: `src/simulation/Simulation.cpp` (heat conduction)
- **Current Approach**:
  - Particle-to-particle heat conduction
  - Some coupling with pressure (`pv[y/CELL][x/CELL] += (pt-hv[y/CELL][x/CELL])*0.004`)
  - Ambient heat map (`hv`) but not properly integrated with pressure
  - Temperature affects pressure only in a simplified way

#### Velocity System
- **Current Approach**:
  - Particles have individual velocities (`parts[i].vx`, `parts[i].vy`)
  - Air velocity field (`vx`, `vy`) affects particles but not vice versa
  - No proper momentum conservation
  - Particles don't contribute to the fluid field

### Current Limitations

1. **Disconnected Systems**: Pressure, velocity, gravity, and temperature are treated separately
2. **Not Physically Accurate**: Uses simplified models, not based on real fluid dynamics
3. **Fixed Cell Size**: Hardcoded `CELL = 4`, cannot be changed at runtime
4. **No Atmospheric Pressure**: No concept of baseline atmospheric pressure
5. **Magic Constants**: Arbitrary values not derived from physical properties
6. **No Mass Conservation**: Particles don't contribute to fluid density/mass
7. **No Momentum Conservation**: Particle interactions don't affect fluid velocity field
8. **Simplified Model**: Doesn't account for:
   - Density variations
   - Viscosity
   - Proper pressure gradients
   - Ideal gas law relationships
   - Temperature-pressure-density coupling
   - Buoyancy (density differences in gravity)
   - Particle-fluid interactions
9. **Broken Particle Stacking**: 
   - Only one particle accessible per position via `pmap[y][x]`
   - Stacked particles are invisible to neighbor lookups
   - Causes incorrect interactions when particles stack
   - No proper handling of multiple particles at same position
9. **Broken Particle Stacking**: 
   - Only one particle accessible per position via `pmap[y][x]`
   - Stacked particles are invisible to neighbor lookups
   - Causes incorrect interactions when particles stack
   - No proper handling of multiple particles at same position

---

## Real Physics Approach: Integrated System

### Core Equations: Navier-Stokes for Compressible Flow with Coupling

The system will be based on the **Navier-Stokes equations** for compressible fluid flow, with proper coupling between all systems:

#### 1. Continuity Equation (Mass Conservation)
```
∂ρ/∂t + ∇·(ρv) = S_mass
```
Where:
- `ρ` = density
- `v` = velocity vector
- `t` = time
- `S_mass` = mass source term (from particles)

#### 2. Momentum Equation (with Gravity and Buoyancy)
```
∂(ρv)/∂t + ∇·(ρv⊗v) = -∇P + μ∇²v + ρg + S_momentum
```
Where:
- `P` = pressure
- `μ` = dynamic viscosity
- `g` = gravity vector (affects fluid directly)
- `S_momentum` = momentum source from particles

**Key**: Gravity term `ρg` creates buoyancy - denser regions fall, lighter regions rise.

#### 3. Energy Equation (Temperature Evolution)
```
∂(ρe)/∂t + ∇·(ρev) = -P∇·v + k∇²T + S_energy
```
Where:
- `e` = specific internal energy
- `T` = temperature
- `k` = thermal conductivity
- `S_energy` = energy source from particles

#### 4. Ideal Gas Law (State Equation - Couples Everything)
```
P = ρRT
```
Where:
- `R` = specific gas constant
- `T` = temperature

**Key Coupling**: 
- Temperature → Pressure (via ideal gas law)
- Pressure → Velocity (via pressure gradient)
- Density → Buoyancy (via gravity term)
- Velocity → Density (via continuity)
- Particles → All fields (via source terms)

#### 5. Particle-Fluid Interaction
Particles contribute to the fluid field:
- **Mass**: `S_mass = Σ(particle_mass * δ(x - x_particle))`
- **Momentum**: `S_momentum = Σ(particle_mass * particle_velocity * δ(x - x_particle))`
- **Energy**: `S_energy = Σ(particle_energy * δ(x - x_particle))`

And particles are affected by the fluid:
- **Drag**: `F_drag = -C_d * ρ * |v_relative|² * A`
- **Buoyancy**: `F_buoyancy = (ρ_fluid - ρ_particle) * g * V`
- **Pressure**: `F_pressure = -∇P * V_particle`

### Simplified 2D Implementation

For real-time performance, we'll use a **semi-implicit method**:

1. **Pressure Calculation** (from density and temperature):
   ```
   P[i,j] = ρ[i,j] * R * T[i,j] + P_atmospheric
   ```

2. **Density Update** (from continuity):
   ```
   ρ_new = ρ_old - dt * ∇·(ρv)
   ```

3. **Velocity Update** (from momentum):
   ```
   v_new = v_old - dt * (v·∇)v - dt/ρ * ∇P + dt * ν∇²v + dt * g
   ```

Where:
- `dt` = time step (frame time)
- `ν = μ/ρ` = kinematic viscosity
- `P_atmospheric` = base atmospheric pressure (configurable)

---

## Implementation Plan

### Phase 1: Infrastructure Changes

#### 1.1 Make CELL Size Configurable
**Files to Modify:**
- `src/Config.template.h` - Change `#define CELL 4` to runtime variable
- `src/simulation/Air.h` - Add `int cellSize` member
- `src/simulation/Simulation.h` - Add cell size configuration
- `src/gui/options/` - Add UI control for cell size

**Implementation:**
```cpp
// In Air.h
class Air {
    int cellSize;  // Runtime configurable (default: 4)
    int gridWidth, gridHeight;  // Calculated from XRES/cellSize, YRES/cellSize
    
    // Dynamic arrays instead of fixed-size
    float** vx;
    float** vy;
    float** pv;  // pressure
    float** rho; // density (NEW)
    // ... etc
};
```

**Challenges:**
- Dynamic memory allocation for arrays
- Recalculation of grid when cell size changes
- Save file compatibility (need version bump)

#### 1.2 Add Atmospheric Pressure Toggle
**Files to Modify:**
- `src/simulation/Air.h` - Add `float atmosphericPressure` and `bool enableAtmosphericPressure`
- `src/gui/options/OptionsView.cpp` - Add checkbox/toggle
- `src/gui/options/OptionsModel.cpp` - Add getter/setter

**Implementation:**
```cpp
class Air {
    float atmosphericPressure;  // Default: 101325 Pa (1 atm)
    bool enableAtmosphericPressure;
    
    // In pressure calculation:
    float GetPressure(int x, int y) {
        if (enableAtmosphericPressure)
            return rho[x][y] * R * T[x][y] + atmosphericPressure;
        else
            return rho[x][y] * R * T[x][y];
    }
};
```

---

### Phase 2: Physics Implementation

#### 2.1 Add Density Field
**New Data Structure:**
- Add `float rho[YRES/CELL][XRES/CELL]` - density field
- Initialize to standard air density: `1.225 kg/m³` (at sea level, 15°C)

#### 2.2 Rewrite Pressure Calculation
**Current (simplified):**
```cpp
dp += vx[y][x-1] - vx[y][x];
dp += vy[y-1][x] - vy[y][x];
pv[y][x] += dp*AIR_TSTEPP;
```

**New (physics-based):**
```cpp
// Calculate pressure from ideal gas law
float temperature = hv[y][x];  // Use existing heat map
float pressure = rho[y][x] * GAS_CONSTANT * temperature;
if (enableAtmosphericPressure)
    pressure += atmosphericPressure;
pv[y][x] = pressure;
```

#### 2.3 Rewrite Velocity Update
**Current (simplified):**
```cpp
dx += pv[y][x] - pv[y][x+1];
dy += pv[y][x] - pv[y+1][x];
vx[y][x] += dx*AIR_TSTEPV;
vy[y][x] += dy*AIR_TSTEPV;
```

**New (physics-based):**
```cpp
// Pressure gradient force
float dpdx = (pv[y][x+1] - pv[y][x-1]) / (2.0f * cellSize);
float dpdy = (pv[y+1][x] - pv[y-1][x]) / (2.0f * cellSize);
float forceX = -dpdx / rho[y][x];
float forceY = -dpdy / rho[y][x];

// Viscous term (simplified Laplacian)
float viscX = VISCOSITY * Laplacian2D(vx, x, y);
float viscY = VISCOSITY * Laplacian2D(vy, x, y);

// Advection (v·∇)v
float advX = AdvectionX(vx, vy, x, y);
float advY = AdvectionY(vx, vy, x, y);

// Update velocity
vx[y][x] += dt * (forceX + viscX - advX);
vy[y][x] += dt * (forceY + viscY - advY);
```

#### 2.4 Add Density Update (with Particle Contributions)
**New Function:**
```cpp
void Air::update_density() {
    // First, accumulate particle contributions to density
    for (int i = 0; i < NPART; i++) {
        if (!parts[i].type) continue;
        int gx = parts[i].x / cellSize;
        int gy = parts[i].y / cellSize;
        if (gx >= 0 && gx < gridWidth && gy >= 0 && gy < gridHeight) {
            float particle_mass = elements[parts[i].type].Weight;
            // Distribute mass to grid cell (simple nearest-neighbor for now)
            rho[gy][gx] += particle_mass * PARTICLE_TO_FLUID_MASS_FACTOR;
        }
    }
    
    // Then update density from continuity equation
    for (int y = 1; y < gridHeight-1; y++) {
        for (int x = 1; x < gridWidth-1; x++) {
            // Continuity equation: ∂ρ/∂t = -∇·(ρv) + S_mass
            float div_rhov_x = (rho[y][x+1]*vx[y][x+1] - rho[y][x-1]*vx[y][x-1]) / (2.0f * cellSize);
            float div_rhov_y = (rho[y+1][x]*vy[y+1][x] - rho[y-1][x]*vy[y-1][x]) / (2.0f * cellSize);
            
            rho[y][x] -= dt * (div_rhov_x + div_rhov_y);
            
            // Clamp to reasonable values
            rho[y][x] = std::max(0.1f, std::min(10.0f, rho[y][x]));
        }
    }
}
```

#### 2.5 Integrate Gravity into Velocity Update
**Current**: Gravity applied only to particles, not fluid.

**New**: Gravity affects fluid directly, creating buoyancy:
```cpp
// In velocity update, add gravity term
float gravityX = 0.0f;
float gravityY = 0.0f;

// Get gravity from gravity field (Newtonian gravity)
if (sim.gravityMode != 1) {  // Not "off"
    gravityX = sim.gravx[(y*gridWidth)+x];
    gravityY = sim.gravy[(y*gridWidth)+x];
    
    // Add constant gravity if enabled
    if (sim.gravityMode == 0) {  // Vertical gravity
        gravityY += STANDARD_GRAVITY;
    }
}

// Buoyancy: denser regions fall, lighter regions rise
// F = ρ * g (gravity force per unit volume)
float buoyancyX = rho[y][x] * gravityX;
float buoyancyY = rho[y][x] * gravityY;

// Add to velocity update
vx[y][x] += dt * buoyancyX;
vy[y][x] += dt * buoyancyY;
```

#### 2.6 Integrate Temperature into Pressure and Density
**Current**: Temperature affects pressure only in simplified way.

**New**: Proper ideal gas law coupling:
```cpp
void Air::update_pressure_from_temperature() {
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            // Ideal gas law: P = ρRT
            // Temperature in Kelvin (convert from current units)
            float T_kelvin = hv[y][x] + 273.15f;  // Assuming hv is in Celsius
            
            // Calculate pressure from density and temperature
            float pressure = rho[y][x] * GAS_CONSTANT * T_kelvin;
            
            if (enableAtmosphericPressure) {
                pressure += atmosphericPressure;
            }
            
            pv[y][x] = pressure;
            
            // Temperature also affects density (for constant pressure regions)
            // But we'll let continuity equation handle density changes
        }
    }
}
```

#### 2.7 Add Particle-Fluid Momentum Exchange (WITH CONSERVATION)
**New Function**: Particles contribute momentum to fluid, fluid affects particles:
**CRITICAL**: All operations must conserve mass and momentum!

```cpp
void Air::update_particle_fluid_interaction() {
    // Step 1: Particles contribute momentum to fluid
    // IMPORTANT: Track mass and momentum contributions to ensure conservation
    for (int i = 0; i < NPART; i++) {
        if (!parts[i].type]) continue;
        int gx = parts[i].x / cellSize;
        int gy = parts[i].y / cellSize;
        if (gx >= 0 && gx < gridWidth && gy >= 0 && gy < gridHeight) {
            float particle_mass = elements[parts[i].type].Weight;
            
            // CONSERVATION: Add mass contribution
            rho[gy][gx] += particle_mass * PARTICLE_TO_FLUID_MASS_FACTOR;
            
            // CONSERVATION: Add momentum contribution (mass * velocity)
            float momentum_x = particle_mass * parts[i].vx * PARTICLE_TO_FLUID_MOMENTUM_FACTOR;
            float momentum_y = particle_mass * parts[i].vy * PARTICLE_TO_FLUID_MOMENTUM_FACTOR;
            
            // Distribute to fluid (simplified: nearest cell)
            // Convert momentum to velocity: v = p / ρ
            vx[gy][gx] += momentum_x / (rho[gy][gx] + 0.001f);  // Avoid division by zero
            vy[gy][gx] += momentum_y / (rho[gy][gx] + 0.001f);
        }
    }
    
    // Step 2: Fluid affects particles (drag, buoyancy, pressure)
    // IMPORTANT: When fluid affects particles, momentum must be transferred back
    for (int i = 0; i < NPART; i++) {
        if (!parts[i].type) continue;
        int gx = parts[i].x / cellSize;
        int gy = parts[i].y / cellSize;
        if (gx >= 0 && gx < gridWidth && gy >= 0 && gy < gridHeight) {
            float particle_mass = elements[parts[i].type].Weight;
            float old_vx = parts[i].vx;
            float old_vy = parts[i].vy;
            
            // Drag force: F = -C_d * ρ * |v_rel|² * A
            float v_rel_x = parts[i].vx - vx[gy][gx];
            float v_rel_y = parts[i].vy - vy[gy][gx];
            float v_rel_mag = sqrtf(v_rel_x*v_rel_x + v_rel_y*v_rel_y);
            
            float drag_coeff = elements[parts[i].type].AirDrag;
            float drag_force = -drag_coeff * rho[gy][gx] * v_rel_mag * v_rel_mag;
            
            float delta_vx = 0.0f, delta_vy = 0.0f;
            if (v_rel_mag > 0.001f) {
                delta_vx = drag_force * v_rel_x / v_rel_mag * dt;
                delta_vy = drag_force * v_rel_y / v_rel_mag * dt;
                parts[i].vx += delta_vx;
                parts[i].vy += delta_vy;
            }
            
            // Buoyancy: F = (ρ_fluid - ρ_particle) * g * V
            float particle_density = elements[parts[i].type].Weight;
            float buoyancy = (rho[gy][gx] - particle_density) * STANDARD_GRAVITY;
            float delta_vy_buoy = buoyancy * dt;
            parts[i].vy += delta_vy_buoy;
            delta_vy += delta_vy_buoy;
            
            // Pressure gradient force (for compressible particles)
            if (elements[parts[i].type].Properties & PROP_GAS) {
                float dpdx = (pv[gy][gx+1] - pv[gy][gx-1]) / (2.0f * cellSize);
                float dpdy = (pv[gy+1][gx] - pv[gy-1][gx]) / (2.0f * cellSize);
                float delta_vx_p = -dpdx / particle_density * dt;
                float delta_vy_p = -dpdy / particle_density * dt;
                parts[i].vx += delta_vx_p;
                parts[i].vy += delta_vy_p;
                delta_vx += delta_vx_p;
                delta_vy += delta_vy_p;
            }
            
            // CONSERVATION: Transfer momentum back to fluid
            // When particle velocity changes, fluid must receive opposite momentum
            float momentum_transfer_x = particle_mass * (parts[i].vx - old_vx);
            float momentum_transfer_y = particle_mass * (parts[i].vy - old_vy);
            
            // Apply opposite momentum to fluid (Newton's 3rd law)
            vx[gy][gx] -= momentum_transfer_x / (rho[gy][gx] + 0.001f);
            vy[gy][gx] -= momentum_transfer_y / (rho[gy][gx] + 0.001f);
        }
    }
}
```

**Conservation Requirements:**
1. **Mass Conservation**: 
   - When particle is created: `total_mass += particle_mass`
   - When particle is destroyed: `total_mass -= particle_mass`
   - Fluid density must account for all particles in cell

2. **Momentum Conservation**:
   - When particle velocity changes: `fluid_momentum -= particle_mass * delta_velocity`
   - When fluid affects particle: `particle_momentum += force * dt`, `fluid_momentum -= force * dt`
   - Total momentum: `Σ(particle_mass * particle_velocity) + Σ(fluid_density * fluid_velocity * volume)` = constant

3. **Energy Conservation** (for temperature):
   - When particle temperature changes: `fluid_energy -= particle_mass * C_v * delta_T`
   - When fluid affects particle: `particle_energy += heat_transfer`, `fluid_energy -= heat_transfer`

---

### Phase 3: Performance Optimization

#### 3.1 Time Step Management
**Challenge**: Real physics requires smaller time steps for stability.

**Solution**: Use **adaptive time stepping** or **sub-stepping**:
```cpp
void Air::update_air() {
    const float max_dt = 1.0f / 60.0f;  // Max 1 frame
    const float physics_dt = 0.001f;    // Physics time step
    
    int steps = (int)(max_dt / physics_dt);
    steps = std::max(1, std::min(10, steps));  // Limit sub-steps
    
    for (int i = 0; i < steps; i++) {
        update_density();
        update_pressure();
        update_velocity();
    }
}
```

#### 3.2 Spatial Optimization
- **Multi-threading**: Process grid in chunks (OpenMP or std::thread)
- **SIMD**: Use SSE/AVX for vectorized operations
- **Reduced Precision**: Use `float` instead of `double` (already done)
- **Conditional Updates**: Skip empty regions

#### 3.3 Cell Size Impact on Performance
- **CELL = 1**: Highest accuracy, ~4× slower (153×96 = 14,688 cells)
- **CELL = 2**: Good accuracy, ~2× slower (306×192 = 58,752 cells)
- **CELL = 4**: Current default, baseline (153×96 = 14,688 cells)
- **CELL = 8**: Faster, lower accuracy (76×48 = 3,648 cells)

**Recommendation**: Allow CELL = 1, 2, 4, 8 with performance warning for CELL = 1.

---

### Phase 4: UI Integration

#### 4.1 Cell Size Control
**Location**: Options menu → Physics tab

**UI Elements:**
- Dropdown or slider: "Pressure Grid Size"
- Options: 1, 2, 4, 8
- Warning label: "Smaller = more accurate but slower"
- Real-time preview of grid (optional)

#### 4.2 Atmospheric Pressure Toggle
**Location**: Options menu → Physics tab

**UI Elements:**
- Checkbox: "Enable Atmospheric Pressure"
- Input field: "Atmospheric Pressure (Pa)" (default: 101325)
- Tooltip: "Adds base pressure to all cells (1 atm = 101325 Pa)"

#### 4.3 Performance Monitor
**New Feature**: Show FPS impact
- Display current pressure update time
- Warning if below 60 FPS
- Suggestion to increase cell size

---

## Physical Constants

```cpp
// Physical constants for air
const float GAS_CONSTANT = 287.058f;        // J/(kg·K) for dry air
const float STANDARD_DENSITY = 1.225f;     // kg/m³ at sea level, 15°C
const float STANDARD_PRESSURE = 101325.0f; // Pa (1 atmosphere)
const float KINEMATIC_VISCOSITY = 1.48e-5f; // m²/s at 15°C
const float DYNAMIC_VISCOSITY = 1.81e-5f;  // Pa·s at 15°C
```

---

## Implementation Steps (Detailed - Integrated System)

### Step 1: Make CELL Runtime Configurable
1. Remove `#define CELL 4` from `Config.template.h`
2. Add `int cellSize` to `Air` class
3. Convert fixed arrays to dynamic allocation
4. Update all `XRES/CELL` and `YRES/CELL` references
5. Add cell size getter/setter
6. Test with different cell sizes

### Step 2: Add Density Field
1. Add `float** rho` to `Air` class
2. Initialize in constructor to standard air density
3. Add `update_density()` function with particle contributions
4. Integrate into main update loop

### Step 3: Integrate Temperature System
1. Modify temperature update to affect pressure via ideal gas law
2. Add temperature advection (temperature moves with fluid)
3. Add thermal diffusion term
4. Couple temperature with density changes

### Step 4: Rewrite Pressure Calculation (Coupled)
1. Replace simplified pressure update with ideal gas law: `P = ρRT`
2. Add atmospheric pressure option
3. Ensure pressure updates after density and temperature
4. Test pressure gradients

### Step 5: Integrate Gravity into Fluid System
1. Add gravity term to velocity update: `+ dt * ρ * g`
2. Implement buoyancy (denser regions fall, lighter rise)
3. Connect gravity field to fluid velocity
4. Test buoyancy effects

### Step 6: Rewrite Velocity Update (Fully Integrated)
1. Implement pressure gradient force: `-dt/ρ * ∇P`
2. Add viscous term: `+ dt * ν∇²v`
3. Add advection term: `- dt * (v·∇)v`
4. Add gravity/buoyancy term: `+ dt * ρ * g`
5. Add particle momentum sources
6. Test velocity propagation

### Step 7: Add Particle-Fluid Interactions (WITH CONSERVATION)
1. **Particles → Fluid**: Add mass, momentum, energy sources
   - **CRITICAL**: Track all mass contributions to fluid density
   - **CRITICAL**: Track all momentum contributions (mass × velocity)
   - **CRITICAL**: Track all energy contributions (for temperature)
2. **Fluid → Particles**: Add drag, buoyancy, pressure forces
   - **CRITICAL**: When fluid affects particle, transfer opposite momentum back to fluid
   - **CRITICAL**: Use Newton's 3rd law: `fluid_momentum_change = -particle_momentum_change`
3. **Implement Conservation Checks**:
   - Verify: `Σ(particle_mass) + Σ(fluid_density × volume)` = constant
   - Verify: `Σ(particle_mass × particle_velocity) + Σ(fluid_density × fluid_velocity × volume)` = constant
   - Verify: `Σ(particle_energy) + Σ(fluid_energy)` = constant (for temperature)
4. Test particle-fluid coupling with conservation validation

### Step 8: Add Mass Conservation from Particles (CRITICAL)
1. Track particle mass contributions to fluid density
   - When particle created: `fluid_density[cell] += particle_mass / cell_volume`
   - When particle destroyed: `fluid_density[cell] -= particle_mass / cell_volume`
   - When particle moves: update both old and new cells
2. Ensure mass conservation in particle creation/destruction
   - **NEVER** create/destroy particles without updating fluid density
   - **NEVER** move particles without transferring mass between cells
3. Update density field when particles are added/removed
   - Use proper mass transfer (not just counting particles)
   - Account for particle volume if applicable
4. **Test mass conservation**:
   - Create/destroy particles, verify total mass constant
   - Move particles, verify mass transfer correct
   - Stack particles, verify all mass accounted for

### Step 9: Add UI Controls
1. Add cell size control to options
2. Add atmospheric pressure toggle
3. Add performance monitor
4. Test UI interactions

### Step 10: Optimization
1. Profile performance
2. Optimize particle-to-grid mapping
3. Add multi-threading if needed
4. Optimize hot paths
5. Ensure 60 FPS target

### Step 11: Testing & Validation
1. Test with various scenarios (pressure waves, buoyancy, etc.)
2. Compare with old system
3. Validate physical accuracy
4. **Test mass conservation** (verify total mass constant)
5. **Test momentum conservation** (verify total momentum constant)
6. **Test energy conservation** (verify total energy constant)
7. Performance benchmarks

---

## Conservation Laws - CRITICAL REQUIREMENTS

### Mass Conservation

**Rule**: Total mass in the system must remain constant (unless explicitly added/removed by user).

**Implementation Requirements:**
1. **Particle Creation**: 
   ```cpp
   // When creating particle:
   fluid_density[cell] += particle_mass / cell_volume;
   total_system_mass += particle_mass;
   ```

2. **Particle Destruction**:
   ```cpp
   // When destroying particle:
   fluid_density[cell] -= particle_mass / cell_volume;
   total_system_mass -= particle_mass;
   ```

3. **Particle Movement**:
   ```cpp
   // When particle moves from cell A to cell B:
   fluid_density[old_cell] -= particle_mass / cell_volume;
   fluid_density[new_cell] += particle_mass / cell_volume;
   // Total mass unchanged ✓
   ```

4. **Stacked Particles**: ALL particles at a position must contribute:
   ```cpp
   // CORRECT: Count all particles
   ForEachParticleAt(x, y, [&](int particleID) {
       float mass = elements[parts[particleID].type].Weight;
       rho[cell] += mass / cell_volume;
   });
   
   // WRONG: Only counts one particle
   if (pmap[y][x]) {
       float mass = elements[parts[ID(pmap[y][x])].type].Weight;
       rho[cell] += mass / cell_volume;  // Misses stacked particles!
   }
   ```

### Momentum Conservation

**Rule**: Total momentum in the system must remain constant (unless external forces).

**Implementation Requirements:**
1. **Particle-Fluid Interaction**:
   ```cpp
   // When fluid affects particle:
   float old_particle_momentum = particle_mass * particle_velocity;
   particle_velocity += force * dt / particle_mass;
   float new_particle_momentum = particle_mass * particle_velocity;
   float momentum_change = new_particle_momentum - old_particle_momentum;
   
   // Transfer opposite momentum to fluid (Newton's 3rd law):
   fluid_momentum -= momentum_change;
   ```

2. **Particle-Particle Collisions** (when stacked):
   ```cpp
   // When particles at same position interact:
   float total_momentum_before = Σ(particle_mass[i] * particle_velocity[i]);
   // ... collision resolution ...
   float total_momentum_after = Σ(particle_mass[i] * particle_velocity[i]);
   assert(abs(total_momentum_after - total_momentum_before) < epsilon);
   ```

3. **Particle Contributions to Fluid**:
   ```cpp
   // When particle contributes to fluid:
   float particle_momentum = particle_mass * particle_velocity;
   fluid_momentum += particle_momentum;
   // Particle momentum is now "in" fluid, not lost
   ```

### Energy Conservation (Temperature)

**Rule**: Total energy in the system must remain constant (unless heat sources/sinks).

**Implementation Requirements:**
1. **Heat Transfer**:
   ```cpp
   // When heat transfers from particle A to particle B:
   float energy_transfer = heat_conductivity * (temp_A - temp_B) * dt;
   particle_A.energy -= energy_transfer;
   particle_B.energy += energy_transfer;
   // Total energy unchanged ✓
   ```

2. **Particle-Fluid Energy Exchange**:
   ```cpp
   // When particle heats fluid:
   float particle_energy = particle_mass * C_v * particle_temp;
   float fluid_energy = fluid_density * C_v * fluid_temp * cell_volume;
   // Energy exchange must conserve total
   ```

### Verification Functions

Add these for debugging:
```cpp
float Simulation::GetTotalMass() {
    float total = 0.0f;
    // Particle mass
    for (int i = 0; i < NPART; i++) {
        if (parts[i].type) {
            total += elements[parts[i].type].Weight;
        }
    }
    // Fluid mass
    float cell_volume = cellSize * cellSize;
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            total += rho[y][x] * cell_volume;
        }
    }
    return total;
}

float Simulation::GetTotalMomentum() {
    float total_x = 0.0f, total_y = 0.0f;
    // Particle momentum
    for (int i = 0; i < NPART; i++) {
        if (parts[i].type) {
            float mass = elements[parts[i].type].Weight;
            total_x += mass * parts[i].vx;
            total_y += mass * parts[i].vy;
        }
    }
    // Fluid momentum
    float cell_volume = cellSize * cellSize;
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            total_x += rho[y][x] * vx[y][x] * cell_volume;
            total_y += rho[y][x] * vy[y][x] * cell_volume;
        }
    }
    return sqrtf(total_x*total_x + total_y*total_y);
}
```

**Use these in tests to verify conservation!**

---

## Performance Targets

| Cell Size | Grid Cells | Target FPS | Max Update Time |
|-----------|------------|------------|-----------------|
| 1         | ~59k       | 30-45      | 33ms            |
| 2         | ~59k       | 45-60      | 22ms            |
| 4         | ~15k       | 60         | 16ms            |
| 8         | ~3.6k      | 60+        | <10ms           |

**Note**: These are estimates. Actual performance depends on:
- CPU speed
- Optimization level
- Other simulation load
- Complexity of scene

---

## Backward Compatibility

### Save File Format
- Add version field for pressure system
- Old saves: Use simplified system or convert
- New saves: Store cell size and atmospheric pressure settings

### Migration Path
1. Detect old save format
2. Optionally convert to new format
3. Default to CELL=4 for old saves
4. Warn user about physics changes

---

## Testing Scenarios

1. **Pressure Wave**: Create high pressure region, watch it propagate
2. **Vacuum**: Create low pressure, verify air flows in
3. **Atmospheric Pressure**: Toggle on/off, verify behavior difference
4. **Cell Size**: Change cell size, verify consistency
5. **Performance**: Monitor FPS with different cell sizes
6. **Edge Cases**: Empty space, full solid, mixed scenarios

---

## Future Enhancements (Post-MVP)

1. **Temperature-Pressure Coupling**: Better integration with heat system
2. **Different Gas Types**: Support for different gases (CO₂, H₂, etc.)
3. **Compressibility**: Variable compressibility based on material
4. **Sound Waves**: Proper sound propagation
5. **Turbulence Models**: More advanced turbulence simulation
6. **GPU Acceleration**: Offload to GPU for CELL=1 performance

---

## Risks & Mitigation

### Risk 1: Performance Degradation
- **Mitigation**: Start with CELL=4, optimize incrementally, allow user to adjust

### Risk 2: Numerical Instability
- **Mitigation**: Use stable time stepping, add damping terms, clamp values

### Risk 3: Breaking Existing Saves
- **Mitigation**: Version detection, conversion tools, fallback to old system

### Risk 4: Complexity
- **Mitigation**: Phased implementation, extensive testing, clear documentation

---

## Timeline Estimate (Integrated System)

- **Phase 1** (Infrastructure): 2-3 days
  - Make CELL configurable
  - Add density field
  - Refactor data structures
  
- **Phase 2** (Core Physics Integration): 5-7 days
  - Integrate temperature with pressure
  - Integrate gravity with velocity
  - Rewrite pressure calculation
  - Rewrite velocity update
  
- **Phase 3** (Particle-Fluid Coupling): 3-4 days
  - Particle contributions to fluid
  - Fluid effects on particles
  - Mass conservation
  
- **Phase 4** (Optimization): 3-4 days
  - Performance profiling
  - Multi-threading
  - Hot path optimization
  
- **Phase 5** (UI & Polish): 2-3 days
  - UI controls
  - Testing
  - Documentation

**Total**: ~15-21 days of development time

**Note**: This is a comprehensive redesign. Consider implementing in stages:
1. **MVP**: Basic pressure-velocity-temperature coupling (Steps 1-6)
2. **Enhanced**: Add particle-fluid interactions (Steps 7-8)
3. **Polish**: UI and optimization (Steps 9-11)

---

## References

- Navier-Stokes Equations: https://en.wikipedia.org/wiki/Navier%E2%80%93Stokes_equations
- Fluid Simulation for Games: Various GDC talks and papers
- Real-Time Fluid Dynamics: Jos Stam's "Stable Fluids" paper
- Ideal Gas Law: Standard physics reference

---

## Notes

- Start with simplified version, iterate
- Profile early and often
- User feedback is crucial
- Maintain backward compatibility where possible
- Document all changes thoroughly

