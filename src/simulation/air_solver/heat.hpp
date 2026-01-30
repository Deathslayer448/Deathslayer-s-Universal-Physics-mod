/**
 * Heat physics — thermal diffusion, heat capacity, temperature.
 * Thermal diffusion and heat capacity. Separate module from the gas solver.
 */

#ifndef HEAT_HPP
#define HEAT_HPP

#include <vector>

namespace heat {

// Wall mask: true = solid (adiabatic, no heat flux through face). Optional: pass nullptr for periodic.
using WallMask = std::vector<std::vector<bool>>;

// Thermal constants (ideal gas, air)
const double R_gas = 287.0;           // J/(kg·K)
const double gamma_gas = 1.4;
const double c_v = R_gas / (gamma_gas - 1.0);   // heat capacity at constant volume
const double c_p = gamma_gas * R_gas / (gamma_gas - 1.0);  // at constant pressure
const double Pr = 0.71;               // Prandtl number
const double mu_ref = 1.8e-5;          // Pa·s (for k = mu*c_p/Pr)
const double k_thermal = mu_ref * c_p / Pr;     // W/(m·K)

using Grid2 = std::vector<std::vector<double>>;

// T from internal energy e (J/kg): T = e / c_v
inline double temperature_from_e(double e, double e_min = 1e-6) {
    return std::max(e, e_min) / c_v;
}

// Max dt for thermal diffusion stability: 0.25*dx² / (k/(ρ*c_p))
double max_heat_dt(int ny, int nx, double dx, double rho_min = 1e-6);

// Add k∇²T to energy: E += dt * k * Laplacian(T). At wall faces: adiabatic (zero flux, use T_self for neighbor).
// wall: nullptr = periodic; else wall[iy][ix]==true means solid (skip update, adiabatic at face).
void heat_diffusion_step(int ny, int nx, double dx, double dt,
    const Grid2& rho, const Grid2& rhou, const Grid2& rhov, Grid2& E,
    const WallMask* wall = nullptr);

} // namespace heat

#endif
