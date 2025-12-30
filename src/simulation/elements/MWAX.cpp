#include "simulation/ElementCommon.h"
#include "../ModTools.h"

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);


void Element::Element_MWAX()
{
	Identifier = "DEFAULT_PT_MWAX";
	Name = "MWAX";
	Colour = 0xE0E0AA_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000001f* CFDS;
	Falldown = 2;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 2;

	Weight = 25;

	DefaultProperties.temp = R_TEMP + 28.0f + 273.15f;
	HeatConduct = 44;
	Description = "Liquid Wax. Hardens into WAX at 45 degrees.";

	Properties = TYPE_LIQUID;

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
	//MWAX is a low carbon powder, it should not have any more than 7 carbons. but its frozen
	//if (parts[i].carbons > 7)sim->part_change_type(i, x, y, PT_DESL);


	if (parts[i].capacity == 0 && parts[i].tmp4 == 0)
	{
		parts[i].tmp4 = 100;
		parts[i].capacity = 400;

		parts[i].life = RNG::Ref().between(15, 19);
		parts[i].tmp = makeAlk(sim->parts[i].life);
		if (parts[i].tmp < 2 * parts[i].life + 2)parts[i].tmp2 = getBondLoc(parts[i].life);
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





	int t = parts[i].temp - sim->pv[y / CELL][x / CELL] / 2;	//Pressure affects state transitions
	//Freezing into WAX
	if ((parts[i].carbons < 5 && t <= (-200 + 273.15)) || (parts[i].carbons > 5 && t <= (14.3f * sqrt((parts[i].carbons - 12))) + 273.15))
		sim->part_change_type(i, x, y, PT_WAX);
	//Boiling into GAS
	if ((parts[i].carbons == 1 && t > (-180 + 273.15)) || (parts[i].carbons == 2 && t > (-100 + 273.15)) || (parts[i].carbons == 3 && t > -50 + 273.15) || (parts[i].carbons >= 4 && t > (4 * sqrt(500 * (parts[i].carbons - 4))) + 273.15))
		sim->part_change_type(i, x, y, PT_GAS);

	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	//Spawns with carbons (5-7)
	sim->parts[i].carbons = RNG::Ref().between(5, 7);
	sim->parts[i].co2 = makeAlk(sim->parts[i].carbons);
	if (sim->parts[i].co2 < 2 * sim->parts[i].carbons + 2)sim->parts[i].tmp3 = getBondLoc(sim->parts[i].carbons);
}


