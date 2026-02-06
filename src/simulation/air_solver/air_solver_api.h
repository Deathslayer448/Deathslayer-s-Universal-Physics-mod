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

/** Copy TPT arrays into solver. pv,vx,vy,rho,wall row-major; wall[i]!=0 means solid. hv = air temp (K); if non-null, used to set internal energy so air temp affects pressure (Phase 2.4). */
void air_solver_sync_from_tpt(AirSolverState* state,
    const float* pv, const float* vx, const float* vy, const float* rho, const unsigned char* wall,
    const float* hv, float game_vel_scale);

/** Copy solver primitives to TPT arrays (pv, vx, vy, hv, rho row-major). */
void air_solver_sync_to_tpt(AirSolverState* state,
    float* pv, float* vx, float* vy, float* hv, float* rho,
    float game_vel_scale);

/** One convection+viscosity step; returns dt used (or 0 on reject). */
double air_solver_step(AirSolverState* state);

/** Apply heat diffusion for dt (call after step with same dt). */
void air_solver_apply_heat_diffusion(AirSolverState* state, double dt);

/** Set boundary cells as walls (reflective box). Call after create if desired. */
void air_solver_set_boundary_walls(AirSolverState* state);

/** Set domain boundary mode from edge mode: 0=void (open, leak pressure), 1=solid (reflective), 2=loop (periodic). ambient_pressure_pa used for open. */
void air_solver_set_boundary_mode(AirSolverState* state, int edgeMode, double ambient_pressure_pa);

/** Set uniform rest state: rho (kg/m³), ux, uy (m/s), p (Pa). */
void air_solver_set_uniform(AirSolverState* state, double rho, double ux, double uy, double p);

#ifdef __cplusplus
}
#endif

#endif
