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

// Phase 1.2: Conservative flux form — each face flux computed once; zero flux at walls; sum dE/dt = 0.
void heat_diffusion_step(int ny, int nx, double dx, double dt,
    const Grid2& rho, const Grid2& rhou, const Grid2& rhov, Grid2& E,
    const WallMask* wall) {
    const double e_min = 1e-6;
    auto is_wall = [&](int iy, int ix) -> bool {
        if (!wall) return false;
        return (iy >= 0 && iy < ny && ix >= 0 && ix < nx) && (*wall)[iy][ix];
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
    // Face fluxes (W/m²): F = k*(T_R - T_L)/dx; zero at wall faces.
    std::vector<double> Fx(ny * (nx + 1), 0.0);
    std::vector<double> Fy((ny + 1) * nx, 0.0);
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx - 1; ix++) {
            if (is_wall(iy, ix) && is_wall(iy, ix + 1)) continue;
            double TL = is_wall(iy, ix) ? T[iy*nx+ix+1] : T[iy*nx+ix];
            double TR = is_wall(iy, ix+1) ? T[iy*nx+ix] : T[iy*nx+ix+1];
            if (!is_wall(iy, ix) && !is_wall(iy, ix+1))
                Fx[iy*(nx+1)+ix+1] = k_thermal * (TR - TL) / dx;
        }
    }
    for (int iy = 0; iy < ny - 1; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix) && is_wall(iy+1, ix)) continue;
            double TL = is_wall(iy, ix) ? T[(iy+1)*nx+ix] : T[iy*nx+ix];
            double TR = is_wall(iy+1, ix) ? T[iy*nx+ix] : T[(iy+1)*nx+ix];
            if (!is_wall(iy, ix) && !is_wall(iy+1, ix))
                Fy[(iy+1)*nx+ix] = k_thermal * (TR - TL) / dx;
        }
    }
    // dE/dt = (flux_in - flux_out)/dx; F in W/m² so div_F in W/m³ = J/(s·m³).
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix)) continue;
            double div_F = 0.0;
            // x: flux at face (ix-1,ix) minus flux at face (ix,ix+1)
            if (ix > 0)        div_F += Fx[iy*(nx+1)+ix] / dx;
            if (ix < nx - 1)   div_F -= Fx[iy*(nx+1)+ix+1] / dx;
            // y: flux at face (iy-1,iy) minus flux at face (iy,iy+1)
            if (iy > 0)        div_F += Fy[iy*nx+ix] / dx;
            if (iy < ny - 1)   div_F -= Fy[(iy+1)*nx+ix] / dx;
            E[iy][ix] += dt * div_F;
        }
    }
}

} // namespace heat
