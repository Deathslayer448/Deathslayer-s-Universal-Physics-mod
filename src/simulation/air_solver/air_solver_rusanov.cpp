/**
 * Gas (air) solver — 2D compressible Navier–Stokes, convection + viscosity.
 * Heat diffusion is in heat.cpp (optional).
 *
 * - Convection: first-order Rusanov (no overshoots).
 * - Viscosity: μ (dynamic), stress τ = μ(∇v + ∇vᵀ) − (2/3)μ(∇·v)I.
 * - Conservative: mass, momentum, energy (periodic BCs).
 *
 * Compile: g++ -O2 -std=c++17 -o air_rusanov air_solver_rusanov.cpp heat.cpp
 * Run:     ./air_rusanov [height] [width]   (default 40 60)
 */

#include "heat.hpp"
#include <vector>
#include <array>
#include <cmath>
#include <cstdio>
#include <algorithm>

// Debug: print to stderr when solver gets stuck (always on; flush so it appears in terminal).
#define AIR_DBG(...) do { std::fprintf(stderr, "[AIR] " __VA_ARGS__); std::fflush(stderr); } while (0)

const double gamma_gas = 1.4;
const double R_gas = 287.0;           // J/(kg·K) dry air
const double c_v = R_gas / (gamma_gas - 1.0);  // heat capacity at constant volume
const double c_p = gamma_gas * R_gas / (gamma_gas - 1.0);  // at constant pressure
const double rho_min = 1e-6;
// Minimum allowed internal energy per unit mass used only for validity checks / flux caps.
// Keep this very small so VAC can still produce (near) 0 kPa without triggering floors.
const double e_min = 1e-6;
// Clamp extreme pressures passed from TPT into the solver so CFL timestep does not collapse for crazy-high tool values.
const double p_max_solver = 1e6;      // ±10 atm inside the solver (visual pv can exceed this).
// Vacuum interface: below this density use one-sided flux (dense side only) so we don't dump huge momentum into vacuum.
const double rho_vacuum = 0.01;
const double CFL = 0.35;
const double mu = 1.8e-5;             // dynamic viscosity Pa·s (air ~300 K)

// U = [rho, rho*u, rho*v, E]; E = rho*e + 0.5*rho*(u^2+v^2). Store as 4 matrices U0[l][iy][ix].
using Mat2 = std::vector<std::vector<double>>;
using WallMask = std::vector<std::vector<bool>>;
struct State {
    int ny, nx;
    double dx;
    std::array<Mat2, 4> U0, U1;
    WallMask wall;  // true = solid (reflective wall), false = fluid. Default all false (periodic).

    State(int ny_, int nx_, double dx_ = 1.0)
        : ny(ny_), nx(nx_), dx(dx_) {
        for (int l = 0; l < 4; l++) {
            U0[l].resize(ny, std::vector<double>(nx, 0.0));
            U1[l].resize(ny, std::vector<double>(nx, 0.0));
        }
        wall.resize(ny, std::vector<bool>(nx, false));
    }

    void set_wall(int iy, int ix, bool w) { if (iy >= 0 && iy < ny && ix >= 0 && ix < nx) wall[iy][ix] = w; }
    bool is_wall(int iy, int ix) const { return (iy >= 0 && iy < ny && ix >= 0 && ix < nx) ? wall[iy][ix] : true; }
    // Set domain boundary cells as walls (reflective box). Call after construction for bounded domain.
    void set_boundary_walls() {
        for (int iy = 0; iy < ny; iy++) { set_wall(iy, 0, true); set_wall(iy, nx - 1, true); }
        for (int ix = 0; ix < nx; ix++) { set_wall(0, ix, true); set_wall(ny - 1, ix, true); }
    }

    double& u(int l, int iy, int ix) { return U0[l][iy][ix]; }
    double u(int l, int iy, int ix) const { return U0[l][iy][ix]; }

    void primitives(int iy, int ix, double& rho, double& ux, double& uy, double& e, double& p) const {
        primitives_from(U0, iy, ix, rho, ux, uy, e, p);
    }
    // T = e/c_v (Kelvin); clamp only for display/output so we never show invalid T.
    double temperature_from_e(double e) const { return std::max(e, e_min) / c_v; }
    double temperature(int iy, int ix) const {
        double r, ux, uy, e, p;
        primitives(iy, ix, r, ux, uy, e, p);
        return temperature_from_e(e);
    }
    // Use raw e = E/r - ke (no floor). Here ke is KE per unit mass: 0.5*(u^2+v^2).
    void primitives_from(const std::array<Mat2, 4>& U, int iy, int ix, double& rho, double& ux, double& uy, double& e, double& p) const {
        double r = std::max(U[0][iy][ix], rho_min);
        rho = r;
        ux = U[1][iy][ix] / r;
        uy = U[2][iy][ix] / r;
        double E = U[3][iy][ix];
        double ke = 0.5 * (U[1][iy][ix]*U[1][iy][ix] + U[2][iy][ix]*U[2][iy][ix]) / (r * r);
        e = E / r - ke;
        p = (gamma_gas - 1.0) * r * e;
    }

