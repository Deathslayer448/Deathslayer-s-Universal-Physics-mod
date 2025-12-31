#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_DUST()
{
	Identifier = "DEFAULT_PT_DUST";
	Name = "DUST";
	Colour = 0xFFE0A0_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 3;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;
	PhotonReflectWavelengths = 0x3FFFFFC0;

	Weight = 85;

	HeatConduct = 20;
	Description = "Very light dust. Flammable, edible for some reason.";

	Properties = TYPE_PART;

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

static int update(UPDATE_FUNC_ARGS) 
{
	auto &elements = sim->elements();
	
	if(parts[i].tmpcity[7] == 0)
		parts[i].tmpcity[7] = 400;
	if(parts[i].water > 5)
		sim->part_change_type(i, x, y, PT_WATR);

	switch(parts[i].ctype)
	{
		case PT_SALT:
			sim->part_change_type(i, x, y, PT_SALT);
			break;
		case PT_SUGR:
			sim->part_change_type(i, x, y, PT_SUGR);
			break;
	}

	int r, rx, ry;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				r = pmap[y+ry][x+rx];
				int capacity = 0;
				int partnum = 0;
				if (!r) 
				{
					if(parts[i].temp > 373.15f && parts[i].water > 0)
					{
						parts[sim->create_part(-1, x + rx, y + ry, PT_WTRV)].water = parts[i].water;
						parts[i].water = 0;
					}
					continue;
				}
				int rt = TYP(r);
				// Water transfer logic
				// Check if it's water-related or same type
				bool is_water = (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_WTRV);
				if (rt == parts[i].type || is_water) {
					if (rt == parts[i].type)
						partnum += 1;
					else
						partnum += 2;

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
			}

	return 0;
}
