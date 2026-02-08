#pragma once

class Simulation;
class Air;

/**
 * Wrapper around the Rusanov air solver for TPT.
 * Grid = XCELLS × YCELLS; dx = CELL * (meter per pixel).
 * sync_from_sim: copy pv, vx, vy, rho, bmap_blockair, hv → solver.
 * sync_to_sim: copy solver primitives → pv, vx, vy, hv, rho.
 * step(dt_frame): advance solver (and heat) so total time ≈ dt_frame, then sync_to_sim.
 *
 * Units: TPT vx/vy are in game velocity (effectively pixels per frame). Solver uses m/s.
 * game_vel_scale converts: v_mps = v_game * game_vel_scale; v_game = v_mps / game_vel_scale.
 * Default 0.06 gives 1 game unit ≈ 0.06 m/s.
 */
class AirSolverWrapper
{
	void* state = nullptr;  // AirSolverState* (opaque C API)
	int ny = 0;
	int nx = 0;
	double dx_m = 0.0;
	// Larger value = same solver m/s becomes smaller game velocity. 0.6 keeps fire/smoke from spreading map-wide.
	double game_vel_scale = 0.6;

public:
	AirSolverWrapper() = default;
	~AirSolverWrapper();

	/** Create or recreate state for grid YCELLS × XCELLS. Call after grid constants are known. */
	void ensure_created(int ny_cells, int nx_cells, double cell_size_m);

	/** Copy sim/air arrays into solver. */
	void sync_from_sim(Simulation& sim, Air& air);

	/** Copy solver primitives to sim/air arrays. */
	void sync_to_sim(Simulation& sim, Air& air);

	/** Advance solver (and heat + gravity) for up to dt_frame seconds; then sync_to_sim.
	 *  Gravity from sim (vertical + Newtonian) applied as body force; buoyancy is implicit. */
	void step(Simulation& sim, double dt_frame, int max_steps = 1);

	/** Set boundary cells as reflective walls. Call after ensure_created if desired. */
	void set_boundary_walls();

	/** Set domain boundary from edge mode: 0=void (open), 1=solid (reflective), 2=loop (periodic). ambient_pressure_pa for open. */
	void set_boundary_mode(int edgeMode, double ambient_pressure_pa);

	/** Set uniform rest state (e.g. 1 atm, ambient temp). */
	void set_uniform(double rho_kg_m3, double ux_mps, double uy_mps, double p_pa);

	/** Copy wall heat lost (J/m per unit depth) to out[row-major]. Call after step(); use to cool particles. */
	void get_wall_heat_lost(float* out);

	bool has_state() const { return state != nullptr; }
};