    // Euler x-flux F(U) = [rho*u, rho*u^2+p, rho*u*v, (E+p)*u]
    void flux_x(double rho, double ux, double uy, double p, double E, double F[4]) const {
        F[0] = rho * ux;
        F[1] = rho * ux * ux + p;
        F[2] = rho * ux * uy;
        F[3] = (E + p) * ux;
    }

    // Euler y-flux G(U) = [rho*v, rho*u*v, rho*v^2+p, (E+p)*v]
    void flux_y(double rho, double ux, double uy, double p, double E, double G[4]) const {
        G[0] = rho * uy;
        G[1] = rho * ux * uy;
        G[2] = rho * uy * uy + p;
        G[3] = (E + p) * uy;
    }

    double sound_speed(double rho, double p) const {
        return std::sqrt(gamma_gas * std::max(p, 1e-10) / std::max(rho, rho_min));
    }

    // Reflective (slip) wall: ghost state with normal velocity flipped. U = [rho, rhou, rhov, E].
    void get_U_ghost_x(const std::array<Mat2, 4>& U, int iy, int ix, double Ughost[4]) const {
        Ughost[0] = U[0][iy][ix]; Ughost[1] = -U[1][iy][ix]; Ughost[2] = U[2][iy][ix]; Ughost[3] = U[3][iy][ix];
    }
    void get_U_ghost_y(const std::array<Mat2, 4>& U, int iy, int ix, double Ughost[4]) const {
        Ughost[0] = U[0][iy][ix]; Ughost[1] = U[1][iy][ix]; Ughost[2] = -U[2][iy][ix]; Ughost[3] = U[3][iy][ix];
    }
    // Raw e (no floor) so Rusanov flux uses real pressure and does not over-drain.
    void primitives_from_U4(double U4[4], double& rho, double& ux, double& uy, double& e, double& p) const {
        double r = std::max(U4[0], rho_min);
        rho = r; ux = U4[1] / r; uy = U4[2] / r;
        double E = U4[3];
        double ke = 0.5 * (U4[1]*U4[1] + U4[2]*U4[2]) / (r * r);
        e = E / r - ke;
        p = (gamma_gas - 1.0) * r * e;
    }

    // Rusanov flux in x; uses reflective ghost when neighbor is wall.
    // At vacuum interface (one side rho < rho_vacuum) use one-sided flux from dense side so we don't dump huge momentum into vacuum.
    void rusanov_x(int iy, int ixL, int ixR, double F[4]) const {
        double UL[4], UR[4];
        if (is_wall(iy, ixL)) get_U_ghost_x(U0, iy, ixR, UL); else for (int l = 0; l < 4; l++) UL[l] = U0[l][iy][ixL];
        if (is_wall(iy, ixR)) get_U_ghost_x(U0, iy, ixL, UR); else for (int l = 0; l < 4; l++) UR[l] = U0[l][iy][ixR];
        double rL, uxL, uyL, eL, pL, rR, uxR, uyR, eR, pR;
        primitives_from_U4(UL, rL, uxL, uyL, eL, pL);
        primitives_from_U4(UR, rR, uxR, uyR, eR, pR);
        double EL = UL[3], ER = UR[3];
        double FL[4], FR[4];
        flux_x(rL, uxL, uyL, pL, EL, FL);
        flux_x(rR, uxR, uyR, pR, ER, FR);
        if (rL < rho_vacuum || rR < rho_vacuum) {
            // Euler flux has F[0]=rho*u: when dense cell is at rest (u=0) we get F[0]=0 but F[1]=p (pressure).
            // That dumps momentum into vacuum without mass → velocity explodes. Use outflow flux: mass and momentum
            // together with velocity = sound speed (rarefaction into vacuum).
            bool L_is_vacuum = (rL < rho_vacuum && rR >= rho_vacuum);
            bool R_is_vacuum = (rR < rho_vacuum && rL >= rho_vacuum);
            double c_d;
            if (L_is_vacuum) {
                c_d = sound_speed(rR, pR);
                // Flow R→L (into vacuum): F[0]<0. Mass flux = rR*cR, velocity = -cR.
                F[0] = -rR * c_d;
                F[1] = rR * c_d * c_d;           // F[0]*u_flow with u_flow = -cR gives momentum flux
                F[2] = -rR * c_d * uyR;
                F[3] = -(ER + pR) * c_d;
            } else if (R_is_vacuum) {
                c_d = sound_speed(rL, pL);
                // Flow L→R (into vacuum): F[0]>0. Mass flux = rL*cL, velocity = +cL.
                F[0] = rL * c_d;
                F[1] = rL * c_d * c_d;
                F[2] = rL * c_d * uyL;
                F[3] = (EL + pL) * c_d;
            } else {
                // Both vacuum: use denser side outflow, direction into the other.
                if (rL >= rR) {
                    c_d = sound_speed(rL, pL);
                    F[0] = rL * c_d; F[1] = rL * c_d * c_d; F[2] = rL * c_d * uyL; F[3] = (EL + pL) * c_d;
                } else {
                    c_d = sound_speed(rR, pR);
                    F[0] = -rR * c_d; F[1] = rR * c_d * c_d; F[2] = -rR * c_d * uyR; F[3] = -(ER + pR) * c_d;
                }
            }
            return;
        }
        double sL = std::abs(uxL) + sound_speed(rL, pL);
        double sR = std::abs(uxR) + sound_speed(rR, pR);
        double s_max = std::max(sL, sR);
        for (int l = 0; l < 4; l++)
            F[l] = 0.5 * (FL[l] + FR[l]) - 0.5 * s_max * (UR[l] - UL[l]);
    }

