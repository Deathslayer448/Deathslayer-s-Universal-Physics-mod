#include "Air.h"
#include "Simulation.h"
#include "ElementClasses.h"
#include "common/tpt-rand.h"
#include "AirSolverWrapper.h"
#include <cmath>
#include <algorithm>

void Air::make_kernel(void) //used for velocity
{
	float s = 0.0f;
	for (auto j=-1; j<2; j++)
	{
		for (auto i=-1; i<2; i++)
		{
			kernel[(i+1)+3*(j+1)] = expf(-2.0f*(i*i+j*j));
			s += kernel[(i+1)+3*(j+1)];
		}
	}
	s = 1.0f / s;
	for (auto j=-1; j<2; j++)
	{
		for (auto i=-1; i<2; i++)
		{
			kernel[(i+1)+3*(j+1)] *= s;
		}
	}
}


float Air::vorticity(const RenderableSimulation & sm, int y, int x)
{
	auto &vx = sm.vx;
	auto &vy = sm.vy;

	if (x > 1 && x < XCELLS-2 && y > 1 && y < YCELLS-2)
	{
		// dvy/dx - dvx/dy
		return (vy[y][x+1] - vy[y][x-1] - (vx[y+1][x] - vx[y-1][x]))*0.5f;
	}
	else
		return 0.0f;
}

void Air::Clear()
{
	std::fill(&sim.vy[0][0], &sim.vy[0][0]+NCELL, 0.0f);
	std::fill(&sim.vx[0][0], &sim.vx[0][0]+NCELL, 0.0f);
		// Initialize density to standard atmospheric density at ambient temperature
		// Using ideal gas law: ρ = P/(RT), with P = 101325 Pa (1 atm), R = 287 J/(kg·K), T = ambientAirTemp
		// Default density ≈ 1.225 kg/m³ at 15°C (288.15 K)
		const float R_gas = 287.0f; // Specific gas constant for air (J/(kg·K))
		const float P_atm = 101325.0f; // Standard atmospheric pressure (Pa)
		float defaultDensity = P_atm / (R_gas * ambientAirTemp);
		std::fill(&rho[0][0], &rho[0][0]+NCELL, defaultDensity);
		// Initialize pressure to atmospheric pressure (absolute, in Pascals)
		// pv stores absolute pressure directly in Pascals
		std::fill(&sim.pv[0][0], &sim.pv[0][0]+NCELL, P_atm);
}

void Air::ClearAirH()
{
	std::fill(&sim.hv[0][0], &sim.hv[0][0]+NCELL, ambientAirTemp);
}

// Used when updating temp or velocity from far away
const float advDistanceMult = 0.7f;

