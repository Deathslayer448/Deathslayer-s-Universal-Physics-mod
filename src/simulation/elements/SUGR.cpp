#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_SUGR() {
	Identifier = "DEFAULT_PT_SUGR";
	Name = "SUGR";
	Colour = 0xFFF9F2_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 20;
	Explosive = 0;
	Meltable = 2;
	Hardness = 2;
	Description = "Sugar. Great food for bacteria.";

	Properties = TYPE_PART | PROP_NEUTPASS;

	DefaultProperties.tmpcity[7] = 400;
	DefaultProperties.tmp4 = 150;
	DefaultProperties.carbons = 100;
	DefaultProperties.hydrogens = 15;
	DefaultProperties.nitrogens = 5;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 273.15f + 186.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	
	if (parts[i].tmp4 <= 0)
	{
		if (parts[i].oxygens > 0 || parts[i].carbons > 0 || parts[i].hydrogens > 0 || parts[i].water > 0 || parts[i].nitrogens > 0)
		{
			if (parts[i].water > 0)
				sim->part_change_type(i, x, y, PT_WATR);
			else
				sim->part_change_type(i, x, y, PT_DUST);
		}
		else
			sim->kill_part(i);
	}
	if(parts[i].water > 10)
	{
		parts[i].ctype = PT_SUGR;
		// Convert to sugar water - SWTR is a custom element
		// Look it up dynamically
		auto &elements = sim->elements();
		for (int j = 0; j < PT_NUM; j++) {
			if (elements[j].Enabled && elements[j].Identifier == "DEFAULT_PT_SWTR") {
				sim->part_change_type(i, x, y, j);
				break;
			}
		}
	}

	int r, rx, ry;
	r = sim->photons[y][x];
	
	// React with NEUT to make carbon, hydrogen, oxygen
	if (TYP(r) == PT_NEUT && sim->rng.chance(1, 20)) {
		sim->kill_part(ID(r));
		if (sim->rng.chance(1, 3))
			sim->part_change_type(i, x, y, PT_CRBN);
		else
			sim->part_change_type(i, x, y, sim->rng.chance(1, 2) ? PT_O2 : PT_H2);
		return 0;
	}

	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				r = pmap[y+ry][x+rx];
				int capacity = 0;
				int partnum = 0;
				if (!r) continue;
				int rt = TYP(r);
				// Water transfer logic
				bool is_water = (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_WTRV);
				if (is_water || rt == parts[i].type) {
					if (rt == parts[i].type)
						partnum += 2;
					else
						partnum += 1;

					capacity = parts[i].tmp4 + parts[i].oxygens + parts[i].carbons + parts[i].hydrogens + parts[i].water + parts[i].nitrogens;
					if (sim->rng.chance(1, 8) && capacity + partnum < parts[i].tmpcity[7])
					{
						// take water
						if (parts[i].water < parts[i].tmpcity[7] / 2 && parts[ID(r)].water > 0 && parts[i].water < parts[ID(r)].water && sim->rng.chance(1, 6))
						{
							parts[i].water += std::min(partnum, parts[ID(r)].water);
							parts[ID(r)].water -= std::min(partnum, parts[ID(r)].water);
						}
					}
					capacity = parts[ID(r)].tmp4 + parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].hydrogens + parts[ID(r)].water + parts[ID(r)].nitrogens;
					if (sim->rng.chance(1, 8) && capacity + partnum < parts[ID(r)].tmpcity[7] && rt == parts[i].type)
					{
						// give water
						if (parts[ID(r)].water < parts[ID(r)].tmpcity[7] / 2 && parts[i].water > 0 && parts[ID(r)].water < parts[i].water && sim->rng.chance(1, 6))
						{
							parts[ID(r)].water += std::min(partnum, parts[i].water);
							parts[i].water -= std::min(partnum, parts[i].water);
						}
					}
				}
				// Grow YEST
				else if (TYP(r) == PT_YEST && sim->rng.chance(1, 300)) {
					sim->part_change_type(i, parts[i].x, parts[i].y, PT_YEST);
					return 0;
				}
				// React with acid to form carbon
				else if (TYP(r) == PT_ACID || TYP(r) == PT_CAUS) {
					parts[i].temp += 120.0f;
					sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y,
						sim->rng.chance(1, 2) ? PT_CRBN : PT_WTRV);
					if (sim->rng.chance(1, 2))
						sim->kill_part(i);
					return 0;
				}
			} 
	return 0;
}