    void rusanov_y(int iyL, int iyR, int ix, double G[4]) const {
        double UL[4], UR[4];
        if (is_wall(iyL, ix)) get_U_ghost_y(U0, iyR, ix, UL); else for (int l = 0; l < 4; l++) UL[l] = U0[l][iyL][ix];
        if (is_wall(iyR, ix)) get_U_ghost_y(U0, iyL, ix, UR); else for (int l = 0; l < 4; l++) UR[l] = U0[l][iyR][ix];
        double rL, uxL, uyL, eL, pL, rR, uxR, uyR, eR, pR;
        primitives_from_U4(UL, rL, uxL, uyL, eL, pL);
        primitives_from_U4(UR, rR, uxR, uyR, eR, pR);
        double EL = UL[3], ER = UR[3];
        double GL[4], GR[4];
        flux_y(rL, uxL, uyL, pL, EL, GL);
        flux_y(rR, uxR, uyR, pR, ER, GR);
        if (rL < rho_vacuum || rR < rho_vacuum) {
            // Outflow flux (mass + momentum at sound speed), same idea as x.
            bool L_is_vacuum = (rL < rho_vacuum && rR >= rho_vacuum);
            bool R_is_vacuum = (rR < rho_vacuum && rL >= rho_vacuum);
            double c_d;
            if (L_is_vacuum) {
                c_d = sound_speed(rR, pR);
                G[0] = -rR * c_d;
                G[1] = -rR * c_d * uxR;
                G[2] = rR * c_d * c_d;
                G[3] = -(ER + pR) * c_d;
            } else if (R_is_vacuum) {
                c_d = sound_speed(rL, pL);
                G[0] = rL * c_d;
                G[1] = rL * c_d * uxL;
                G[2] = rL * c_d * c_d;
                G[3] = (EL + pL) * c_d;
            } else {
                if (rL >= rR) {
                    c_d = sound_speed(rL, pL);
                    G[0] = rL * c_d; G[1] = rL * c_d * uxL; G[2] = rL * c_d * c_d; G[3] = (EL + pL) * c_d;
                } else {
                    c_d = sound_speed(rR, pR);
                    G[0] = -rR * c_d; G[1] = -rR * c_d * uxR; G[2] = rR * c_d * c_d; G[3] = -(ER + pR) * c_d;
                }
            }
            return;
        }
        double sL = std::abs(uyL) + sound_speed(rL, pL);
        double sR = std::abs(uyR) + sound_speed(rR, pR);
        double s_max = std::max(sL, sR);
        for (int l = 0; l < 4; l++)
            G[l] = 0.5 * (GL[l] + GR[l]) - 0.5 * s_max * (UR[l] - UL[l]);
    }