void Air::update_airh(void)
{
	// When using Rusanov solver, air temp (hv) is updated by the solver in update_air(); skip legacy hv advection.
	if (rusanovSolver.has_state())
		return;
	auto &vx = sim.vx;
	auto &vy = sim.vy;
	auto &hv = sim.hv;
	for (auto i=0; i<YCELLS; i++) //sets air temp on the edges every frame
	{
		hv[i][0] = ambientAirTemp;
		hv[i][1] = ambientAirTemp;
		hv[i][XCELLS-2] = ambientAirTemp;
		hv[i][XCELLS-1] = ambientAirTemp;
	}
	for (auto i=0; i<XCELLS; i++) //sets air temp on the edges every frame
	{
		hv[0][i] = ambientAirTemp;
		hv[1][i] = ambientAirTemp;
		hv[YCELLS-2][i] = ambientAirTemp;
		hv[YCELLS-1][i] = ambientAirTemp;
	}
	for (auto y=0; y<YCELLS; y++) //update air temp and velocity
	{
		for (auto x=0; x<XCELLS; x++)
		{
			auto dh = 0.0f;
			auto dx = 0.0f;
			auto dy = 0.0f;
			for (auto j = -1; j <= 1; j++)
			{
				for (auto i = -1; i <= 1; i++)
				{
					if (y+j > 0 && y+j < YCELLS-1 && x+i > 0 && x+i < XCELLS-1 && !(bmap_blockairh[y+j][x+i]&0x8))
					{
						auto f = kernel[i+1+(j+1)*3];
						dh += hv[y+j][x+i]*f;
						dx += vx[y+j][x+i]*f;
						dy += vy[y+j][x+i]*f;
					}
					else
					{
						auto f = kernel[i+1+(j+1)*3];
						dh += hv[y][x]*f;
						dx += vx[y][x]*f;
						dy += vy[y][x]*f;
					}
				}
			}

			// Trying to take air temp from far away.
			// The code is almost identical to the "far away" velocity code from update_air
			auto tx = x - dx*advDistanceMult;
			auto ty = y - dy*advDistanceMult;
			if ((std::abs(dx*advDistanceMult)>1.0f || std::abs(dy*advDistanceMult)>1.0f) && (tx>=2 && tx<XCELLS-2 && ty>=2 && ty<YCELLS-2))
			{
				float stepX, stepY;
				int stepLimit;
				if (std::abs(dx)>std::abs(dy))
				{
					stepX = (dx<0.0f) ? 1.f : -1.f;
					stepY = -dy/fabsf(dx);
					stepLimit = (int)(fabsf(dx*advDistanceMult));
				}
				else
				{
					stepY = (dy<0.0f) ? 1.f : -1.f;
					stepX = -dx/fabsf(dy);
					stepLimit = (int)(fabsf(dy*advDistanceMult));
				}
				tx = float(x);
				ty = float(y);
				auto step = 0;
				for (; step<stepLimit; ++step)
				{
					tx += stepX;
					ty += stepY;
					if (bmap_blockairh[(int)(ty+0.5f)][(int)(tx+0.5f)]&0x8)
					{
						tx -= stepX;
						ty -= stepY;
						break;
					}
				}
				if (step==stepLimit)
				{
					// No wall found
					tx = x - dx*advDistanceMult;
					ty = y - dy*advDistanceMult;
				}
			}
			auto i = (int)tx;
			auto j = (int)ty;
			tx -= i;
			ty -= j;
			if (!(bmap_blockairh[y][x]&0x8) && i>=0 && i<XCELLS-1 && j>=0 && j<YCELLS-1)
			{
				auto odh = dh;
				dh *= 1.0f - AIR_VADV;
				dh += AIR_VADV*(1.0f-tx)*(1.0f-ty)*((bmap_blockairh[j][i]&0x8) ? odh : hv[j][i]);
				dh += AIR_VADV*tx*(1.0f-ty)*((bmap_blockairh[j][i+1]&0x8) ? odh : hv[j][i+1]);
				dh += AIR_VADV*(1.0f-tx)*ty*((bmap_blockairh[j+1][i]&0x8) ? odh : hv[j+1][i]);
				dh += AIR_VADV*tx*ty*((bmap_blockairh[j+1][i+1]&0x8) ? odh : hv[j+1][i+1]);
			}

			// Don't update if the current cell blocks ambient heat
			if (bmap_blockairh[y][x]&0x8)
				dh = hv[y][x];

			// Temp caps
			if (dh > MAX_TEMP) dh = MAX_TEMP;
			if (dh < MIN_TEMP) dh = MIN_TEMP;

			ohv[y][x] = dh;

			// TEMPORARILY DISABLED: Air convection modifies velocity, interfering with our physics
			// This is called AFTER update_air() and overwrites our velocity calculations
			// TODO: Integrate heat effects properly into the physics system
			/*
			// Air convection.
			// We use the Boussinesq approximation, i.e. we assume density to be nonconstant only
			// near the gravity term of the fluid equation, and we suppose that it depends linearly on the
			// difference between the current temperature (hv[y][x]) and some "stationary" temperature (ambientAirTemp).
			float dvx, dvy;
			dvx = vx[y][x];
		       	dvy = vy[y][x];

			if (x>=2 && x<XCELLS-2 && y>=2 && y<YCELLS-2)
			{
				float convGravX, convGravY;
				sim.GetGravityField(x*CELL, y*CELL, -1.0f, -1.0f, convGravX, convGravY);

				// Cap the gravity field
				float gravMagn = std::sqrt(convGravX*convGravX + convGravY*convGravY);
				if (gravMagn > 10.0f)
				{
					convGravX /= 0.1f*gravMagn;
					convGravY /= 0.1f*gravMagn;
				}

				auto weight = (hv[y][x] - ambientAirTemp) / 10000.0f;

				// Our approximation works best when the temperature difference is small, so we cap it from above.
				if (weight > 0.01f) weight = 0.01f;

				dvx += weight * convGravX;
				dvy += weight * convGravY;
			}

			// Velocity cap
			if (dvx > MAX_PRESSURE) dvx = MAX_PRESSURE;
			if (dvx < MIN_PRESSURE) dvx = MIN_PRESSURE;
			if (dvy > MAX_PRESSURE) dvy = MAX_PRESSURE;
			if (dvy < MIN_PRESSURE) dvy = MIN_PRESSURE;

			vx[y][x] = dvx;
			vy[y][x] = dvy;
			*/
		}
	}
	memcpy(hv, ohv, sizeof(hv));
}

