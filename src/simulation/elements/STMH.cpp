#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
int Element_FLSH_update(UPDATE_FUNC_ARGS);

void Element::Element_STMH() {
	Identifier = "DEFAULT_PT_STMH";
	Name = "STMH";
	Colour = 0xc4AE95_rgb;
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
	Hardness = 0;

	Weight = 100;

	HeatConduct = 104;
	Description = "Stomach. Releases HCl and absorbs nutrients.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	DefaultProperties.oxygens = 100;
	DefaultProperties.carbons = 100;
//	DefaultProperties.co2 = 100;
	DefaultProperties.water = 50;
	DefaultProperties.tmp3 = 100;
	DefaultProperties.tmp4 = 100;
	DefaultProperties.capacity = 800;
	DefaultProperties.tmpcity[9] = 0;
	DefaultProperties.tmpcity[3] = 100;
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
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties
	 * life:  Graphics
	 * oxygens:   Oxygen stored
	 * carbons:  Nutrients stored
	 * tmp2: Highest temperature
	 * tmp3: Type 0 = inside, 1 = skin, 2 = dead
	 * co2: carbon dioxide & waste
	 * tmpcity[5] signal to release hcl/water
	 * tmpville[9] != 0 Deactivates hcl/water production
	 */
	Element_FLSH_update(sim, i, x, y, surround_space, nt, parts, pmap);
	//if (parts[i].tmp3 == 1) // Override skin formation
	//	parts[i].tmp3 = 0;
	if (parts[i].tmp3 != 2) {
	if (surround_space != 0 && parts[i].tmpcity[0] == 0)
		parts[i].tmpcity[0] = 1;
	if (surround_space == 0 && sim->currentTick % 50 == 0 && parts[i].tmpcity[0] != 0)
		parts[i].tmpcity[0] = 0;
	if(sim->currentTick % 500 == 0 && parts[i].tmpcity[5] > 0)
		parts[i].tmpcity[5]--;



	
		int rx, ry, r, rt;
		int lcapacity = 0;
		for (ry = -1; ry < 2; ++ry)
			for (rx = -1; rx < 2; ++rx)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
				r = pmap[y + ry][x + rx];
				if (!r && parts[i].tmpcity[5] > 0 && parts[i].water > 30 && parts[i].oxygens > 30 && sim->rng.chance(1, 8) && parts[i].tmpville[9] == 0)
				{
					if (sim->rng.chance(1, 2))
					{
					
						parts[i].water -= 20;
						parts[i].oxygens -= 20;
						parts[sim->create_part(-1, x + rx, y + ry, PT_HCL)].water += 10;
						parts[i].tmpcity[5]--;
					}
					else
					{
						parts[i].water -= 20;
						parts[sim->create_part(-1, x + rx, y + ry, PT_WATR)].water += 10;
						parts[i].tmpcity[5]--;
					}
					
				}
				if (!r) continue;
				rt = TYP(r);
				if(parts[ID(r)].tmpcity[5] && sim->rng.chance(1, 8))
				{
					parts[ID(r)].tmpcity[5]--;
					parts[i].tmpcity[5]++;
				}


	
				if (parts[ID(r)].tmp4 > 0 && parts[i].tmpcity[0] == 1 && parts[i].tmpcity[5] == 0 && sim->rng.chance(1, 50))
					parts[i].tmpcity[5]++;


				//Getting food dissolved in hcl and water
				if (rt == PT_HCL || rt != PT_H2O2)
				{
					lcapacity = parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;
					if (parts[ID(r)].tmp4 > 0 && lcapacity < parts[i].capacity / 1.5 && rt == PT_HCL && !(TYP(r) == PT_FLSH && parts[ID(r)].tmp3 != 2) && sim->rng.chance(1, 8))
					{

					/*if (parts[ID(r)].ctype == PT_SUGR || parts[ID(r)].ctype == PT_SWTR && parts[i].co2 < 290)
						{
							parts[i].carbons += std::min(15, parts[ID(r)].tmp4);
							parts[i].co2 += std::min(5, parts[ID(r)].tmp4);;
	
							parts[ID(r)].tmp4 -= std::min(20, parts[ID(r)].tmp4);;
						}
						else
						{*/
						//parts[i].carbons += std::min(10, parts[ID(r)].tmp4);
						//parts[ID(r)].tmp4 -= std::min(10, parts[ID(r)].tmp4);
						if (parts[ID(r)].tmp4 > 0 && parts[i].carbons < parts[i].capacity / 2 && sim->rng.chance(1, 6))
						{
							parts[i].carbons += std::min(20, parts[ID(r)].tmp4);
							parts[ID(r)].tmp4 -= std::min(20, parts[ID(r)].tmp4);
						}
						if (parts[ID(r)].carbons > 0 && parts[i].carbons < parts[i].capacity / 2 && sim->rng.chance(1, 6))
						{
							parts[i].carbons += std::min(20, parts[ID(r)].carbons);
							parts[ID(r)].carbons -= std::min(20, parts[ID(r)].carbons);
						}
						if (parts[ID(r)].oxygens > 0 && parts[i].oxygens < parts[i].capacity / 2 && sim->rng.chance(1, 6))
						{
							parts[i].oxygens += std::min(20, parts[ID(r)].oxygens);
							parts[ID(r)].oxygens -= std::min(20, parts[ID(r)].oxygens);
						}
							if (parts[ID(r)].co2 > 0 && parts[i].co2 < parts[i].capacity / 3 && sim->rng.chance(1, 6))
						{
							parts[i].co2 += std::min(20, parts[ID(r)].co2);
							parts[ID(r)].co2 -= std::min(20, parts[ID(r)].co2);
						}
						if (parts[ID(r)].co2 > 0 && parts[i].co2 < parts[i].capacity / 3 && sim->rng.chance(1, 6))
						{
							parts[i].co2 += std::min(20, parts[ID(r)].co2);
							parts[ID(r)].co2 -= std::min(20, parts[ID(r)].co2);
						}
						if (parts[ID(r)].nitrogens > 0 && parts[i].nitrogens < parts[i].capacity / 3 && sim->rng.chance(1, 6))
						{
							parts[i].nitrogens += std::min(20, parts[ID(r)].nitrogens);
							parts[ID(r)].nitrogens -= std::min(20, parts[ID(r)].nitrogens);
						}
						if (parts[ID(r)].water > 0 && parts[i].water < parts[i].capacity / 4 && sim->rng.chance(1, 6))
						{
							parts[i].water += std::min(10, parts[ID(r)].water);
							parts[ID(r)].water -= std::min(10, parts[ID(r)].water);
						}
						if (parts[ID(r)].tmp4 <= 0 && parts[ID(r)].co2 <= 0 && parts[ID(r)].co2 <= 0 && parts[ID(r)].oxygens <= 0 && parts[ID(r)].carbons <= 0 && parts[ID(r)].water <= 0 && parts[ID(r)].nitrogens <= 0 && sim->rng.chance(1, 10))
							sim->kill_part(ID(r));

						}
					if(parts[ID(r)].tmp4 <= 0 && parts[ID(r)].co2 <= 0 && parts[ID(r)].oxygens <= 0 && parts[ID(r)].co2 <= 0 && parts[ID(r)].carbons <= 0 && parts[ID(r)].water <= 0 && parts[ID(r)].nitrogens <= 0 && rt == PT_HCL && sim->rng.chance(restrict_flt(parts[ID(r)].tmpcity[2],0 , 99999), 100000))
						sim->kill_part(ID(r));

						
					
					//}

				}
			}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	// Redden if oxygenated
	int red = std::min(20, cpart->oxygens / 10);
	*colr += cpart->tmpcity[5] * 50;
	*colg -= red;
	*colb -= cpart->tmpcity[0] * 255;

	// Cooking
	// Well done (Around 70 - 80 C)
	if (cpart->carbons > 273.15f + 40.0f) {
		float percent_fade = std::min(cpart->tmp2 - 273.15f, 80.0f) / 80.0f;
		percent_fade += ((abs(nx - ny) * (nx + ny) + nx) % 5) / 10.0f; // Noise

		*colr -= (*colr - 176) * percent_fade;
		*colg -= (*colg - 131) * percent_fade;
		*colb -= (*colb - 90) * percent_fade;

		// Grill lines
		if ((nx + ny) % 30 < 3) {
			*colr *= 0.9f, *colg *= 0.9f, *colb *= 0.9f;
		}
	}
	// Burnt (Above 110 C)
	if (cpart->tmp2 > 273.15f + 110.0f) {
		float m = 1.0f - std::min(cpart->tmp2 - 273.15f + 90.0f, 200.0f) / 200.0f;
		m = 0.2 + 0.8 * m; // Prevent 100% black
		*colr *= m, *colg *= m, *colb *= m;
	}
	// Blue when cold
	if (cpart->temp < 273 && cpart->tmp2 < 273.15f + 110.0f) {
		*colr -= (int)restrict_flt((273.15f-cpart->temp)/5,0,80);
		*colg += (int)restrict_flt((273.15f-cpart->temp)/4,0,40);
		*colb += (int)restrict_flt((273.15f-cpart->temp)/1.5,0,100);
	}

	return 0;
}
