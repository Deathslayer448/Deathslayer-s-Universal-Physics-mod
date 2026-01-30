#include "AirSolverWrapper.h"
#include "Air.h"
#include "Simulation.h"
#include "SimulationConfig.h"
#include "air_solver_api.h"
#include <cmath>
#include <algorithm>

AirSolverWrapper::~AirSolverWrapper()
{
	if (state)
	{
		air_solver_destroy(static_cast<AirSolverState*>(state));
		state = nullptr;
	}
}

void AirSolverWrapper::ensure_created(int ny_cells, int nx_cells, double cell_size_m)
{
	if (state && ny == ny_cells && nx == nx_cells && dx_m == cell_size_m)
		return;
	if (state)
	{
		air_solver_destroy(static_cast<AirSolverState*>(state));
		state = nullptr;
	}
	ny = ny_cells;
	nx = nx_cells;
	dx_m = cell_size_m;
	state = air_solver_create(ny, nx, dx_m);
}

void AirSolverWrapper::sync_from_sim(Simulation& sim, Air& air)
{
	if (!state)
		return;
	// TPT arrays are [y][x]; row-major flat is [iy*nx+ix]
	const float* pv = &sim.pv[0][0];
	const float* vx = &sim.vx[0][0];
	const float* vy = &sim.vy[0][0];
	const float* rho = &air.rho[0][0];
	const unsigned char* wall = &air.bmap_blockair[0][0];
	air_solver_sync_from_tpt(static_cast<AirSolverState*>(state),
		pv, vx, vy, rho, wall, (float)game_vel_scale);
}

void AirSolverWrapper::sync_to_sim(Simulation& sim, Air& air)
{
	if (!state)
		return;
	float* pv = &sim.pv[0][0];
	float* vx = &sim.vx[0][0];
	float* vy = &sim.vy[0][0];
	float* hv = &sim.hv[0][0];
	float* rho = &air.rho[0][0];
	air_solver_sync_to_tpt(static_cast<AirSolverState*>(state),
		pv, vx, vy, hv, rho, (float)game_vel_scale);
}

void AirSolverWrapper::step(double dt_frame)
{
	if (!state)
		return;
	AirSolverState* s = static_cast<AirSolverState*>(state);
	double advance = 0.0;
	// One step per frame keeps 60 FPS even in debug builds. Use -Dbuildtype=release for full speed + more steps.
	const int max_steps = 1;
	int steps = 0;
	int reject = 0;
	while (advance < dt_frame && steps < max_steps)
	{
		double dt = air_solver_step(s);
		if (dt <= 0.0)
		{
			reject++;
			if (reject > 5)
				break;
			continue;
		}
		reject = 0;
		air_solver_apply_heat_diffusion(s, dt);
		advance += dt;
		steps++;
	}
}

void AirSolverWrapper::set_boundary_walls()
{
	if (state)
		air_solver_set_boundary_walls(static_cast<AirSolverState*>(state));
}

void AirSolverWrapper::set_uniform(double rho_kg_m3, double ux_mps, double uy_mps, double p_pa)
{
	if (state)
		air_solver_set_uniform(static_cast<AirSolverState*>(state), rho_kg_m3, ux_mps, uy_mps, p_pa);
}