void Air::update_air(void)
{
	auto &vx = sim.vx;
	auto &vy = sim.vy;
	auto &pv = sim.pv;
	(void)sim.hv;
	(void)sim.fvx;
	(void)sim.fvy;
	(void)sim.bmap;
	if (airMode != AIR_NOUPDATE) //airMode 4 is no air/pressure update
	{
		// Rusanov solver path: conservative compressible gas + heat diffusion
		// Air grid: CELL (4) particle units per air cell. Use effective dx so CFL gives usable dt:
		// real 1 part = 1 mm => dx = 0.004 m => dt ~ 4 µs => 4000+ steps/frame (1 FPS).
		// Use 1 air cell = 1 cm (CELL*0.0025) => dx=0.01 m => dt~10 µs; 8 steps => ~0.08 ms/frame.
		// Use 1 air cell = 4 cm (CELL*0.01) => dx=0.04 m => dt~40 µs; 8 steps => ~0.32 ms/frame, pressure visible.
		const double cell_size_m = CELL * 0.01;   // effective 4 cm per air cell for solver (CFL + visible pressure)
		const double frame_dt = 1.0 / 60.0;       // target sim time per frame (60 fps)
		rusanovSolver.ensure_created(YCELLS, XCELLS, cell_size_m);
		rusanovSolver.set_boundary_walls();
		rusanovSolver.sync_from_sim(sim, *this);
		rusanovSolver.step(frame_dt);
		rusanovSolver.sync_to_sim(sim, *this);
		// Keep wall cells consistent: no velocity, pressure = adjacent (solver doesn't write to walls)
		for (auto j = 1; j < YCELLS - 1; j++)
		{
			for (auto i = 1; i < XCELLS - 1; i++)
			{
				if (bmap_blockair[j][i])
				{
					vx[j][i] = 0.0f;
					vy[j][i] = 0.0f;
					if (!bmap_blockair[j][i-1])
						pv[j][i] = pv[j][i-1];
					else if (!bmap_blockair[j][i+1])
						pv[j][i] = pv[j][i+1];
					else if (!bmap_blockair[j-1][i])
						pv[j][i] = pv[j-1][i];
					else if (!bmap_blockair[j+1][i])
						pv[j][i] = pv[j+1][i];
					else
						pv[j][i] = 101325.0f;
				}
			}
		}
		return;
	}

#if 0  // USE_LEGACY_AIR_PHYSICS — old pressure/velocity update (kept for reference/revert)
	if (airMode != AIR_NOUPDATE)
	{
		// Boundary conditions: check if edges are void or solid
		// Void (bmap_blockair == false): pressure can escape (open boundary)
		// Solid (bmap_blockair == true): pressure is blocked (no-slip boundary, pressure contained)
		const float P_atm = 101325.0f;
		
		// TEMPORARILY DISABLED: Boundary conditions that lose energy
		// These were:
		// 1. Forcing pressure to atmospheric at edges (losing pressure/energy)
		// 2. Damping velocity at edges (losing energy)
		//
		// We need proper boundary conditions that conserve energy:
		// - For solid walls: no-slip (v=0) but pressure should reflect, not be forced
		// - For void edges: open boundary (pressure can escape naturally through physics)
		//
		// TODO: Implement proper boundary conditions that conserve energy
		// For now, we disable these to let our physics work without interference
		/*
		for (auto i=0; i<YCELLS; i++)
		{
			// Left edge (x=0)
			if (!bmap_blockair[i][0]) // Void - pressure can escape
			{
				pv[i][0] = pv[i][0] * 0.9f + P_atm * 0.1f; // Gradually restore to atmospheric
			}
			// else solid wall - pressure is blocked, don't modify
			
			// Right edge (x=XCELLS-1)
			if (!bmap_blockair[i][XCELLS-1]) // Void - pressure can escape
			{
				pv[i][XCELLS-1] = pv[i][XCELLS-1] * 0.9f + P_atm * 0.1f;
			}
			// else solid wall - pressure is blocked, don't modify
			
			// Damp velocity at all edges (no-slip boundary)
			vx[i][0] = vx[i][0]*0.9f;
			vx[i][1] = vx[i][1]*0.9f;
			vx[i][XCELLS-2] = vx[i][XCELLS-2]*0.9f;
			vx[i][XCELLS-1] = vx[i][XCELLS-1]*0.9f;
			vy[i][0] = vy[i][0]*0.9f;
			vy[i][1] = vy[i][1]*0.9f;
			vy[i][XCELLS-2] = vy[i][XCELLS-2]*0.9f;
			vy[i][XCELLS-1] = vy[i][XCELLS-1]*0.9f;
		}
		for (auto i=0; i<XCELLS; i++)
		{
			// Top edge (y=0)
			if (!bmap_blockair[0][i]) // Void - pressure can escape
			{
				pv[0][i] = pv[0][i] * 0.9f + P_atm * 0.1f;
			}
			// else solid wall - pressure is blocked, don't modify
			
			// Bottom edge (y=YCELLS-1)
			if (!bmap_blockair[YCELLS-1][i]) // Void - pressure can escape
			{
				pv[YCELLS-1][i] = pv[YCELLS-1][i] * 0.9f + P_atm * 0.1f;
			}
			// else solid wall - pressure is blocked, don't modify
			
			// Damp velocity at all edges (no-slip boundary)
			vx[0][i] = vx[0][i]*0.9f;
			vx[1][i] = vx[1][i]*0.9f;
			vx[YCELLS-2][i] = vx[YCELLS-2][i]*0.9f;
			vx[YCELLS-1][i] = vx[YCELLS-1][i]*0.9f;
			vy[0][i] = vy[0][i]*0.9f;
			vy[1][i] = vy[1][i]*0.9f;
			vy[YCELLS-2][i] = vy[YCELLS-2][i]*0.9f;
			vy[YCELLS-1][i] = vy[YCELLS-1][i]*0.9f;
		}
		*/

		// Initialize wall pressure and velocity FIRST, before any calculations
		// This prevents uninitialized wall pressure from creating huge gradients
		for (auto j=1; j<YCELLS-1; j++)
		{
			for (auto i=1; i<XCELLS-1; i++)
			{
				if (bmap_blockair[j][i])
				{
					// No-slip boundary: velocity at wall = 0
					vx[j][i] = 0.0f;
					vy[j][i] = 0.0f;
					
					// Initialize wall pressure to match adjacent air cell (reflection boundary condition)
					// This MUST be done before any pressure/velocity calculations to prevent huge gradients
					// Find the nearest air cell and use its pressure
					if (!bmap_blockair[j][i-1])
						pv[j][i] = pv[j][i-1];
					else if (!bmap_blockair[j][i+1])
						pv[j][i] = pv[j][i+1];
					else if (!bmap_blockair[j-1][i])
						pv[j][i] = pv[j-1][i];
					else if (!bmap_blockair[j+1][i])
						pv[j][i] = pv[j+1][i];
					else
					{
						// If all neighbors are walls, use atmospheric pressure as fallback
						const float P_atm = 101325.0f;
						pv[j][i] = P_atm;
					}
				}
			}
		}

		// SIMPLIFIED PHYSICS: Only pressure and velocity, no interference
		// Use temporary arrays to avoid race conditions (checkerboard pattern)
		// Read from vx/vy/pv, write to ovx/ovy/opv, then copy back
		
		const float R_gas = 287.0f; // Specific gas constant for air (J/(kg·K))
		const float gamma = 1.4f; // Adiabatic index for air (cp/cv)
		const float dt_p = AIR_TSTEPP;
		const float dt_v = AIR_TSTEPV;
		const float frame_to_second = 1.0f / 60.0f;
		const float cell_to_meter = CELL * 0.001f; // Cell size in meters
		const float pixel_to_meter = 0.001f;
		
		// Initialize temporary arrays with current values
		for (auto y=0; y<YCELLS; y++)
		{
			for (auto x=0; x<XCELLS; x++)
			{
				ovx[y][x] = vx[y][x];
				ovy[y][x] = vy[y][x];
				opv[y][x] = pv[y][x];
			}
		}
		
		// STEP 1: Update pressure and density from velocity divergence
		// Iterate consistently: top to bottom, left to right
		// DEBUG: Track stats
		int cells_updated = 0;
		float max_div = 0.0f;
		float max_pressure = 0.0f;
		
		for (auto y=1; y<YCELLS-1; y++)
		{
			for (auto x=1; x<XCELLS-1; x++)
			{
				if (!bmap_blockair[y][x])
				{
					cells_updated++;
					
					// SIMPLIFIED: Use central differences, handle walls by using reflection
					// Read from CURRENT arrays (vx/vy), write to TEMPORARY arrays (ovx/ovy/opv)
					
					// Clamp P and rho before using in calculations to prevent NaN/Inf
					float rho_cell = rho[y][x];
					if (rho_cell < 0.01f) rho_cell = 0.01f;
					float P_cell = pv[y][x];
					if (P_cell < 0.1f) P_cell = 0.1f;
					
					// Sound speed: c = sqrt(γ * P / ρ)
					float c_sound = std::sqrt(gamma * P_cell / rho_cell); // m/s
					
					// Check for NaN/Inf in velocity BEFORE calculating divergence
					// If velocity is already corrupted, reset it to prevent cascade
					if (!std::isfinite(vx[y][x]) || !std::isfinite(vy[y][x]))
					{
						vx[y][x] = 0.0f;
						vy[y][x] = 0.0f;
					}
					
					// Divergence: ∇·v = ∂vx/∂x + ∂vy/∂y
					// Handle boundaries: if neighbor is wall or out of bounds, use reflection (0 velocity)
					float vx_left = (x-1 < 0 || bmap_blockair[y][x-1]) ? 0.0f : vx[y][x-1];
					float vx_right = (x+1 >= XCELLS || bmap_blockair[y][x+1]) ? 0.0f : vx[y][x+1];
					float vy_up = (y-1 < 0 || bmap_blockair[y-1][x]) ? 0.0f : vy[y-1][x];
					float vy_down = (y+1 >= YCELLS || bmap_blockair[y+1][x]) ? 0.0f : vy[y+1][x];
					
					// Check neighbors for NaN/Inf
					if (!std::isfinite(vx_left)) vx_left = 0.0f;
					if (!std::isfinite(vx_right)) vx_right = 0.0f;
					if (!std::isfinite(vy_up)) vy_up = 0.0f;
					if (!std::isfinite(vy_down)) vy_down = 0.0f;
					
					float dvx_dx = (vx_right - vx_left) / (2.0f * CELL);
					float dvy_dy = (vy_down - vy_up) / (2.0f * CELL);
					float div_v_frame = dvx_dx + dvy_dy; // 1/frame
					
					if (std::abs(div_v_frame) > max_div) max_div = std::abs(div_v_frame);
					
					// PHYSICAL BOUND ON DIVERGENCE (Navier-Stokes constraint)
					// In compressible flow, the maximum divergence is limited by the sound speed and cell size.
					// For a cell of size dx, the maximum physically reasonable divergence is c_sound / dx.
					// This represents the rate at which a pressure wave can expand at the speed of sound.
					// This is NOT an arbitrary limit - it's a physical constraint from compressible flow theory.
					float max_div_v_physical = c_sound / cell_to_meter; // Maximum divergence (1/s) physically possible
					
					// Convert divergence to 1/second
					float div_v = div_v_frame / frame_to_second;
					
					// Apply physical bound: |div_v| <= c_sound / dx
					// This ensures the divergence respects the physics of compressible flow
					if (div_v > max_div_v_physical) div_v = max_div_v_physical;
					if (div_v < -max_div_v_physical) div_v = -max_div_v_physical;
					
					// Velocity magnitude in m/s
					float v_mag_mps = std::sqrt(vx[y][x]*vx[y][x] + vy[y][x]*vy[y][x]) * pixel_to_meter / frame_to_second;
					
					// CFL-limited time step: dt < dx / (c + |v|)
					// This ensures information doesn't travel more than one cell per time step
					float max_dt_cfl = cell_to_meter / (c_sound + v_mag_mps + 1.0f);
					float dt_stable = std::min(dt_p * frame_to_second, max_dt_cfl);
					
					// CFL condition ensures stability: dt < dx/(c+|v|)
					// Combined with physical bound on div_v, this ensures |div_v * dt| is bounded
					float div_v_dt = div_v * dt_stable;
					
				// Update density: ∂ρ/∂t = -ρ∇·v
				// Use exponential form: ρ(t+dt) = ρ(t) * exp(-div_v * dt)
				// This is mathematically exact and stable for any div_v_dt
				float rho_new = rho[y][x] * std::exp(-div_v_dt);
				
				// Clamp density to physical limits
				if (rho_new < 0.01f) rho_new = 0.01f;
				if (rho_new > 10.0f) rho_new = 10.0f;
				rho[y][x] = rho_new;
					
				// Update pressure: ∂P/∂t = -γP∇·v (adiabatic process)
				// Use exponential form: P(t+dt) = P(t) * exp(-γ * div_v * dt)
				// This allows pressure to spread naturally through the pressure gradient → velocity → divergence feedback
				// The adiabatic relationship P/ρ^γ = constant will be maintained approximately for adiabatic flow
				// If we enforce it cell-by-cell, we prevent pressure from spreading (checkerboard pattern)
				float P_new = P_cell * std::exp(-gamma * div_v_dt);
				
				// Clamp pressure to physical limits
				if (P_new < 0.1f) P_new = 0.1f;
				if (P_new > MAX_PRESSURE) P_new = MAX_PRESSURE;
					
					// Write to temporary array
					opv[y][x] = P_new;
					if (P_new > max_pressure) max_pressure = P_new;
					
					// TEMPORARILY DISABLED: Gravity effect on pressure (hydrostatic pressure)
					// We're focusing on getting the core fluid dynamics (density, velocity, pressure) working first
					// before adding gravity's effect on pressure
					/*
					// Add gravity effect on pressure (hydrostatic pressure: P = P0 + ρgh)
					// Gravity creates a pressure gradient based on height from reference point
					// This is an additional pressure contribution, not part of the adiabatic relationship
					if (sim.grav && x >= 2 && x < XCELLS-2 && y >= 2 && y < YCELLS-2)
					{
						// Get gravity at this cell
						float gravX, gravY;
						sim.GetGravityField(x*CELL, y*CELL, 0.0f, 1.0f, gravX, gravY);
						
						// Hydrostatic pressure: P = P0 + ρgh
						// Integrate from reference point (top of map, y=0) to current position
						// Convert from game units to real units
						const float pixel_to_meter = 0.001f; // Rough conversion: 1 pixel ≈ 1 mm
						const float cell_size_m = CELL * pixel_to_meter;
						
						// Integrate gravity over height: P = P0 + ∫ρg·dh
						// For constant density and gravity: P = P0 + ρg·Δh
						float height_diff = (y - 0) * cell_size_m; // Height from top of map
						float gravity_pressure = rho[y][x] * gravY * height_diff;
						
						// Also account for horizontal gravity component
						float width_diff = (x - XCELLS/2) * cell_size_m; // Distance from center
						gravity_pressure += rho[y][x] * gravX * width_diff;
						
						// Add gravity contribution to pressure
						// This is a separate energy source (gravitational potential energy)
						pv[y][x] += gravity_pressure;
					}
					*/
					
					// Clamp pressure
					if (opv[y][x] > MAX_PRESSURE) opv[y][x] = MAX_PRESSURE;
					if (opv[y][x] < MIN_PRESSURE) opv[y][x] = MIN_PRESSURE;
				}
			}
		}

		// DEBUG: Log pressure update stats
		if (cells_updated > 0)
		{
			printf("[AIR DEBUG] Step 1: Updated %d cells, max_div=%.3f, max_pressure=%.1f Pa\n", 
			       cells_updated, max_div, max_pressure);
		}
		
		// STEP 2: Update velocity from pressure gradient
		// Read from UPDATED pressure (opv), write to temporary velocity (ovx/ovy)
		int vel_cells_updated = 0;
		float max_accel = 0.0f;
		float max_vel = 0.0f;
		
		for (auto y=1; y<YCELLS-1; y++)
		{
			for (auto x=1; x<XCELLS-1; x++)
			{
				if (bmap_blockair[y][x])
				{
					ovx[y][x] = 0.0f;
					ovy[y][x] = 0.0f;
					continue;
				}
				
				vel_cells_updated++;
				
				// Calculate pressure gradient from UPDATED pressure (opv)
				// Use reflection boundary condition at walls and boundaries
				// Central difference: ∇P = (P[x+1] - P[x-1]) / (2*dx)
				float p_left = (x-1 < 1 || bmap_blockair[y][x-1]) ? opv[y][x] : opv[y][x-1];
				float p_right = (x+1 >= XCELLS-1 || bmap_blockair[y][x+1]) ? opv[y][x] : opv[y][x+1];
				float p_up = (y-1 < 1 || bmap_blockair[y-1][x]) ? opv[y][x] : opv[y-1][x];
				float p_down = (y+1 >= YCELLS-1 || bmap_blockair[y+1][x]) ? opv[y][x] : opv[y+1][x];
				
				// Pressure gradient in Pa/m
				// If p_right > p_left, gradient points right (from low to high pressure)
				// Force = -gradP points left (from high to low pressure) - CORRECT
				float gradP_x = (p_right - p_left) / (2.0f * cell_to_meter);
				float gradP_y = (p_down - p_up) / (2.0f * cell_to_meter);
				
				// Acceleration: a = -∇P / ρ
				float rho_cell = rho[y][x];
				if (rho_cell < 0.01f) rho_cell = 0.01f;
				
				// Pressure gradient force: a = -∇P / ρ (m/s²)
				float accel_x_mps2 = -gradP_x / rho_cell;
				float accel_y_mps2 = -gradP_y / rho_cell;
				
				// Add proper viscosity: ν∇²v (kinematic viscosity times Laplacian of velocity)
				// Dynamic viscosity of air: μ ≈ 1.8e-5 Pa·s at 20°C
				// Kinematic viscosity: ν = μ/ρ ≈ 1.5e-5 m²/s at standard conditions
				const float mu = 1.8e-5f; // Dynamic viscosity (Pa·s)
				const float nu = mu / rho_cell; // Kinematic viscosity (m²/s)
				
				// Laplacian of velocity: ∇²v = ∂²v/∂x² + ∂²v/∂y²
				// Convert velocities to m/s for calculation
				const float pixel_to_meter_vel = pixel_to_meter / frame_to_second;
				float vx_mps = vx[y][x] * pixel_to_meter_vel;
				float vy_mps = vy[y][x] * pixel_to_meter_vel;
				
				// Get neighbor velocities (in m/s), use 0 at walls
				float vx_left_mps = (x-1 < 0 || bmap_blockair[y][x-1]) ? 0.0f : vx[y][x-1] * pixel_to_meter_vel;
				float vx_right_mps = (x+1 >= XCELLS || bmap_blockair[y][x+1]) ? 0.0f : vx[y][x+1] * pixel_to_meter_vel;
				float vx_up_mps = (y-1 < 0 || bmap_blockair[y-1][x]) ? 0.0f : vx[y-1][x] * pixel_to_meter_vel;
				float vx_down_mps = (y+1 >= YCELLS || bmap_blockair[y+1][x]) ? 0.0f : vx[y+1][x] * pixel_to_meter_vel;
				
				float vy_left_mps = (x-1 < 0 || bmap_blockair[y][x-1]) ? 0.0f : vy[y][x-1] * pixel_to_meter_vel;
				float vy_right_mps = (x+1 >= XCELLS || bmap_blockair[y][x+1]) ? 0.0f : vy[y][x+1] * pixel_to_meter_vel;
				float vy_up_mps = (y-1 < 0 || bmap_blockair[y-1][x]) ? 0.0f : vy[y-1][x] * pixel_to_meter_vel;
				float vy_down_mps = (y+1 >= YCELLS || bmap_blockair[y+1][x]) ? 0.0f : vy[y+1][x] * pixel_to_meter_vel;
				
				// Laplacian using central differences
				float laplacian_vx = (vx_right_mps - 2.0f*vx_mps + vx_left_mps) / (cell_to_meter * cell_to_meter) +
				                     (vx_down_mps - 2.0f*vx_mps + vx_up_mps) / (cell_to_meter * cell_to_meter);
				float laplacian_vy = (vy_right_mps - 2.0f*vy_mps + vy_left_mps) / (cell_to_meter * cell_to_meter) +
				                     (vy_down_mps - 2.0f*vy_mps + vy_up_mps) / (cell_to_meter * cell_to_meter);
				
				// Viscous acceleration: a_visc = ν∇²v (m/s²)
				float accel_visc_x_mps2 = nu * laplacian_vx;
				float accel_visc_y_mps2 = nu * laplacian_vy;
				
				// Total acceleration: a = -∇P/ρ + ν∇²v
				float total_accel_x_mps2 = accel_x_mps2 + accel_visc_x_mps2;
				float total_accel_y_mps2 = accel_y_mps2 + accel_visc_y_mps2;
				
				// CFL condition for velocity update: dt < dx / (c_sound + |v|)
				float P_vel = opv[y][x];
				if (P_vel < 0.1f) P_vel = 0.1f;
				float c_sound_vel = std::sqrt(gamma * P_vel / rho_cell); // m/s
				float v_mag_vel_mps = std::sqrt(vx[y][x]*vx[y][x] + vy[y][x]*vy[y][x]) * pixel_to_meter / frame_to_second;
				float max_dt_vel_cfl = cell_to_meter / (c_sound_vel + v_mag_vel_mps + 1.0f);
				float dt_vel_stable = std::min(dt_v * frame_to_second, max_dt_vel_cfl);
				
				// Convert to game units: pixels/frame²
				const float unit_scale = pixel_to_meter / (frame_to_second * frame_to_second);
				float accel_x = total_accel_x_mps2 * unit_scale;
				float accel_y = total_accel_y_mps2 * unit_scale;
				
				float accel_mag = std::sqrt(accel_x*accel_x + accel_y*accel_y);
				if (accel_mag > max_accel) max_accel = accel_mag;
				
				// Update velocity: read from CURRENT (vx/vy), write to TEMPORARY (ovx/ovy)
				// Use CFL-limited time step to prevent instability
				// NO ARTIFICIAL DAMPING - let proper viscosity handle energy dissipation
				float dt_vel_game = dt_vel_stable / frame_to_second; // Convert back to game time units
				float vx_new = vx[y][x] + accel_x * dt_vel_game;
				float vy_new = vy[y][x] + accel_y * dt_vel_game;
				
				// Zero at walls
				if (bmap_blockair[y][x-1] || bmap_blockair[y][x] || bmap_blockair[y][x+1])
					vx_new = 0.0f;
				if (bmap_blockair[y-1][x] || bmap_blockair[y][x] || bmap_blockair[y+1][x])
					vy_new = 0.0f;
				
				ovx[y][x] = vx_new;
				ovy[y][x] = vy_new;
				
				float vel_mag = std::sqrt(vx_new*vx_new + vy_new*vy_new);
				if (vel_mag > max_vel) max_vel = vel_mag;
			}
		}
		
		// DEBUG: Log velocity update stats
		if (vel_cells_updated > 0)
		{
			// Sample a few cells to see what's happening
			int sample_y = YCELLS / 2;
			int sample_x = XCELLS / 2;
			if (!bmap_blockair[sample_y][sample_x])
			{
				float sample_p = opv[sample_y][sample_x];
				float sample_vx = ovx[sample_y][sample_x];
				float sample_vy = ovy[sample_y][sample_x];
				printf("[AIR DEBUG] Step 2: Updated %d cells, max_accel=%.3f, max_vel=%.3f | Sample[%d,%d]: P=%.1f Pa, v=(%.3f,%.3f)\n",
				       vel_cells_updated, max_accel, max_vel, sample_x, sample_y, sample_p, sample_vx, sample_vy);
			}
			else
			{
				printf("[AIR DEBUG] Step 2: Updated %d cells, max_accel=%.3f, max_vel=%.3f\n",
				       vel_cells_updated, max_accel, max_vel);
			}
		}
		
		// STEP 3: Copy temporary arrays back to main arrays
		// This completes the update without race conditions
		// Use memcpy for efficiency - we've updated all cells that need updating
		memcpy(vx, ovx, sizeof(vx));
		memcpy(vy, ovy, sizeof(vy));
		memcpy(pv, opv, sizeof(pv));

		// DISABLED: Old advection step - we want real physics, not simplified smoothing
		// The physics must be stable on its own
		/*
		for (auto y=0; y<YCELLS; y++) //update velocity and pressure
		{
			for (auto x=0; x<XCELLS; x++)
			{
				auto dx = 0.0f;
				auto dy = 0.0f;
				auto dp = 0.0f;
				for (auto j=-1; j<2; j++)
				{
					for (auto i=-1; i<2; i++)
					{
						if (y+j>0 && y+j<YCELLS-1 &&
						        x+i>0 && x+i<XCELLS-1 &&
						        !bmap_blockair[y+j][x+i])
						{
							auto f = kernel[i+1+(j+1)*3];
							dx += vx[y+j][x+i]*f;
							dy += vy[y+j][x+i]*f;
							dp += pv[y+j][x+i]*f;
						}
						else
						{
							auto f = kernel[i+1+(j+1)*3];
							dx += vx[y][x]*f;
							dy += vy[y][x]*f;
							dp += pv[y][x]*f;
						}
					}
				}

				auto tx = x - dx*advDistanceMult;
				auto ty = y - dy*advDistanceMult;
				if ((std::abs(dx*advDistanceMult)>1.0f || std::abs(dy*advDistanceMult)>1.0f) && (tx>=2 && tx<XCELLS-2 && ty>=2 && ty<YCELLS-2))
				{
					// Trying to take velocity from far away, check whether there is an intervening wall.
					// Step from current position to desired source location, looking for walls, with either the x or y step size being 1 cell
					float stepX, stepY;
					int stepLimit;
					if (std::abs(dx)>std::abs(dy))
					{
						stepX = (dx<0.0f) ? 1.f : -1.f;
						stepY = -dy/fabsf(dx);
						stepLimit = (int)(fabsf(dx*advDistanceMult));
					}
					else
					{
						stepY = (dy<0.0f) ? 1.f : -1.f;
						stepX = -dx/fabsf(dy);
						stepLimit = (int)(fabsf(dy*advDistanceMult));
					}
					tx = float(x);
					ty = float(y);
					auto step = 0;
					for (; step<stepLimit; ++step)
					{
						tx += stepX;
						ty += stepY;
						if (bmap_blockair[(int)(ty+0.5f)][(int)(tx+0.5f)])
						{
							tx -= stepX;
							ty -= stepY;
							break;
						}
					}
					if (step==stepLimit)
					{
						// No wall found
						tx = x - dx*advDistanceMult;
						ty = y - dy*advDistanceMult;
					}
				}
				auto i = (int)tx;
				auto j = (int)ty;
				tx -= i;
				ty -= j;
				if (!bmap_blockair[y][x] && i>=2 && i<XCELLS-3 && j>=2 && j<YCELLS-3)
				{
					dx *= 1.0f - AIR_VADV;
					dy *= 1.0f - AIR_VADV;

					dx += AIR_VADV*(1.0f-tx)*(1.0f-ty)*vx[j][i];
					dy += AIR_VADV*(1.0f-tx)*(1.0f-ty)*vy[j][i];

					dx += AIR_VADV*tx*(1.0f-ty)*vx[j][i+1];
					dy += AIR_VADV*tx*(1.0f-ty)*vy[j][i+1];

					dx += AIR_VADV*(1.0f-tx)*ty*vx[j+1][i];
					dy += AIR_VADV*(1.0f-tx)*ty*vy[j+1][i];

					dx += AIR_VADV*tx*ty*vx[j+1][i+1];
					dy += AIR_VADV*tx*ty*vy[j+1][i+1];
				}

				//Vorticity confinement
				if (vorticityCoeff > 0.0f && x > 1 && x < XCELLS-2 && y > 1 && y < YCELLS-2)
				{
					auto dwx = (std::abs(vorticity(sim, y, x+1)) - std::abs(vorticity(sim, y, x-1)))*0.5f;
					auto dwy = (std::abs(vorticity(sim, y+1, x)) - std::abs(vorticity(sim, y-1, x)))*0.5f;
					auto norm = std::sqrt(dwx*dwx + dwy*dwy);
					auto w = vorticity(sim, y, x);

					dx += vorticityCoeff/5.0f * dwy / (norm + 0.001f) * w;
					dy += vorticityCoeff/5.0f * (-dwx) / (norm + 0.001f) * w;
				}

				if (bmap[y][x] == WL_FAN)
				{
					dx += fvx[y][x];
					dy += fvy[y][x];
				}
				// pressure/velocity caps (pressure in Pascals, velocity in pixels/frame)
				if (dp > MAX_PRESSURE) dp = MAX_PRESSURE;
				if (dp < MIN_PRESSURE) dp = MIN_PRESSURE;
				// Velocity limits (not pressure limits!)
				const float MAX_VEL = 50.0f; // pixels/frame
				if (dx > MAX_VEL) dx = MAX_VEL;
				if (dx < -MAX_VEL) dx = -MAX_VEL;
				if (dy > MAX_VEL) dy = MAX_VEL;
				if (dy < -MAX_VEL) dy = -MAX_VEL;


				switch (airMode)
				{
				default:
				case AIR_ON:  //Default
					break;
				case AIR_PRESSUREOFF:  //0 Pressure
					dp = 0.0f;
					break;
				case AIR_VELOCITYOFF:  //0 Velocity
					dx = 0.0f;
					dy = 0.0f;
					break;
				case AIR_OFF: //0 Air
					dx = 0.0f;
					dy = 0.0f;
					dp = 0.0f;
					break;
				case AIR_NOUPDATE: //No Update
					break;
				}

				ovx[y][x] = dx;
				ovy[y][x] = dy;
				opv[y][x] = dp;
			}
		}
		memcpy(vx, ovx, sizeof(vx));
		memcpy(vy, ovy, sizeof(vy));
		memcpy(pv, opv, sizeof(pv));
		*/
	}
#endif  // USE_LEGACY_AIR_PHYSICS
}

