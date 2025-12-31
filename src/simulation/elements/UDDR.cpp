#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
int Element_FLSH_update(UPDATE_FUNC_ARGS);

void Element::Element_UDDR() {
	Identifier = "DEFAULT_PT_UDDR";
	Name = "UDDR";
	Colour = 0xFFADAD_rgb;
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 15;

	Weight = 100;

	HeatConduct = 104;
	Description = "Udder. Makes milk when squeezed (pressure). Requires nutrients to make more milk.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	DefaultProperties.oxygens = 100;
	DefaultProperties.carbons = 100;
	DefaultProperties.co2 = 20;
	DefaultProperties.water = 50;
	DefaultProperties.tmp3 = 100;
	DefaultProperties.tmp4 = 100;
	DefaultProperties.capacity = 800;
	DefaultProperties.tmpcity[3] = 100;
	DefaultProperties.tmpcity[9] = 0;
	DefaultProperties.metabolism = 50;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties
	 * life:  Graphics
	 * oxygens:   Oxygen stored
	 * carbons:  Nutrients stored
	 * tmp2: Highest temperature
	 * tmp3: Type 0 = inside, 1 = skin, 2 = dead
	 */
	Element_FLSH_update(sim, i, x, y, surround_space, nt, parts, pmap);
	//if (parts[i].tmp3 == 1) // Override skin formation
		//parts[i].tmp3 = 0;

	if (parts[i].tmp3 != 2) {
		int rx, ry, r;
		for (rx = -1; rx < 2; ++rx)
		for (ry = -1; ry < 2; ++ry)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				r = pmap[y + ry][x + rx];
				if (!r)
				{
					if (parts[i].carbons > 10 + 50 && parts[i].water > 10 + 50 && sim->rng.chance(1, 500)) {
						int melk = sim->create_part(-1, x + rx, y + ry, PT_MILK);
							parts[melk].carbons += 50;
							parts[melk].water += 50;
							parts[i].carbons -= 50;
							parts[i].water -= 50;
					}
					continue;
				}
			}
	}

	return 0;
}
