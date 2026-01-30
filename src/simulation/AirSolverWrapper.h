#pragma once

class Simulation;
class Air;

/**
 * Wrapper around the Rusanov air solver for TPT.
 * Grid = XCELLS × YCELLS; dx = CELL * (meter per pixel).
 * sync_from_sim: copy pv, vx, vy, rho, bmap_blockair → solver.
 * sync_to_sim: copy solver primitives → pv, vx, vy, hv, rho.
 * step(dt_frame): advance solver (and heat) so total time ≈ dt_frame, then sync_to_sim.
 */
class AirSolverWrapper
{
	void* state = nullptr;  // AirSolverState* (opaque C API)
	int ny = 0;
	int nx = 0;
	double dx_m = 0.0;
	double game_vel_scale = 0.06;  // TPT velocity (pixels/frame) to m/s: 0.001 * 60

public:
	AirSolverWrapper() = default;
	~AirSolverWrapper();

	/** Create or recreate state for grid YCELLS × XCELLS. Call after grid constants are known. */
	void ensure_created(int ny_cells, int nx_cells, double cell_size_m);

	/** Copy sim/air arrays into solver. */
	void sync_from_sim(Simulation& sim, Air& air);

	/** Copy solver primitives to sim/air arrays. */
	void sync_to_sim(Simulation& sim, Air& air);

	/** Advance solver (and heat diffusion) for up to dt_frame seconds; then sync_to_sim. */
	void step(double dt_frame);

	/** Set boundary cells as reflective walls. Call after ensure_created if desired. */
	void set_boundary_walls();

	/** Set uniform rest state (e.g. 1 atm, ambient temp). */
	void set_uniform(double rho_kg_m3, double ux_mps, double uy_mps, double p_pa);

	bool has_state() const { return state != nullptr; }
};
