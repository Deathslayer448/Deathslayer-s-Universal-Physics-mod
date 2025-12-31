#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_AMNA() {
	Identifier = "DEFAULT_PT_AMNA";
	Name = "AMNA";
	Colour = 0x84E3DD_rgb;
	MenuVisible = 0;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = 0.00f;
	Diffusion = 1.50f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 2;
	HeatConduct = 20;
	Description = "Ammonia. Created from nitrogen and hydrogen, decomposes in an endothermic reaction.";

	Properties = TYPE_GAS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 30.0f;
	HighPressureTransition = PT_LQUD;
	LowTemperature = 273.15f - 33.34f;
	LowTemperatureTransition = PT_LQUD;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	BoilingPoint = 273.15f - 33.34f;
	MeltingPoint = 273.15f - 77.73f;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	for (int rx = -1; rx <= 1; rx++)
	for (int ry = -1; ry <= 1; ry++)
		if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
			int r = pmap[y + ry][x + rx];
			if (!r) continue;
			int rt = TYP(r);

			if (rt == PT_ACID || rt == PT_HCL) {
				sim->part_change_type(ID(r), x + rx, y + ry, PT_DSTW);
				sim->part_change_type(i, x, y, PT_DSTW);
				parts[i].temp += 1.0f;
				parts[ID(r)].temp += 1.0f;
				return 1;
			}
			else if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_WATR || rt == PT_WTRV) {
				sim->part_change_type(ID(r), x + rx, y + ry, PT_SOAP);
				sim->part_change_type(i, x, y, PT_SOAP);
				return 1;
			}
			else if (rt == PT_PLNT)
				sim->part_change_type(ID(r), x + rx, y + ry, PT_VINE);
		}

	// Break apart
	if (sim->rng.chance(1, 250) && (parts[i].temp > 500.0f + 273.15f || sim->pv[y / CELL][x / CELL] < -15.0f)) {
		sim->part_change_type(i, x, y, sim->rng.chance(1, 3) ? PT_O2 : PT_H2);
		parts[i].temp -= 15.0f;
		return 1;
	}

	return 0;
}
