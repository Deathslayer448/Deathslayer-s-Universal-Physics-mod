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
    for (int iy = 0; iy < ny; iy++) {
        for (int ix = 0; ix < nx; ix++) {
            if (is_wall(iy, ix)) continue;
            int ixp = (ix + 1) % nx, ixm = (ix - 1 + nx) % nx;
            int iyp = (iy + 1) % ny, iym = (iy - 1 + ny) % ny;
            double Tp = is_wall(iy, ixp) ? T[iy*nx+ix] : T[iy*nx+ixp];
            double Tm = is_wall(iy, ixm) ? T[iy*nx+ix] : T[iy*nx+ixm];
            double Tpy = is_wall(iyp, ix) ? T[iy*nx+ix] : T[iyp*nx+ix];
            double Tmy = is_wall(iym, ix) ? T[iy*nx+ix] : T[iym*nx+ix];
            double lap_T = (Tp + Tm + Tpy + Tmy - 4.0 * T[iy*nx+ix]) / (dx * dx);
            E[iy][ix] += dt * k_thermal * lap_T;
        }
    }
}

} // namespace heat