void Air::Invert()
{
	auto &vx = sim.vx;
	auto &vy = sim.vy;
	auto &pv = sim.pv;
	for (auto nx = 0; nx<XCELLS; nx++)
	{
		for (auto ny = 0; ny<YCELLS; ny++)
		{
			pv[ny][nx] = -pv[ny][nx];
			vx[ny][nx] = -vx[ny][nx];
			vy[ny][nx] = -vy[ny][nx];
		}
	}
}

// called when loading saves / stamps to ensure nothing "leaks" the first frame
void Air::ApproximateBlockAirMaps()
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;
	for (int i = 0; i < sim.parts.active; i++)
	{
		int type = sim.parts[i].type;
		if (!type)
			continue;
		// Real TTAN would only block if there was enough TTAN
		// but it would be more expensive and complicated to actually check that
		// so just block for a frame, if it wasn't supposed to block it will continue allowing air next frame
		if (type == PT_TTAN)
		{
			int x = ((int)(sim.parts[i].x+0.5f))/CELL, y = ((int)(sim.parts[i].y+0.5f))/CELL;
			if (InBounds(x, y))
			{
				bmap_blockair[y][x] = 1;
				bmap_blockairh[y][x] = 0x8;
			}
		}
		// mostly accurate insulator blocking, besides checking GEL
		else if (sd.IsHeatInsulator(sim.parts[i]) || elements[type].HeatConduct <= (sim.rng()%250))
		{
			int x = ((int)(sim.parts[i].x+0.5f))/CELL, y = ((int)(sim.parts[i].y+0.5f))/CELL;
			if (InBounds(x, y) && !(bmap_blockairh[y][x]&0x8))
				bmap_blockairh[y][x]++;
		}
	}
}

