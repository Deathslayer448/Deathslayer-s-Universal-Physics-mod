#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_SALT()
{
	Identifier = "DEFAULT_PT_SALT";
	Name = "SALT";
	Colour = 0xFFFFFF_rgb;
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
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 5;
	Hardness = 1;

	Weight = 75;

	HeatConduct = 110;
	Description = "Salt, dissolves in water.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1173.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
}


static int update(UPDATE_FUNC_ARGS) {

	/**
 * Properties
 * tmp4:  amount of matter
  water: water 

 */
	


	if (parts[i].capacity == 0 && parts[i].tmp4 == 0)
	{
		parts[i].tmp4 = 100;
		parts[i].capacity = 400;
	}

	if (parts[i].tmp4 <= 0)
	{
		if (parts[i].oxygens > 0 || parts[i].carbons > 0 || parts[i].co2 > 0 || parts[i].co2 > 0 || parts[i].water > 0 || parts[i].nitrogens > 0)
		{
			if (parts[i].water > 0)
				sim->part_change_type(i, x, y, PT_WATR);
			else
				sim->part_change_type(i, x, y, PT_DUST);
				
		}
		else
			sim->kill_part(i);
		return 0;

	}


	if(parts[i].water > 5)
	{
	parts[i].ctype = PT_SALT;
	sim->part_change_type(i, x, y, PT_SLTW);
	}
	


	int r, rx, ry;



	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry)) {
				r = pmap[y+ry][x+rx];
				int lcapacity = 0;
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
				// Dissolve
				// if (sim->elements[rt].Properties & PROP_WATER) {
				// 	//sim->part_change_type(i, x, y, PT_SWTR);
				// 	//	sim->kill_part(ID(r));
				// 	return 0;
				// }

					if (sim->elements[rt].Properties & PROP_WATER || rt == parts[i].type) {



						if (rt == parts[i].type)
							partnum += 1;
						else
							partnum += 2;

						lcapacity = parts[i].tmp4 + parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].co2 + parts[i].water + parts[i].nitrogens;
						if (RNG::Ref().chance(1, 8) && lcapacity + partnum < parts[i].capacity)
						{

							// take

							if (parts[i].water < parts[i].capacity / 2 && parts[ID(r)].water > 0 && parts[i].water < parts[ID(r)].water && RNG::Ref().chance(1, 6))
							{
								parts[i].water += std::min(partnum, parts[ID(r)].water);
								parts[ID(r)].water -= std::min(partnum, parts[ID(r)].water);

							}
						}
						lcapacity = parts[ID(r)].tmp4 + parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].co2 + parts[ID(r)].water + parts[ID(r)].nitrogens;
						if (RNG::Ref().chance(1, 8) && lcapacity + partnum < parts[ID(r)].capacity && rt == parts[i].type)
						{

							if (parts[ID(r)].water < parts[ID(r)].capacity / 2 && parts[i].water > 0 && parts[ID(r)].water < parts[i].water && RNG::Ref().chance(1, 6))
							{
								parts[ID(r)].water += std::min(partnum, parts[i].water);
								parts[i].water -= std::min(partnum, parts[i].water);

							}

						}
					}
			}
	return 0;
}
