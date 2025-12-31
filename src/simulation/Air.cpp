#include "Air.h"
#include "Simulation.h"
#include "ElementClasses.h"
#include "common/tpt-rand.h"
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
	const float P_scale = MAX_PRESSURE / P_atm;
	float defaultDensity = P_atm / (R_gas * ambientAirTemp);
	std::fill(&rho[0][0], &rho[0][0]+NCELL, defaultDensity);
	// Initialize pressure to match initial density state (atmospheric pressure)
	// This ensures consistency: P = ρRT at initialization
	if (useAtmosphericPressure)
	{
		// Pressure difference is 0 at atmospheric conditions
		std::fill(&sim.pv[0][0], &sim.pv[0][0]+NCELL, 0.0f);
	}
	else
	{
		// Absolute pressure = atmospheric pressure
		std::fill(&sim.pv[0][0], &sim.pv[0][0]+NCELL, P_atm * P_scale);
	}
}

void Air::ClearAirH()
{
	std::fill(&sim.hv[0][0], &sim.hv[0][0]+NCELL, ambientAirTemp);
}

// Used when updating temp or velocity from far away
const float advDistanceMult = 0.7f;

void Air::update_airh(void)
{
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
		}
	}
	memcpy(hv, ohv, sizeof(hv));
}

void Air::update_air(void)
{
	auto &vx = sim.vx;
	auto &vy = sim.vy;
	auto &pv = sim.pv;
	auto &hv = sim.hv;
	auto &fvx = sim.fvx;
	auto &fvy = sim.fvy;
	auto &bmap = sim.bmap;
	if (airMode != AIR_NOUPDATE) //airMode 4 is no air/pressure update
	{
		for (auto i=0; i<YCELLS; i++) //reduces pressure/velocity on the edges every frame
		{
			pv[i][0] = pv[i][0]*0.8f;
			pv[i][1] = pv[i][1]*0.8f;
			pv[i][XCELLS-2] = pv[i][XCELLS-2]*0.8f;
			pv[i][XCELLS-1] = pv[i][XCELLS-1]*0.8f;
			vx[i][0] = vx[i][0]*0.9f;
			vx[i][1] = vx[i][1]*0.9f;
			vx[i][XCELLS-2] = vx[i][XCELLS-2]*0.9f;
			vx[i][XCELLS-1] = vx[i][XCELLS-1]*0.9f;
			vy[i][0] = vy[i][0]*0.9f;
			vy[i][1] = vy[i][1]*0.9f;
			vy[i][XCELLS-2] = vy[i][XCELLS-2]*0.9f;
			vy[i][XCELLS-1] = vy[i][XCELLS-1]*0.9f;
		}
		for (auto i=0; i<XCELLS; i++) //reduces pressure/velocity on the edges every frame
		{
			pv[0][i] = pv[0][i]*0.8f;
			pv[1][i] = pv[1][i]*0.8f;
			pv[YCELLS-2][i] = pv[YCELLS-2][i]*0.8f;
			pv[YCELLS-1][i] = pv[YCELLS-1][i]*0.8f;
			vx[0][i] = vx[0][i]*0.9f;
			vx[1][i] = vx[1][i]*0.9f;
			vx[YCELLS-2][i] = vx[YCELLS-2][i]*0.9f;
			vx[YCELLS-1][i] = vx[YCELLS-1][i]*0.9f;
			vy[0][i] = vy[0][i]*0.9f;
			vy[1][i] = vy[1][i]*0.9f;
			vy[YCELLS-2][i] = vy[YCELLS-2][i]*0.9f;
			vy[YCELLS-1][i] = vy[YCELLS-1][i]*0.9f;
		}

		for (auto j=1; j<YCELLS-1; j++) //clear some velocities near walls
		{
			for (auto i=1; i<XCELLS-1; i++)
			{
				if (bmap_blockair[j][i])
				{
					vx[j][i] = 0.0f;
					vx[j][i-1] = 0.0f;
					vx[j][i+1] = 0.0f;
					vy[j][i] = 0.0f;
					vy[j-1][i] = 0.0f;
					vy[j+1][i] = 0.0f;
				}
			}
		}

		// Update density using continuity equation: ∂ρ/∂t + ∇·(ρv) = 0
		// For compressible flow: ∂ρ/∂t = -∇·(ρv) ≈ -ρ∇·v (advection handled by existing code)
		const float R_gas = 287.0f; // Specific gas constant for air (J/(kg·K))
		const float gamma = 1.4f; // Adiabatic index for air (cp/cv)
		const float dt = AIR_TSTEPP;
		
		for (auto y=1; y<YCELLS-1; y++)
		{
			for (auto x=1; x<XCELLS-1; x++)
			{
				if (!bmap_blockair[y][x])
				{
					// Calculate velocity divergence: ∇·v = ∂vx/∂x + ∂vy/∂y
					float div_v = (vx[y][x+1] - vx[y][x-1]) + (vy[y+1][x] - vy[y-1][x]);
					
					// Update density: ∂ρ/∂t = -ρ∇·v
					// When fluid compresses (div_v < 0), density increases
					// When fluid expands (div_v > 0), density decreases
					rho[y][x] -= rho[y][x] * div_v * dt;
					
					// Clamp density to prevent negative or extreme values
					if (rho[y][x] < 0.01f) rho[y][x] = 0.01f;
					if (rho[y][x] > 10.0f) rho[y][x] = 10.0f;
				}
			}
		}
		
		// Update pressure using pressure evolution equation for compressible flow
		// From ideal gas law and continuity: ∂P/∂t = -v·∇P - γP∇·v
		// We use a hybrid approach: evolve pressure naturally, but keep it close to ideal gas law
		const float P_atm = 101325.0f; // Standard atmospheric pressure (Pa)
		const float P_scale = MAX_PRESSURE / P_atm; // Scale factor: game units per Pa
		
		for (auto y=1; y<YCELLS-1; y++)
		{
			for (auto x=1; x<XCELLS-1; x++)
			{
				if (!bmap_blockair[y][x])
				{
					// Get temperature from ambient heat field (in Kelvin)
					float T = hv[y][x];
					if (T < 0.0f) T = ambientAirTemp;
					
					// Ideal gas law: P = ρRT (what pressure should be)
					float P_pa_ideal = rho[y][x] * R_gas * T;
					
					// Get current pressure in Pa (convert from game units)
					float P_pa_current;
					if (useAtmosphericPressure)
					{
						P_pa_current = pv[y][x] / P_scale + P_atm;
					}
					else
					{
						P_pa_current = pv[y][x] / P_scale;
					}
					
					// Calculate velocity divergence
					float div_v = (vx[y][x+1] - vx[y][x-1]) + (vy[y+1][x] - vy[y-1][x]);
					
					// Pressure evolution: ∂P/∂t = -γP∇·v (compressible flow)
					// This naturally evolves pressure based on compression/expansion
					float P_pa_evolved = P_pa_current - gamma * P_pa_current * div_v * dt;
					
					// Blend evolved pressure with ideal gas law
					// Use a small correction to keep pressure close to ideal gas law
					// This prevents drift while allowing natural evolution
					const float ideal_correction = 0.02f; // Very small correction for stability
					float P_pa_new = P_pa_evolved * (1.0f - ideal_correction) + P_pa_ideal * ideal_correction;
					
					// Ensure pressure doesn't go negative
					if (P_pa_new < 100.0f) P_pa_new = 100.0f; // Minimum 100 Pa
					
					// Convert back to game units
					float P_game;
					if (useAtmosphericPressure)
					{
						P_game = (P_pa_new - P_atm) * P_scale;
					}
					else
					{
						P_game = P_pa_new * P_scale;
					}
					
					// Apply damping and update pressure gradually
					// Use very gradual update to prevent instability from large pressure differences
					pv[y][x] *= AIR_PLOSS;
					float pressure_diff = P_game - pv[y][x];
					// Limit the maximum pressure change per frame to prevent explosions
					const float max_change = 2.0f; // Maximum pressure change per frame
					if (pressure_diff > max_change) pressure_diff = max_change;
					if (pressure_diff < -max_change) pressure_diff = -max_change;
					pv[y][x] += pressure_diff * AIR_TSTEPP * 0.3f; // Even slower update for stability
					
					// Clamp to game pressure limits
					if (pv[y][x] > MAX_PRESSURE) pv[y][x] = MAX_PRESSURE;
					if (pv[y][x] < MIN_PRESSURE) pv[y][x] = MIN_PRESSURE;
				}
			}
		}

		for (auto y=1; y<YCELLS-1; y++) //velocity adjustments from pressure
		{
			for (auto x=1; x<XCELLS-1; x++)
			{
				auto dx = 0.0f;
				auto dy = 0.0f;
				dx += pv[y][x-1] - pv[y][x+1];
				dy += pv[y-1][x] - pv[y+1][x];
				vx[y][x] *= AIR_VLOSS;
				vy[y][x] *= AIR_VLOSS;
				vx[y][x] += dx*AIR_TSTEPV * 0.5f;
				vy[y][x] += dy*AIR_TSTEPV * 0.5f;
				if (bmap_blockair[y][x-1] || bmap_blockair[y][x] || bmap_blockair[y][x+1])
					vx[y][x] = 0;
				if (bmap_blockair[y-1][x] || bmap_blockair[y][x] || bmap_blockair[y+1][x])
					vy[y][x] = 0;
			}
		}

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
				// pressure/velocity caps
				if (dp > MAX_PRESSURE) dp = MAX_PRESSURE;
				if (dp < MIN_PRESSURE) dp = MIN_PRESSURE;
				if (dx > MAX_PRESSURE) dx = MAX_PRESSURE;
				if (dx < MIN_PRESSURE) dx = MIN_PRESSURE;
				if (dy > MAX_PRESSURE) dy = MAX_PRESSURE;
				if (dy < MIN_PRESSURE) dy = MIN_PRESSURE;


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
	}
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
	useAtmosphericPressure(true) // Default: enabled
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