Air::Air(Simulation & simulation):
	sim(simulation),
	airMode(AIR_ON),
	ambientAirTemp(R_TEMP + 273.15f),
	vorticityCoeff(0.0f),
	useAtmosphericPressure(true) // Default: show relative pressure in UI
{
	//Simulation should do this.
	make_kernel();
	std::fill(&bmap_blockair [0][0], &bmap_blockair [0][0] + NCELL, 0);
	std::fill(&bmap_blockairh[0][0], &bmap_blockairh[0][0] + NCELL, 0);
	// Initialize density to standard atmospheric density at ambient temperature
	// Using ideal gas law: ρ = P/(RT), with P = 101325 Pa (1 atm), R = 287 J/(kg·K), T = ambientAirTemp
	// Default density ≈ 1.225 kg/m³ at 15°C (288.15 K)
	const float R_gas = 287.0f; // Specific gas constant for air (J/(kg·K))
	const float P_atm = 101325.0f; // Standard atmospheric pressure (Pa)
	float defaultDensity = P_atm / (R_gas * ambientAirTemp);
	std::fill(&rho[0][0], &rho[0][0] + NCELL, defaultDensity);
	std::fill(&sim.vx[0][0], &sim.vx[0][0] + NCELL, 0.0f);
	std::fill(&ovx   [0][0], &ovx   [0][0] + NCELL, 0.0f);
	std::fill(&sim.vy[0][0], &sim.vy[0][0] + NCELL, 0.0f);
	std::fill(&ovy   [0][0], &ovy   [0][0] + NCELL, 0.0f);
	std::fill(&sim.hv[0][0], &sim.hv[0][0] + NCELL, 0.0f);
	std::fill(&ohv   [0][0], &ohv   [0][0] + NCELL, 0.0f);
	std::fill(&sim.pv[0][0], &sim.pv[0][0] + NCELL, 0.0f);
	std::fill(&opv   [0][0], &opv   [0][0] + NCELL, 0.0f);
}

