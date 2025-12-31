#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);


void Element::Element_MILK() {
	Identifier = "DEFAULT_PT_MILK";
	Name = "MILK";
	Colour = 0xFFFFFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
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

	Weight = 31;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	HeatConduct = 29;
	Description = "Milk. Heals STKM, goes bad over time.";

	Properties = TYPE_LIQUID | PROP_CONDUCTS | PROP_NEUTPASS ;
	//FoodValue = 5;



	DefaultProperties.oxygens = 10;
	DefaultProperties.carbons = 100;
	DefaultProperties.co2 = 20;
	DefaultProperties.tmp4 = 100;
	DefaultProperties.water = 80;
	DefaultProperties.capacity = 300;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties
	 * - tmp:  Has it been pasturized?// removed cuz it's not in use
	 * - life: How long it's been around (increases)
	 */
	 // Evaporate
	if (parts[i].temp > 373.0f) {
		sim->part_change_type(i, x, y, sim->rng.chance(1, 3) ? PT_SUGR : PT_WTRV);
		return 1;
	}
	if (parts[i].water <= 0)
		sim->part_change_type(i, x, y, PT_SUGR);

	if (parts[i].temp > 63.0f + 273.15f)
		parts[i].life--; // Pasturized!
	if (parts[i].temp > 72.0f + 273.15f)
		parts[i].life += 4; // Pasturized!

	// Cold milk doesn't spoil as fast
	if (parts[i].temp < 5.0f + 273.15f)
	{
	
	if (sim->rng.chance(1, 10))
		parts[i].life++;
	}
	else
		parts[i].life +=2;

	// Spoil
	if (parts[i].life > 18000 && sim->rng.chance(restrict_flt(parts[i].life, 1, 70000), 70000)) {
		//sim->kill_part(i);
		parts[sim->create_part(-3, x, y, PT_BCTR)].ctype = parts[i].ctype ^ (1 << sim->rng.between(0, 32));
		parts[i].water -= 10;
		return 1;
	}

	// Touching dirty stuff spoils it
	int lcapacity = 0;
	int rx, ry, r;
	for (rx = -1; rx < 2; ++rx)
	for (ry = -1; ry < 2; ++ry)
		if (x>=0 && y>=0 && x<XRES && y<YRES) {
			r = pmap[y + ry][x + rx];
			int partnum = 0;
			if (!r) continue;
			int rt = TYP(r);

			// diffusion

			if (rt == PT_BLOD || rt == PT_HCL) {



				if (rt == parts[i].type)
					partnum += 5;
				else
					partnum += 2;

				lcapacity = parts[i].tmp4 + parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].co2 + parts[i].water + parts[i].nitrogens;
				if (sim->rng.chance(1, 8) && lcapacity + 2 < parts[i].capacity)
				{

					// take
					if (parts[i].tmp4 < parts[i].capacity / 2 && parts[ID(r)].tmp4 > 0 && parts[i].tmp4 < parts[ID(r)].tmp4 && (parts[i].tmp4 <= 0 || parts[ID(r)].ctype == parts[i].ctype) && sim->rng.chance(1, 6))
					{
						parts[i].tmp4 += std::min(partnum, parts[ID(r)].tmp4);
						parts[ID(r)].tmp4 -= std::min(partnum, parts[ID(r)].tmp4);
						parts[i].ctype = parts[ID(r)].ctype;
					}
					if (parts[i].oxygens < parts[i].capacity / 2 && parts[ID(r)].oxygens > 0 && parts[i].oxygens < parts[ID(r)].oxygens && sim->rng.chance(1, 6))
					{
						parts[i].oxygens += std::min(partnum, parts[ID(r)].oxygens);
						parts[ID(r)].oxygens -= std::min(partnum, parts[ID(r)].oxygens);
					}
					if (parts[i].carbons < parts[i].capacity / 2 && parts[ID(r)].carbons > 0 && parts[i].carbons < parts[ID(r)].carbons && sim->rng.chance(1, 6))
					{
						parts[i].carbons += std::min(partnum, parts[ID(r)].carbons);
						parts[ID(r)].carbons -= std::min(partnum, parts[ID(r)].carbons);
					}
					if (parts[i].co2 < parts[i].capacity / 2 && parts[ID(r)].co2 > 0 && parts[i].co2 < parts[ID(r)].co2 && sim->rng.chance(1, 6))
					{
						parts[i].co2 += std::min(partnum, parts[ID(r)].co2);
						parts[ID(r)].co2 -= std::min(partnum, parts[ID(r)].co2);
					}
					if (parts[i].co2 < parts[i].capacity / 2 && parts[ID(r)].co2 > 0 && parts[i].co2 < parts[ID(r)].co2 && sim->rng.chance(1, 6))
					{
						parts[i].co2 += std::min(partnum, parts[ID(r)].co2);
						parts[ID(r)].co2 -= std::min(partnum, parts[ID(r)].co2);
					}
					if (parts[i].nitrogens < parts[i].capacity / 2 && parts[ID(r)].nitrogens > 0 && parts[i].nitrogens < parts[ID(r)].nitrogens && sim->rng.chance(1, 6))
					{
						parts[i].nitrogens += std::min(partnum, parts[ID(r)].nitrogens);
						parts[ID(r)].nitrogens -= std::min(partnum, parts[ID(r)].nitrogens);

					}
					if (parts[i].water < parts[i].capacity / 2 && parts[ID(r)].water > 0 && parts[i].water < parts[ID(r)].water && sim->rng.chance(1, 6))
					{
						parts[i].water += std::min(partnum, parts[ID(r)].water);
						parts[ID(r)].water -= std::min(partnum, parts[ID(r)].water);

					}

				}
				//give
				lcapacity = parts[ID(r)].tmp4 + parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].co2 + parts[ID(r)].co2 + parts[ID(r)].water + parts[ID(r)].nitrogens;
				if (sim->rng.chance(1, 8) && lcapacity + 2	 < parts[ID(r)].capacity)
				{

					{
						if (parts[ID(r)].tmp4 < parts[ID(r)].capacity / 2 && parts[i].tmp4 > parts[ID(r)].tmp4 && (parts[ID(r)].tmp4 == 0 || parts[i].ctype == parts[ID(r)].ctype) && sim->rng.chance(1, 6))
						{
							parts[ID(r)].tmp4 += std::min(partnum, parts[i].tmp4);
							parts[i].tmp4 -= std::min(partnum, parts[i].tmp4);
							parts[ID(r)].ctype = parts[i].ctype;
						}
						//((parts[ID(r)].tmp4 == 0 && parts[ID(r)].ctype == 0) || parts[ID(r)].ctype == parts[i].ctype)
						if (parts[ID(r)].oxygens < parts[ID(r)].capacity / 2 && parts[i].oxygens > 0 && parts[ID(r)].oxygens < parts[i].oxygens && sim->rng.chance(1, 6))
						{
							parts[ID(r)].oxygens += std::min(partnum, parts[i].oxygens);
							parts[i].oxygens -= std::min(partnum, parts[i].oxygens);
						}
						if (parts[ID(r)].carbons < parts[ID(r)].capacity / 2 && parts[i].carbons > 0 && parts[ID(r)].carbons < parts[i].carbons && sim->rng.chance(1, 6))
						{
							parts[ID(r)].carbons += std::min(partnum, parts[i].carbons);
							parts[i].carbons -= std::min(partnum, parts[i].carbons);
						}
						if (parts[ID(r)].co2 < parts[ID(r)].capacity / 2 && parts[i].co2 > 0 && parts[ID(r)].co2 < parts[i].co2 && sim->rng.chance(1, 6))
						{
							parts[ID(r)].co2 += std::min(partnum, parts[i].co2);
							parts[i].co2 -= std::min(partnum, parts[i].co2);
						}
						if (parts[ID(r)].co2 < parts[ID(r)].capacity / 2 && parts[i].co2 > 0 && parts[ID(r)].co2 < parts[i].co2 && sim->rng.chance(1, 6))
						{
							parts[ID(r)].co2 += std::min(partnum, parts[i].co2);
							parts[i].co2 -= std::min(partnum, parts[i].co2);
						}
						if (parts[ID(r)].nitrogens < parts[ID(r)].capacity / 2 && parts[i].nitrogens > 0 && parts[ID(r)].nitrogens < parts[i].nitrogens && sim->rng.chance(1, 6))
						{
							parts[ID(r)].nitrogens += std::min(partnum, parts[i].nitrogens);
							parts[i].nitrogens -= std::min(partnum, parts[i].nitrogens);

						}
						if (parts[ID(r)].water < parts[ID(r)].capacity / 2 && parts[i].water > 0 && parts[ID(r)].water < parts[i].water && sim->rng.chance(1, 6))
						{
							parts[ID(r)].water += std::min(partnum, parts[i].water);
							parts[i].water -= std::min(partnum, parts[i].water);

						}
					}


				}
			}
			//color diffusion
			if (rt == PT_MILK && sim->rng.chance(1, 80) && (parts[i].tmpville[5] != 0 || parts[i].tmpville[6] != 0 || parts[i].tmpville[7] != 0))
			{
				if (parts[i].tmpville[5] != 0)
				{
					parts[ID(r)].tmpville[5] += restrict_flt(parts[i].tmpville[5], -5, 5);
					parts[i].tmpville[5] -= restrict_flt(parts[i].tmpville[5], -5, 5);
				}
				if (parts[i].tmpville[6] != 0)
				{
					parts[ID(r)].tmpville[6] += restrict_flt(parts[i].tmpville[6], -5, 5);
					parts[i].tmpville[6] -= restrict_flt(parts[i].tmpville[6], -5, 5);
				}

				if (parts[i].tmpville[7] != 0)
				{
					parts[ID(r)].tmpville[7] += restrict_flt(parts[i].tmpville[7], -5, 5);
					parts[i].tmpville[7] -= restrict_flt(parts[i].tmpville[7], -5, 5);

				}

			}
			if (sim->rng.chance(1, 320))
			{
				int randconst = sim->rng.between(40, 80);
				if (parts[ID(r)].tmpville[5] < 150 )
					parts[ID(r)].tmpville[5] += randconst;
				if (parts[ID(r)].tmpville[6] < 150 )
					parts[ID(r)].tmpville[6] += randconst;
				if (parts[ID(r)].tmpville[7] < 150)
					parts[ID(r)].tmpville[7] += randconst;

			}



			if (rt == PT_BCTR && sim->rng.chance(1, 80))
				parts[i].life += 10;
		}


	return 0;
}


static int graphics(GRAPHICS_FUNC_ARGS) {
	*pixel_mode |= PMODE_BLUR;
	//if (cpart->tmp2 >= COLOR_FRAMES)
	//	return 0;
	// Ease colors from water (2030D0) to F05000 (red)
//	float ease = 1;//cpart->tmp2 * 1.0f / COLOR_FRAMES;
	*colr += cpart->tmpville[5];
	*colg += cpart->tmpville[6];
	*colb += cpart->tmpville[7];

	return 0;
}