#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_VNGR() {
	Identifier = "DEFAULT_PT_VNGR";
	Name = "VNGR";
	Colour = PIXPACK(0xDBB797);
	MenuVisible = 0;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Mass = 30/1000.0f;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	HeatConduct = 29;
	Description = "Vinegar.";

	Properties = TYPE_LIQUID | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f + 16.6f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 273.15f + 118.1f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	/*int rx, ry, r, rt;
	for (rx = -1; rx <= 1; rx++)
	for (ry = -1; ry <= 1; ry++)
		if (BOUNDS_CHECK && (rx || ry)) {
			r = pmap[y + ry][x + rx];
			if (!r) continue;
			rt = TYP(r);

			if (rt == PT_BAKS && RNG::Ref().chance(1, 50)) {
				sim->pv[y / CELL][x / CELL] += 0.6f;
				sim->part_change_type(i, x, y, PT_FOAM);
				sim->part_change_type(ID(r), x + rx, y + ry, PT_FOAM);
				parts[i].tmp = parts[ID(r)].tmp = 70;
				parts[i].life = parts[ID(r)].life = 200;
				parts[i].temp += 5.0f;
				return 1;
			}
		}*/

	return 0;
}
