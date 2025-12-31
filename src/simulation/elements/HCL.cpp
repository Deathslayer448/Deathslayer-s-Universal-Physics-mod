#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static const int COLOR_FRAMES = 300;

void Element::Element_HCL() {
	Identifier = "DEFAULT_PT_HCL";
	Name = "HCL";
	Colour = 0xF05000_rgb;
	MenuVisible = 1;
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
	Hardness = 0;

	Weight = 31;
	HeatConduct = 74;
	Description = "Hydrochloric acid. Acid used by the stomach.";

	Properties = TYPE_LIQUID;

	DefaultProperties.water = 40;
	DefaultProperties.tmpcity[7] = 500;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 255.15f;
	LowTemperatureTransition = NT;
	HighTemperature = 376.15f;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	auto &elements = sim->elements();
	
	if (parts[i].temp + sim->pv[y / CELL][x / CELL] < elements[parts[i].type].LowTemperature) 
	{
		parts[i].tmpcity[6] = parts[i].ctype;
		parts[i].ctype = parts[i].type;
		sim->part_change_type(i, x, y, PT_ICEI);
		return 1;
	}
	if (parts[i].temp - sim->pv[y / CELL][x / CELL] > elements[parts[i].type].HighTemperature)
	{
		parts[i].tmpcity[6] = parts[i].ctype;
		sim->part_change_type(i, x, y, parts[i].ctype);
		parts[sim->create_part(-3, x, y, parts[i].ctype)].tmp4 = parts[i].tmp4;
		parts[i].tmp4 = 0;
		parts[i].ctype = parts[i].type;
		return 1;
	}
	if (parts[i].tmp4 <= 0 && parts[i].ctype != 0)
		parts[i].ctype = 0;

	for (int ry = -1; ry <= 1; ry++)
		for (int rx = -1; rx <= 1; rx++)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				int r = pmap[y + ry][x + rx];
				int partnum = 0;
				if (!r) continue;
				int rt = TYP(r);
				int rt2 = rt; 
				if (rt2 == PT_BRKN)
					rt2 = parts[ID(r)].ctype;
				if (rt2 == PT_ROCK)
					rt2 = parts[ID(r)].ctype;
				if (rt2 < 0 || rt2 >= PT_NUM)
					rt2 = 0;
				bool rt_is_noble_metl = rt2 == PT_BRAS || rt2 == PT_PTNM || rt2 == PT_GOLD;

				// Stuff diffusion with BLOD, HCL, MILK
				if (rt == PT_BLOD || rt == PT_HCL || rt == PT_MILK) {
					if (rt == parts[i].type)
						partnum += 10;
					else
						partnum += 5;

					int capacity = parts[i].tmp4 + parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;
					if (sim->rng.chance(1, 8) && capacity + 2 < parts[i].tmpcity[7])
					{
						// take
						if (parts[i].tmp4 < parts[i].tmpcity[7] / 2 && parts[ID(r)].tmp4 > 0 && parts[i].tmp4 < parts[ID(r)].tmp4 && (parts[i].tmp4 == 0 || parts[ID(r)].ctype == parts[i].ctype) && sim->rng.chance(1, 60))
						{
							parts[i].tmp4 += std::min(20, parts[ID(r)].tmp4);
							parts[ID(r)].tmp4 -= std::min(20, parts[ID(r)].tmp4);
							parts[i].ctype = parts[ID(r)].ctype;
						}
						if (parts[i].oxygens < parts[i].tmpcity[7] / 2 && parts[ID(r)].oxygens > 0 && parts[i].oxygens < parts[ID(r)].oxygens && sim->rng.chance(1, 6))
						{
							parts[i].oxygens += std::min(partnum, parts[ID(r)].oxygens);
							parts[ID(r)].oxygens -= std::min(partnum, parts[ID(r)].oxygens);
						}
						if (parts[i].carbons < parts[i].tmpcity[7] / 2 && parts[ID(r)].carbons > 0 && parts[i].carbons < parts[ID(r)].carbons && sim->rng.chance(1, 6))
						{
							parts[i].carbons += std::min(partnum, parts[ID(r)].carbons);
							parts[ID(r)].carbons -= std::min(partnum, parts[ID(r)].carbons);
						}
						if (parts[i].co2 < parts[i].tmpcity[7] / 2 && parts[ID(r)].co2 > 0 && parts[i].co2 < parts[ID(r)].co2 && sim->rng.chance(1, 6))
						{
							parts[i].co2 += std::min(partnum, parts[ID(r)].co2);
							parts[ID(r)].co2 -= std::min(partnum, parts[ID(r)].co2);
						}
						if (parts[i].nitrogens < parts[i].tmpcity[7] / 2 && parts[ID(r)].nitrogens > 0 && parts[i].nitrogens < parts[ID(r)].nitrogens && sim->rng.chance(1, 6))
						{
							parts[i].nitrogens += std::min(partnum, parts[ID(r)].nitrogens);
							parts[ID(r)].nitrogens -= std::min(partnum, parts[ID(r)].nitrogens);
						}
						if (parts[i].water < parts[i].tmpcity[7] / 2 && parts[ID(r)].water > 0 && parts[i].water < parts[ID(r)].water && sim->rng.chance(1, 6))
						{
							parts[i].water += std::min(partnum, parts[ID(r)].water);
							parts[ID(r)].water -= std::min(partnum, parts[ID(r)].water);
						}
					}
					// give
					capacity = parts[ID(r)].tmp4 + parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].co2 + parts[ID(r)].water + parts[ID(r)].nitrogens;
					if (sim->rng.chance(1, 8) && capacity + 2 < parts[ID(r)].tmpcity[7])
					{
						if (parts[ID(r)].tmp4 < parts[ID(r)].tmpcity[7] / 2 && parts[i].tmp4 > parts[ID(r)].tmp4 && (parts[ID(r)].tmp4 == 0 || parts[i].ctype == parts[ID(r)].ctype) && sim->rng.chance(1, 6))
						{
							parts[ID(r)].tmp4 += std::min(10, parts[i].tmp4);
							parts[i].tmp4 -= std::min(10, parts[i].tmp4);
							parts[ID(r)].ctype = parts[i].ctype;
						}
						if (parts[ID(r)].oxygens < parts[ID(r)].tmpcity[7] / 2 && parts[i].oxygens > 0 && parts[ID(r)].oxygens < parts[i].oxygens && sim->rng.chance(1, 6))
						{
							parts[ID(r)].oxygens += std::min(partnum, parts[i].oxygens);
							parts[i].oxygens -= std::min(partnum, parts[i].oxygens);
						}
						if (parts[ID(r)].carbons < parts[ID(r)].tmpcity[7] / 2 && parts[i].carbons > 0 && parts[ID(r)].carbons < parts[i].carbons && sim->rng.chance(1, 6))
						{
							parts[ID(r)].carbons += std::min(partnum, parts[i].carbons);
							parts[i].carbons -= std::min(partnum, parts[i].carbons);
						}
						if (parts[ID(r)].co2 < parts[ID(r)].tmpcity[7] / 2 && parts[i].co2 > 0 && parts[ID(r)].co2 < parts[i].co2 && sim->rng.chance(1, 6))
						{
							parts[ID(r)].co2 += std::min(partnum, parts[i].co2);
							parts[i].co2 -= std::min(partnum, parts[i].co2);
						}
						if (parts[ID(r)].nitrogens < parts[ID(r)].tmpcity[7] / 2 && parts[i].nitrogens > 0 && parts[ID(r)].nitrogens < parts[i].nitrogens && sim->rng.chance(1, 6))
						{
							parts[ID(r)].nitrogens += std::min(partnum, parts[i].nitrogens);
							parts[i].nitrogens -= std::min(partnum, parts[i].nitrogens);
						}
						if (parts[ID(r)].water < parts[ID(r)].tmpcity[7] / 2 && parts[i].water > 0 && parts[ID(r)].water < parts[i].water && sim->rng.chance(1, 6))
						{
							parts[ID(r)].water += std::min(partnum, parts[i].water);
							parts[i].water -= std::min(partnum, parts[i].water);
						}
					}
				}
				
				// Dissolve stuff
				bool is_water = (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_WTRV);
				if (is_water && rt != PT_CLNE && rt != PT_PCLN && rt != PT_ACID && rt != PT_HCL && rt != PT_CAUS && sim->rng.chance(1, 8))
				{
					int capacity = parts[i].tmp4 + parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;
					if (sim->rng.chance(1, restrict_flt(elements[rt2].Hardness + parts[i].water - parts[i].temp, 1.0f, MAX_TEMP)) && (parts[i].ctype == rt || parts[i].ctype == 0) && capacity < parts[i].tmpcity[7]) {
						if (sim->parts_avg(i, ID(r), PT_GLAS) != PT_GLAS) {
							if (rt_is_noble_metl)
								parts[i].ctype = rt2;
							if (rt == PT_BRKN && parts[ID(r)].ctype != 0)
								rt = parts[ID(r)].ctype;

							// Dissolve organic/edible stuff
							if (((elements[rt].Properties & TYPE_LIQUID || elements[rt].Properties & TYPE_PART) && rt != PT_STMH) && sim->rng.chance(1, 8))
							{
								if ((parts[ID(r)].co2 > 0 || parts[ID(r)].oxygens > 0 || parts[ID(r)].carbons > 0 || parts[ID(r)].nitrogens > 0 || parts[ID(r)].water > 0))
								{
									parts[i].carbons += std::min(20, parts[ID(r)].carbons);
									parts[i].oxygens += std::min(20, parts[ID(r)].oxygens);
									parts[i].co2 += std::min(20, parts[ID(r)].co2);
									parts[i].nitrogens += std::min(20, parts[ID(r)].nitrogens);
									parts[i].water += std::min(20, parts[ID(r)].water);
									parts[i].tmp4 += std::min(20, parts[ID(r)].tmp4);
									parts[ID(r)].carbons -= std::min(20, parts[ID(r)].carbons);
									parts[ID(r)].oxygens -= std::min(20, parts[ID(r)].oxygens);
									parts[ID(r)].co2 -= std::min(20, parts[ID(r)].co2);
									parts[ID(r)].nitrogens -= std::min(20, parts[ID(r)].nitrogens);
									parts[ID(r)].water -= std::min(20, parts[ID(r)].water);
									parts[ID(r)].tmp4 -= std::min(20, parts[ID(r)].tmp4);
									parts[i].temp++;

									parts[i].ctype = rt;

									if (parts[ID(r)].co2 <= 0 && parts[ID(r)].oxygens <= 0 && parts[ID(r)].carbons <= 0 && parts[ID(r)].tmp4 <= 0 && parts[ID(r)].nitrogens <= 0 && parts[ID(r)].water <= 0)
									{
										parts[i].temp += parts[ID(r)].temp - 273.15f;
										sim->kill_part(ID(r));
									}
								}
								if (parts[ID(r)].tmp4 > 0 && sim->rng.chance(1, 8))
								{
									parts[i].tmp4 += std::min(20, parts[ID(r)].tmp4);
									parts[ID(r)].tmp4 -= std::min(20, parts[ID(r)].tmp4);
									parts[i].ctype = rt;
								}
							}
						}
					}
				}
				// Weight switching
				if (sim->NoWeightSwitching && sim->pmap_count[y][x]<2 && TYP(r) != parts[i].type && sim->rng.chance(1, 8) && ((y > parts[ID(r)].y && sim->rng.chance(1, restrict_flt(elements[parts[i].type].Weight - pow(elements[TYP(r)].Weight, 2) / 10.0f, 1.0f, MAX_TEMP))) || (y < parts[ID(r)].y && sim->rng.chance(1, 100))) && (elements[TYP(r)].Properties & TYPE_PART || elements[TYP(r)].Properties & TYPE_LIQUID)) {
					float temp = parts[i].x;
					parts[i].x = parts[ID(r)].x;
					parts[ID(r)].x = temp;
					temp = parts[i].y;
					parts[i].y = parts[ID(r)].y;
					parts[ID(r)].y = temp;
				}
			}

	parts[i].tmpcity[2]++;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	*pixel_mode |= PMODE_BLUR;
	float ease = 1;
	*colr = 0x20 + (0xF0 - 0x20) * ease;
	*colg = 0x30 + (0x50 - 0x30) * ease;
	*colb = 0xD0 + (0x00 - 0xD0) * ease;

	return 0;
}
