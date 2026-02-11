/**
 * Heat physics — implementation.
 */

#include "heat.hpp"
#include <cmath>
#include <algorithm>

namespace heat {

double max_heat_dt(int ny, int nx, double dx, double rho_min) {
    (void)ny;
    (void)nx;
    double alpha_max = k_thermal / (rho_min * c_p);
    return 0.25 * dx * dx / std::max(alpha_max, 1e-30);
}

// Phase 1.2: Conservative flux form — each face flux computed once. At walls: adiabatic unless wall_T given (heat from hot walls into fluid).
// wall_blocks_heat: when set, only cells with wall_blocks_heat[iy][ix]==false (particles) get convective transfer; actual walls block heat.
// wall_heat_lost: if non-null, accumulated energy lost from each wall cell (J/m per unit depth) so TPT can cool particles.
void heat_diffusion_step(int ny, int nx, double dx, double dt,
    const Grid2& rho, const Grid2& rhou, const Grid2& rhov, Grid2& E,
    const WallMask* wall, const Grid2* wall_T, const WallMask* wall_blocks_heat, Grid2* wall_heat_lost, double heat_scale) {
    const double e_min = 1e-6;
    double dt_eff = dt * std::max(heat_scale, 0.1);
    auto is_wall = [&](int iy, int ix) -> bool {
        if (!wall) return false;
        return (iy >= 0 && iy < ny && ix >= 0 && ix < nx) && (*wall)[iy][ix];
    };
    double rho_min = 1e30;
    for (int iy = 0; iy < ny; iy++)
        for (int ix = 0; ix < nx; ix++)
            if (!is_wall(iy, ix))
                rho_min = std::min(rho_min, std::max(rho[iy][ix], 1e-30));
    rho_min = std::max(rho_min, 1e-30);
    dt_eff = std::min(dt_eff, max_heat_dt(ny, nx, dx, rho_min));
    auto wall_temperature = [&](int iy, int ix) -> double {
        if (!wall_T || iy < 0 || iy >= ny || ix < 0 || ix >= nx) return 300.0;
        return (*wall_T)[iy][ix];
    };
    std::vector<double> T(ny * nx);
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix)) { T[iy*nx+ix] = 0; continue; }
            double r = std::max(rho[iy][ix], 1e-30);
            double ux = rhou[iy][ix] / r, uy = rhov[iy][ix] / r;
            double E_c = E[iy][ix];
            double ke = 0.5 * r * (ux*ux + uy*uy);
            double e = std::max((E_c - ke) / r, e_min);
            T[iy * nx + ix] = temperature_from_e(e, e_min);
        }
    }
    // Face fluxes (W/m²). Fluid–fluid only: Fourier F = k*(T_L - T_R)/dx.
    // Wall–fluid is applied explicitly below so it cannot be blocked by flux indexing.
    std::vector<double> Fx(ny * (nx + 1), 0.0);
    std::vector<double> Fy((ny + 1) * nx, 0.0);
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx - 1; ix++) {
            if (is_wall(iy, ix) || is_wall(iy, ix + 1)) continue;  // skip any face touching a wall
            double TL = T[iy*nx+ix], TR = T[iy*nx+ix+1];
            Fx[iy*(nx+1)+ix+1] = k_thermal * (TL - TR) / dx;
        }
    }
    for (int iy = 0; iy < ny - 1; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix) || is_wall(iy+1, ix)) continue;
            double TL = T[iy*nx+ix], TR = T[(iy+1)*nx+ix];
            Fy[(iy+1)*nx+ix] = k_thermal * (TL - TR) / dx;
        }
    }
    // Fluid–fluid: dE/dt = div(flux); E is J/m³.
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix)) continue;
            double div_F = 0.0;
            if (ix > 0)        div_F += Fx[iy*(nx+1)+ix] / dx;
            if (ix < nx - 1)   div_F -= Fx[iy*(nx+1)+ix+1] / dx;
            if (iy > 0)        div_F += Fy[iy*nx+ix] / dx;
            if (iy < ny - 1)   div_F -= Fy[(iy+1)*nx+ix] / dx;
            E[iy][ix] += dt_eff * div_F;
        }
    }
    // Wall→fluid: natural-convection heat transfer (Nusselt correlation). h = Nu*k/L, Nu = f(Gr, Pr).
    // Gr = g*β*ΔT*L³/ν², Ra = Gr*Pr; β = 1/T (ideal gas), ν = μ/ρ so density (pressure) increases h.
    // Below this density we skip wall→fluid transfer: same flux into near-vacuum gives huge T (E/ρ) and instability; wall also shouldn't cool into vacuum.
    const double rho_vacuum_no_wall_heat = 0.02;  // kg/m³ (~1.7 kPa at 300 K)
    auto wall_blocks_heat_at = [&](int iy, int ix) -> bool {
        if (!wall_blocks_heat || iy < 0 || iy >= ny || ix < 0 || ix >= nx) return true;
        return (*wall_blocks_heat)[iy][ix];
    };
    // Minimum h = conduction across half-cell so we never do worse than pure conduction at the interface.
    const double h_min_conduction = 2.0 * k_thermal / std::max(dx, 1e-10);
    auto convective_h = [&](double T_wall, double T_fluid, double rho_fluid, double L) -> double {
        const double T_avg = std::max(0.5 * (T_wall + T_fluid), 200.0);  // K, avoid 0
        const double delta_T = std::max(std::fabs(T_wall - T_fluid), 1.0);  // K, avoid 0 Gr
        const double beta = 1.0 / T_avg;  // ideal gas volumetric expansion
        const double rho = std::max(rho_fluid, 1e-6);
        const double nu = mu_ref / rho;  // kinematic viscosity; higher pressure → smaller ν → larger Gr
        const double Gr = g_natural * beta * delta_T * (L * L * L) / (nu * nu);
        const double Ra = Gr * Pr;
        const double Ra_clamp = std::min(std::max(Ra, 1e-6), 1e12);  // laminar–turbulent range
        // Laminar natural convection (vertical surface): Nu = 0.68 + 0.670*Ra^(1/4) / [1+(0.492/Pr)^(9/16)]^(4/9)
        const double denom = std::pow(1.0 + std::pow(0.492 / Pr, 9.0/16.0), 4.0/9.0);
        const double Nu = std::max(0.5, 0.68 + (0.670 * std::pow(Ra_clamp, 0.25)) / denom);
        const double h_nu = Nu * k_thermal / std::max(L, 1e-10);
        return std::max(h_nu, h_min_conduction);
    };
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix)) continue;
            const double T_fluid = T[iy*nx+ix];
            const double rho_fluid = std::max(rho[iy][ix], 1e-6);
            // No wall→fluid heat into near-vacuum: avoids huge T (E/ρ) and flicker; wall (e.g. TTAN) won't cool into vacuum.
            if (rho_fluid < rho_vacuum_no_wall_heat) continue;
            double rate = 0.0;
            if (ix > 0        && is_wall(iy, ix - 1) && !wall_blocks_heat_at(iy, ix - 1)) {
                double Tw = wall_temperature(iy, ix - 1);
                double h = convective_h(Tw, T_fluid, rho_fluid, dx);
                double flux = h * (Tw - T_fluid);
                rate += flux / dx;
                if (wall_heat_lost) (*wall_heat_lost)[iy][ix - 1] += flux * dx * dt_eff;  // J/m per unit depth
            }
            if (ix < nx - 1   && is_wall(iy, ix + 1) && !wall_blocks_heat_at(iy, ix + 1)) {
                double Tw = wall_temperature(iy, ix + 1);
                double h = convective_h(Tw, T_fluid, rho_fluid, dx);
                double flux = h * (Tw - T_fluid);
                rate += flux / dx;
                if (wall_heat_lost) (*wall_heat_lost)[iy][ix + 1] += flux * dx * dt_eff;
            }
            if (iy > 0        && is_wall(iy - 1, ix) && !wall_blocks_heat_at(iy - 1, ix)) {
                double Tw = wall_temperature(iy - 1, ix);
                double h = convective_h(Tw, T_fluid, rho_fluid, dx);
                double flux = h * (Tw - T_fluid);
                rate += flux / dx;
                if (wall_heat_lost) (*wall_heat_lost)[iy - 1][ix] += flux * dx * dt_eff;
            }
            if (iy < ny - 1   && is_wall(iy + 1, ix) && !wall_blocks_heat_at(iy + 1, ix)) {
                double Tw = wall_temperature(iy + 1, ix);
                double h = convective_h(Tw, T_fluid, rho_fluid, dx);
                double flux = h * (Tw - T_fluid);
                rate += flux / dx;
                if (wall_heat_lost) (*wall_heat_lost)[iy + 1][ix] += flux * dx * dt_eff;
            }
            E[iy][ix] += dt_eff * rate;
        }
    }
}

} // namespace heat