    // CFL: lam = max over cells of (|u| + c). Cap velocity contribution so one bad cell can't collapse dt (0-pressure safety).
    double max_lambda() const {
        const double v_max_mult = 5.0;  // max |u| contribution = this many × sound speed
        double lam = 0.0;
        for (int iy = 0; iy < ny; iy++) {
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) continue;
                double r, ux, uy, e, p;
                primitives(iy, ix, r, ux, uy, e, p);
                double c = sound_speed(r, p);
                double v_mag = std::abs(ux) + std::abs(uy);
                double v_capped = std::min(v_mag, v_max_mult * c);
                double v_contrib = v_capped + c;
                lam = std::max(lam, v_contrib);
            }
        }
        return (lam > 1e-12) ? lam : 1.0;
    }

    // Diffusion limit (viscous only): dt < 0.25*dx² / (μ/ρ)
    double max_diffusion_dt() const {
        double nu_max = mu / rho_min;
        return 0.25 * dx * dx / std::max(nu_max, 1e-30);
    }

    // Add viscous stress only to U in place. At wall faces use reflective ghost (zero flux through wall).
    void apply_viscous(std::array<Mat2, 4>& U, double dt) const {
        const int n = ny * nx;
        std::vector<double> u(n), v(n), txx(n), tyy(n), txy(n), FE(n), GE(n);
        for (int iy = 0; iy < ny; iy++) {
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) { u[iy*nx+ix]=0; v[iy*nx+ix]=0; txx[iy*nx+ix]=tyy[iy*nx+ix]=txy[iy*nx+ix]=FE[iy*nx+ix]=GE[iy*nx+ix]=0; continue; }
                double r, ux, uy, e, p;
                primitives_from(U, iy, ix, r, ux, uy, e, p);
                int i = iy * nx + ix;
                u[i] = ux; v[i] = uy;
                int ixp = (ix + 1) % nx, ixm = (ix - 1 + nx) % nx;
                int iyp = (iy + 1) % ny, iym = (iy - 1 + ny) % ny;
                double uP = is_wall(iy, ixp) ? -ux : (U[1][iy][ixp] / std::max(U[0][iy][ixp], rho_min));
                double uM = is_wall(iy, ixm) ? -ux : (U[1][iy][ixm] / std::max(U[0][iy][ixm], rho_min));
                double vP = is_wall(iy, ixp) ? uy : (U[2][iy][ixp] / std::max(U[0][iy][ixp], rho_min));
                double vM = is_wall(iy, ixm) ? uy : (U[2][iy][ixm] / std::max(U[0][iy][ixm], rho_min));
                double dudx = (uP - uM) / (2.0 * dx);
                double dvdx = (vP - vM) / (2.0 * dx);
                uP = is_wall(iyp, ix) ? ux : (U[1][iyp][ix] / std::max(U[0][iyp][ix], rho_min));
                uM = is_wall(iym, ix) ? ux : (U[1][iym][ix] / std::max(U[0][iym][ix], rho_min));
                vP = is_wall(iyp, ix) ? -uy : (U[2][iyp][ix] / std::max(U[0][iyp][ix], rho_min));
                vM = is_wall(iym, ix) ? -uy : (U[2][iym][ix] / std::max(U[0][iym][ix], rho_min));
                double dudy = (uP - uM) / (2.0 * dx);
                double dvdy = (vP - vM) / (2.0 * dx);
                double div = dudx + dvdy;
                txx[i] = 2.0 * mu * dudx - (2.0/3.0) * mu * div;
                tyy[i] = 2.0 * mu * dvdy - (2.0/3.0) * mu * div;
                txy[i] = mu * (dudy + dvdx);
                FE[i] = u[i]*txx[i] + v[i]*txy[i];
                GE[i] = u[i]*txy[i] + v[i]*tyy[i];
            }
        }
        for (int iy = 0; iy < ny; iy++) {
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) continue;
                int i = iy * nx + ix;
                int ixp = (ix + 1) % nx, ixm = (ix - 1 + nx) % nx;
                int iyp = (iy + 1) % ny, iym = (iy - 1 + ny) % ny;
                int ipx = iy * nx + ixp, imx = iy * nx + ixm;
                int ipy = iyp * nx + ix, imy = iym * nx + ix;
                double div_tau_x = (txx[ipx] - txx[imx]) / (2.0 * dx) + (txy[ipy] - txy[imy]) / (2.0 * dx);
                double div_tau_y = (txy[ipx] - txy[imx]) / (2.0 * dx) + (tyy[ipy] - tyy[imy]) / (2.0 * dx);
                double div_FE = (FE[ipx] - FE[imx]) / (2.0 * dx) + (GE[ipy] - GE[imy]) / (2.0 * dx);
                U[1][iy][ix] += dt * div_tau_x;
                U[2][iy][ix] += dt * div_tau_y;
                U[3][iy][ix] += dt * div_FE;
            }
        }
    }

    // Thermal diffusion (heat.cpp); call after step() with same dt. Adiabatic at walls.
    void apply_heat_diffusion(double dt) {
        heat::heat_diffusion_step(ny, nx, dx, dt, U0[0], U0[1], U0[2], U0[3], &wall);
    }

    bool state_valid(int* out_iy, int* out_ix, double* out_rho, double* out_e) const {
        for (int iy = 0; iy < ny; iy++)
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) continue;
                if (U0[0][iy][ix] < rho_min) {
                    if (out_iy) *out_iy = iy;
                    if (out_ix) *out_ix = ix;
                    if (out_rho) *out_rho = U0[0][iy][ix];
                    if (out_e) *out_e = -1.0;
                    return false;
                }
                double r = U0[0][iy][ix], E = U0[3][iy][ix];
                double ke = 0.5 * (U0[1][iy][ix]*U0[1][iy][ix] + U0[2][iy][ix]*U0[2][iy][ix]) / (r * r);
                double e_int = E / r - ke;
                if (e_int < e_min) {
                    if (out_iy) *out_iy = iy;
                    if (out_ix) *out_ix = ix;
                    if (out_rho) *out_rho = r;
                    if (out_e) *out_e = e_int;
                    return false;
                }
            }
        return true;
    }
    bool state_valid() const { return state_valid(nullptr, nullptr, nullptr, nullptr); }

    // Try to repair cells with e_int < e_min by dissipating kinetic energy into internal energy (E stays constant).
    // This avoids creating energy: we only scale down momentum so that e_int >= e_min where possible.
    void fix_invalid_cells_dissipative() {
        for (int iy = 0; iy < ny; ++iy) {
            for (int ix = 0; ix < nx; ++ix) {
                if (is_wall(iy, ix)) continue;
                double r = std::max(U0[0][iy][ix], rho_min);
                double mx = U0[1][iy][ix];
                double my = U0[2][iy][ix];
                double E = U0[3][iy][ix];
                double M2 = mx * mx + my * my;
                if (M2 <= 0.0) continue;
                double ke = 0.5 * M2 / (r * r);
                double e_int = E / r - ke;
                if (e_int >= e_min) continue;
                double e_tot = E / r;              // e + ke
                double ke_max = e_tot - e_min;     // max KE so that e_int >= e_min
                if (ke_max <= 0.0) continue;       // cannot fix without adding energy; leave for validity check
                if (ke <= 0.0) continue;
                double f = ke_max / ke;
                if (f >= 1.0) continue;            // already fine numerically
                if (f < 0.0) f = 0.0;
                double s = std::sqrt(f);
                U0[1][iy][ix] = mx * s;
                U0[2][iy][ix] = my * s;
            }
        }
    }

    // One step (convection + viscosity); returns dt used. Heat diffusion is separate (apply_heat_diffusion).
    double step() {
        double lam = max_lambda();
        double dt_cfl = CFL * dx / lam;
        double dt_diff = max_diffusion_dt();
        double dt = std::min({ dt_cfl, dt_diff, 0.01 });

        if (dt < 1e-3)
            AIR_DBG("dt tiny: lam=%.6e dt_cfl=%.6e dt=%.6e (frame advance tiny => stuck)\n", lam, dt_cfl, dt);

        for (int iy = 0; iy < ny; iy++) {
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) {
                    for (int l = 0; l < 4; l++) U1[l][iy][ix] = U0[l][iy][ix];
                    continue;
                }
                int ixp = (ix + 1) % nx, ixm = (ix - 1 + nx) % nx;
                int iyp = (iy + 1) % ny, iym = (iy - 1 + ny) % ny;

                double Fp[4], Fm[4], Gp[4], Gm[4];
                rusanov_x(iy, ix, ixp, Fp);
                rusanov_x(iy, ixm, ix, Fm);
                rusanov_y(iy, iyp, ix, Gp);
                rusanov_y(iym, iy, ix, Gm);

                // Cap net outgoing energy flux so this cell never drops below e_int >= e_min (no flooring = no energy creation).
                double r, ux, uy, e, p;
                primitives_from(U0, iy, ix, r, ux, uy, e, p);
                double ke = 0.5 * (U0[1][iy][ix]*U0[1][iy][ix] + U0[2][iy][ix]*U0[2][iy][ix]) / r;
                double E_min_cell = r * e_min + ke;
                double net_E_flux_out = (Fp[3] - Fm[3] + Gp[3] - Gm[3]);
                if (net_E_flux_out > 0) {
                    double max_E_out = (U0[3][iy][ix] - E_min_cell) * (dx / dt);
                    if (max_E_out <= 0) {
                        Fp[3] = Fm[3] = Gp[3] = Gm[3] = 0;
                    } else if (net_E_flux_out > max_E_out) {
                        double factor = max_E_out / net_E_flux_out;
                        Fp[3] *= factor; Fm[3] *= factor; Gp[3] *= factor; Gm[3] *= factor;
                    }
                }

                for (int l = 0; l < 4; l++)
                    U1[l][iy][ix] = U0[l][iy][ix] - (dt / dx) * (Fp[l] - Fm[l] + Gp[l] - Gm[l]);
            }
        }
        apply_viscous(U1, dt);
        std::swap(U0, U1);
        // First try a local, dissipative repair: reduce KE where e_int < e_min without changing E.
        fix_invalid_cells_dissipative();
        int bad_iy = -1, bad_ix = -1;
        double bad_rho = 0, bad_e = 0;
        if (!state_valid(&bad_iy, &bad_ix, &bad_rho, &bad_e)) {
            AIR_DBG("state INVALID at iy=%d ix=%d rho=%.6e e_int=%.6e (e_min=%.6e) => reject step\n", bad_iy, bad_ix, bad_rho, bad_e, e_min);
            std::swap(U0, U1);
            return 0.0;
        }
        return dt;
    }

    double total_mass() const {
        double sum = 0;
        for (int iy = 0; iy < ny; iy++)
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) continue;
                sum += U0[0][iy][ix];
            }
        return sum * dx * dx;
    }

    double total_energy() const {
        double sum = 0;
        for (int iy = 0; iy < ny; iy++)
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) continue;
                sum += U0[3][iy][ix];
            }
        return sum * dx * dx;
    }

    void set_uniform(double rho, double ux, double uy, double p) {
        double e = p / ((gamma_gas - 1.0) * rho);
        double E = rho * e + 0.5 * rho * (ux*ux + uy*uy);
        for (int iy = 0; iy < ny; iy++)
            for (int ix = 0; ix < nx; ix++) {
                if (is_wall(iy, ix)) continue;
                U0[0][iy][ix] = rho;
                U0[1][iy][ix] = rho * ux;
                U0[2][iy][ix] = rho * uy;
                U0[3][iy][ix] = E;
            }
    }

    void add_bump(int cy, int cx, double dP, int rad) {
        for (int iy = std::max(0, cy - rad); iy < std::min(ny, cy + rad + 1); iy++)
            for (int ix = std::max(0, cx - rad); ix < std::min(nx, cx + rad + 1); ix++) {
                if (is_wall(iy, ix)) continue;
                double d = std::sqrt((iy - cy)*(iy - cy) + (ix - cx)*(ix - cx));
                if (d > rad) continue;
                double f = 1.0 - d / std::max(1, rad);
                double r, ux, uy, e, p;
                primitives(iy, ix, r, ux, uy, e, p);
                p += dP * f;
                r += (dP * f) / (gamma_gas * 287.0 * 300.0);
                r = std::max(r, rho_min);
                e = p / ((gamma_gas - 1.0) * r);
                double ke = 0.5 * (U0[1][iy][ix]*U0[1][iy][ix] + U0[2][iy][ix]*U0[2][iy][ix]) / r;
                U0[0][iy][ix] = r;
                U0[3][iy][ix] = r * e + ke;
            }
    }
};

