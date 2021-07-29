#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PHSP() {
	Identifier = "DEFAULT_PT_PHSP";
	Name = "PHSP";
	Colour = PIXPACK(0xEDD5D3);
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 2;
	PhotonReflectWavelengths = 0xFFFFFFFF;

	Weight = 90;

	HeatConduct = 150;
	Description = "Phosphorus. Highly flammable powder.";

	Properties = TYPE_PART | PROP_SPARKSETTLE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 280.5f + 273.15f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * life: Fire burn
	 * tmp:  Phot absorb timer
	 * tmp2: Is red variant
	 */
	if (parts[i].life == 1) {
		sim->kill_part(i);
		return 1;
	}
	if (parts[i].life > 1)
		parts[i].life--;
	if (parts[i].tmp > 0)
		parts[i].tmp--;

	// Change to red PHSP if hot
	if (parts[i].temp > 70.0f + 273.15f && RNG::Ref().chance(1, 2000))
		parts[i].tmp2 = 1;

	int rx, ry, r;
	for (rx = -1; rx <= 1; ++rx)
	for (ry = -1; ry <= 1; ++ry)
		if (BOUNDS_CHECK && (rx || ry)) {
			r = pmap[y + ry][x + rx];

			// White PHSP random combustion
			if (!r && parts[i].tmp2 == 0 && RNG::Ref().chance(1, 1000000)) {
				sim->part_change_type(i, x, y, PT_FIRE);
				parts[i].life = 100;
				parts[i].temp += 200.0f;
				return 1;
			}

			// Ignite
			if (TYP(r) == PT_FIRE || TYP(r) == PT_PLSM || TYP(r) == PT_LAVA || (TYP(r) == PT_O2 && parts[i].tmp2 == 0 )) {
				parts[i].life += RNG::Ref().between(0, 5);
			}
			// Burn
			else if (parts[i].life > 0 && !r) {
				int ni = sim->create_part(-1, x + rx, y + ry, RNG::Ref().chance(1, 3) ? PT_EMBR : PT_FIRE);
				if (ni > -1) {
					parts[ni].life = RNG::Ref().between(50, 200);
					parts[ni].vx = rx;
					parts[ni].vy = ry;
					parts[ni].temp = parts[i].temp + 1500.0f;
					parts[ni].dcolour = 0xFFFFB37D;
					parts[ni].tmp2 = 1;
				}
			}
			// Fertilize plnt
			else if (TYP(r) == PT_PLNT) {
				sim->part_change_type(ID(r), x + rx, y + ry, PT_VINE);
				sim->kill_part(i);
				return 1;
			}
			// Glow when touching WTRV
			if (TYP(r) == PT_WTRV)
				parts[i].tmp++;

			// Absorb PHOT
			r = sim->photons[y + ry][x + rx];
			if (TYP(r) == PT_PHOT && parts[i].tmp2 == 0) {
				sim->kill_part(ID(r));
				parts[i].tmp += 30;
				if (RNG::Ref().chance(1, 10))
					parts[i].tmp2 = 1;
			}
		}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	if (cpart->tmp2 == 0 && cpart->tmp > 0) {
		*firea = 150;
		*firer = *fireg = *fireb = 255;
		*pixel_mode |= FIRE_ADD;
	}
	else if (cpart->tmp2) {
		// Red variant
		*colr = 194, *colg = 51, *colb = 19;
	}
	return 0;
}
