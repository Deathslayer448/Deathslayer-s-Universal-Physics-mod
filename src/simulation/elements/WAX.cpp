#include "simulation/ElementCommon.h"
#include "../ModTools.h"

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_WAX()
{
	Identifier = "DEFAULT_PT_WAX";
	Name = "WAX";
	Colour = 0xF0F0BB_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 10;

	Weight = 100;

	HeatConduct = 44;
	Description = "Flammable, melts at moderately high temperatures.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Create = &create;

}

	static int update(UPDATE_FUNC_ARGS) {
		//WAX is a low to medium carbon solid, it should not have any more than 19 carbons.
//		if (parts[i].carbons > 19)sim->part_change_type(i, x, y, PT_PRFN);



	if (parts[i].capacity == 0 && parts[i].tmp4 == 0)
	{
		parts[i].tmp4 = 100;
		parts[i].capacity = 400;

		parts[i].carbons = RNG::Ref().between(15, 19);
		parts[i].co2 = makeAlk(parts[i].carbons);
		if (parts[i].co2 < 2 * parts[i].carbons + 2)parts[i].tmp3 = getBondLoc(parts[i].carbons);
	}

	if (parts[i].tmp4 <= 0)
	{
		if (parts[i].oxygens > 0 || parts[i].carbons > 0 || parts[i].co2 > 0 || parts[i].water > 0 || parts[i].nitrogens > 0)
		{
				sim->part_change_type(i, x, y, PT_DUST);
				
		}
		else
			sim->kill_part(i);
		return 0;

	}






		int t = parts[i].temp - sim->pv[y / CELL][x / CELL];	//Pressure affects state transitions
		//Melting
		if ((parts[i].carbons <= 4 && t < -230 + parts[i].carbons * 50 + 273.15f) || (parts[i].carbons > 4 && parts[i].carbons < 12 && t >= (16.5f * parts[i].carbons - 200 + 273.15)) || (parts[i].carbons >= 12 && t > (14.3f * sqrt(parts[i].carbons - 12)) + 273.15))
		{
			
			if (parts[i].carbons < 8)//Low carbon melting
				sim->part_change_type(i, x, y, PT_MWAX);
			else //Medium carbon melting
				sim->part_change_type(i, x, y, PT_DESL);
		}

		return 0;
	}

	static void create(ELEMENT_CREATE_FUNC_ARGS) {
		//Spawns with carbons (15-19)
		sim->parts[i].carbons = RNG::Ref().between(15, 19);
		sim->parts[i].co2 = makeAlk(sim->parts[i].carbons);
		if (sim->parts[i].co2 < 2 * sim->parts[i].carbons + 2)sim->parts[i].tmp3 = getBondLoc(sim->parts[i].carbons);
	}
