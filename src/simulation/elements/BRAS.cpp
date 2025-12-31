#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_BRAS() {
	Identifier = "DEFAULT_PT_BRAS";
	Name = "BRAS";
	Colour = 0xE3C359_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 20;

	Weight = 100;

	HeatConduct = 251;
	Description = "Brass. Kills bacteria, made from mixing copper and zinc. Accelerates air.";
	PhotonReflectWavelengths = 0xFFA0000;

	Properties = TYPE_SOLID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = ITH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 900.0f + 273.15f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	sim->vx[y / CELL][x / CELL] *= 1.002f;
	sim->vy[y / CELL][x / CELL] *= 1.002f;
	return 0;
}
