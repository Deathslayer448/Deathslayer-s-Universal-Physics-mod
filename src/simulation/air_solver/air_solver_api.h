/**
 * Opaque API for the Rusanov air solver — use from TPT without including solver internals.
 * All arrays are row-major: a[iy*nx+ix].
 * Units: pv, hv (Pa, K); rho (kg/m³); solver velocity is m/s; game_vel_scale converts game velocity to m/s (e.g. 0.06).
 */
#ifndef AIR_SOLVER_API_H
#define AIR_SOLVER_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AirSolverState AirSolverState;

/** Create state for grid ny×nx, cell size dx (meters). */
AirSolverState* air_solver_create(int ny, int nx, double dx);

void air_solver_destroy(AirSolverState* state);

/** Copy TPT arrays into solver. pv,vx,vy,rho,wall row-major; wall[i]!=0 means solid. hv = air temp (K). wall_blocks_heat: row-major; non-zero = actual wall (adiabatic); only wall cells that are not actual walls (e.g. TTAN) get convective heat transfer. Pass nullptr to treat all walls as adiabatic. */
void air_solver_sync_from_tpt(AirSolverState* state,
    const float* pv, const float* vx, const float* vy, const float* rho, const unsigned char* wall,
    const float* hv, float game_vel_scale, const unsigned char* wall_blocks_heat);

/** Copy solver primitives to TPT arrays (pv, vx, vy, hv, rho row-major). */
void air_solver_sync_to_tpt(AirSolverState* state,
    float* pv, float* vx, float* vy, float* hv, float* rho,
    float game_vel_scale);

/** One convection+viscosity step; returns dt used (or 0 on reject). */
double air_solver_step(AirSolverState* state);

/** Apply heat diffusion for dt (call after step with same dt). */
/** heat_scale: solver setting (1 = physics-based; >1 = faster). Applied to air-air diffusion and wall-fluid. */
void air_solver_apply_heat_diffusion(AirSolverState* state, double dt, double heat_scale = 1.0);

/** Apply gravity source terms: momentum += dt*rho*g, energy += dt*rho*g·v. gx, gy in m/s², row-major [iy*nx+ix]. Call after step and heat with same dt. */
void air_solver_apply_gravity(AirSolverState* state, double dt, const float* gx_mps2, const float* gy_mps2);

/** Set boundary cells as walls (reflective box). Call after create if desired. */
void air_solver_set_boundary_walls(AirSolverState* state);

/** Set domain boundary mode from edge mode: 0=void (open, leak pressure), 1=solid (reflective), 2=loop (periodic). ambient_pressure_pa used for open. */
void air_solver_set_boundary_mode(AirSolverState* state, int edgeMode, double ambient_pressure_pa);

/** Set uniform rest state: rho (kg/m³), ux, uy (m/s), p (Pa). */
void air_solver_set_uniform(AirSolverState* state, double rho, double ux, double uy, double p);

/** Copy wall heat lost (J/m per unit depth) to out[row-major]. Call after apply_heat_diffusion; use to cool particles. */
void air_solver_get_wall_heat_lost(AirSolverState* state, float* out);

#ifdef __cplusplus
}
#endif

#endif