#ifdef TPT_INTEGRATION
#include "air_solver_api.h"

static inline State* S(AirSolverState* p) { return reinterpret_cast<State*>(p); }
static inline const State* S(const AirSolverState* p) { return reinterpret_cast<const State*>(p); }

AirSolverState* air_solver_create(int ny, int nx, double dx) {
    return reinterpret_cast<AirSolverState*>(new State(ny, nx, dx));
}
void air_solver_destroy(AirSolverState* state) {
    delete S(state);
}
// Use pv (pressure) from TPT as the primary source of internal energy via ideal gas law: p = (γ−1)ρe.
// hv (air temperature) is driven by the solver; we don't treat it as an independent energy source here so that tools
// like VAC which edit pv directly have an immediate effect on the solver state.
// Ensure stored E is valid: E >= r*e_min + KE so we never inject invalid state (no hidden floors on e).
void air_solver_sync_from_tpt(AirSolverState* state,
    const float* pv, const float* vx, const float* vy, const float* rho, const unsigned char* wall,
    const float* hv, float game_vel_scale) {
    State* s = S(state);
    for (int iy = 0; iy < s->ny; iy++)
        for (int ix = 0; ix < s->nx; ix++) {
            int i = iy * s->nx + ix;
            s->set_wall(iy, ix, (wall[i] != 0));
            if (s->is_wall(iy, ix)) continue;
            double r = (double)rho[i];
            if (r < rho_min) r = rho_min;
            double ux = (double)vx[i] * (double)game_vel_scale;
            double uy = (double)vy[i] * (double)game_vel_scale;
            // Clamp extreme pressures for stability inside solver; visual pv can still show the full range.
            double p = std::clamp((double)pv[i], -p_max_solver, p_max_solver);
            // Ideal gas: p = (γ−1)ρe  => e = p /((γ−1)ρ).
            double e = p / ((gamma_gas - 1.0) * r);
            double ke = 0.5 * r * (ux*ux + uy*uy);
            double E = r * e + ke;
            double E_min_valid = r * e_min + ke;
            if (E < E_min_valid) E = E_min_valid;
            s->U0[0][iy][ix] = r;
            s->U0[1][iy][ix] = r * ux;
            s->U0[2][iy][ix] = r * uy;
            s->U0[3][iy][ix] = E;
        }
}
// Output: clamp p and T only for display so TPT never sees negative pressure or invalid T.
void air_solver_sync_to_tpt(AirSolverState* state,
    float* pv, float* vx, float* vy, float* hv, float* rho,
    float game_vel_scale) {
    const State* s = S(state);
    double inv_scale = 1.0 / (double)game_vel_scale;
    for (int iy = 0; iy < s->ny; iy++)
        for (int ix = 0; ix < s->nx; ix++) {
            int i = iy * s->nx + ix;
            if (s->is_wall(iy, ix)) continue;
            double r, ux, uy, e, p;
            s->primitives(iy, ix, r, ux, uy, e, p);
            pv[i] = (float)std::max(p, 1e-10);
            vx[i] = (float)(ux * inv_scale);
            vy[i] = (float)(uy * inv_scale);
            hv[i] = (float)s->temperature_from_e(e);
            rho[i] = (float)r;
        }
}
double air_solver_step(AirSolverState* state) {
    return S(state)->step();
}
void air_solver_apply_heat_diffusion(AirSolverState* state, double dt) {
    S(state)->apply_heat_diffusion(dt);
}
void air_solver_set_boundary_walls(AirSolverState* state) {
    S(state)->set_boundary_walls();
}
void air_solver_set_uniform(AirSolverState* state, double rho, double ux, double uy, double p) {
    S(state)->set_uniform(rho, ux, uy, p);
}

#else
// Headless test (default build)
#ifndef USE_SDL
int main(int argc, char** argv) {
    int ny = 40, nx = 60;
    if (argc >= 3) { ny = std::atoi(argv[1]); nx = std::atoi(argv[2]); }
    double dx = 1.0;

    State s(ny, nx, dx);
    s.set_uniform(1.2, 0.0, 0.0, 101325.0);
    s.add_bump(ny/2, nx/2, 50000.0, 5);

    double mass0 = s.total_mass(), energy0 = s.total_energy();
    int steps = 0;
    double sim_t = 0.0;
    const double t_end = 0.05;
    int reject = 0;

    while (sim_t < t_end) {
        double dt = s.step();
        if (dt <= 0.0) {
            reject++;
            if (reject > 100) break;
            continue;
        }
        reject = 0;
        s.apply_heat_diffusion(dt);
        sim_t += dt;
        steps++;
    }

    double mass1 = s.total_mass(), energy1 = s.total_energy();
    std::printf("Steps %d, t=%.6f, mass err=%.2e, energy err=%.2e\n",
                steps, sim_t, std::fabs(mass1 - mass0) / mass0, std::fabs(energy1 - energy0) / energy0);
    return 0;
}

#else
// SDL2 visualization: build with -DUSE_SDL $(pkg-config --cflags --libs sdl2)
#include <SDL2/SDL.h>

static uint32_t pressure_to_color(double p, double p_min, double p_max) {
    if (p_max <= p_min) return 0xFF000000;
    double t = (p - p_min) / (p_max - p_min);
    if (t < 0.0) t = 0.0; else if (t > 1.0) t = 1.0;
    uint8_t r, g, b;
    if (t < 0.33) {
        double s = t / 0.33;
        r = 0; g = (uint8_t)(s * 255); b = 255;
    } else if (t < 0.67) {
        double s = (t - 0.33) / 0.34;
        r = (uint8_t)(s * 255); g = 255; b = (uint8_t)((1.0 - s) * 255);
    } else {
        double s = (t - 0.67) / 0.33;
        r = 255; g = (uint8_t)((1.0 - s) * 255); b = 0;
    }
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    const int ny = 80, nx = 120;
    const int scale = 6;
    const int win_w = nx * scale, win_h = ny * scale;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow("Rusanov 2D compressible", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h, SDL_WINDOW_SHOWN);
    if (!win) { SDL_Quit(); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) { SDL_DestroyWindow(win); SDL_Quit(); return 1; }
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, nx, ny);
    if (!tex) { SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit(); return 1; }

    State s(ny, nx, 1.0);
    s.set_boundary_walls();  // reflective walls at edges (no periodic wrap)
    s.set_uniform(1.2, 0.0, 0.0, 101325.0);
    s.add_bump(ny/2, nx/2, 50000.0, 8);
    const double mass0 = s.total_mass(), energy0 = s.total_energy();

    double p_min = 80000.0, p_max = 200000.0;
    bool quit = false;
    bool paused = false;
    Uint32 last_ticks = SDL_GetTicks();
    int frame_count = 0;

    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) quit = true;
                if (e.key.keysym.sym == SDLK_SPACE) paused = !paused;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                int mx = e.button.x / scale, my = e.button.y / scale;
                if (mx >= 0 && mx < nx && my >= 0 && my < ny) {
                    if (e.button.button == SDL_BUTTON_LEFT)
                        s.add_bump(my, mx, 80000.0, 6);
                    else if (e.button.button == SDL_BUTTON_RIGHT)
                        s.set_wall(my, mx, true);   // add wall cell
                    else if (e.button.button == SDL_BUTTON_MIDDLE)
                        s.set_wall(my, mx, false);  // remove wall cell
                }
            }
            if (e.type == SDL_MOUSEMOTION) {
                int mx = e.motion.x / scale, my = e.motion.y / scale;
                if (mx >= 0 && mx < nx && my >= 0 && my < ny) {
                    if (e.motion.state & SDL_BUTTON_RMASK)
                        s.set_wall(my, mx, true);   // drag to paint walls
                    else if (e.motion.state & SDL_BUTTON_MMASK)
                        s.set_wall(my, mx, false);  // drag to erase walls
                }
            }
        }

        if (!paused) {
            int steps_this_frame = 0;
            double advance = 0.0;
            const double target_advance = 0.02;
            int reject = 0;
            while (steps_this_frame < 15 && advance < target_advance) {
                double dt = s.step();
                if (dt <= 0.0) {
                    if (++reject > 5) break;
                    continue;
                }
                reject = 0;
                s.apply_heat_diffusion(dt);
                advance += dt;
                steps_this_frame++;
            }
        }

        p_min = 1e30; p_max = -1e30;
        for (int iy = 0; iy < ny; iy++)
            for (int ix = 0; ix < nx; ix++) {
                if (s.is_wall(iy, ix)) continue;
                double r, ux, uy, e, p;
                s.primitives(iy, ix, r, ux, uy, e, p);
                if (p < p_min) p_min = p;
                if (p > p_max) p_max = p;
            }
        if (p_max <= p_min) p_max = p_min + 1.0;

        std::vector<uint32_t> pixels(nx * ny);
        for (int iy = 0; iy < ny; iy++)
            for (int ix = 0; ix < nx; ix++) {
                if (s.is_wall(iy, ix)) { pixels[iy * nx + ix] = 0xFF202020; continue; }
                double r, ux, uy, e, p;
                s.primitives(iy, ix, r, ux, uy, e, p);
                pixels[iy * nx + ix] = pressure_to_color(p, p_min, p_max);
            }
        SDL_UpdateTexture(tex, nullptr, pixels.data(), nx * sizeof(uint32_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);

        double mass = s.total_mass(), energy = s.total_energy();
        double mass_err = (mass0 > 1e-30) ? std::fabs(mass - mass0) / mass0 : 0.0;
        double energy_err = (energy0 > 1e-30) ? std::fabs(energy - energy0) / energy0 : 0.0;
        char title[128];
        std::snprintf(title, sizeof(title), "Rusanov 2D | mass err=%.2e  energy err=%.2e", mass_err, energy_err);
        SDL_SetWindowTitle(win, title);
        std::fprintf(stderr, "frame %d  mass err=%.2e  energy err=%.2e\r", frame_count++, mass_err, energy_err);
        std::fflush(stderr);

        Uint32 now = SDL_GetTicks();
        int elapsed = (int)(now - last_ticks);
        if (elapsed < 16) SDL_Delay(16 - elapsed);
        last_ticks = SDL_GetTicks();
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
#endif
#endif
