#include "simulation/ElementCommon.h"
#include "simulation/ModTools.h"

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
	Description = "Wax. Melts at moderately high temperatures.";

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
	//if (parts[i].carbons > 19)sim->part_change_type(i, x, y, PT_PRFN);

	int t = parts[i].temp - sim->pv[y / CELL][x / CELL];	//Pressure affects state transitions
	//Melting
	if ((parts[i].carbons <= 4 && t < -230.0f + parts[i].carbons * 50.0f + 273.15f) || (parts[i].carbons > 4 && parts[i].carbons < 12 && t >= (16.5f * parts[i].carbons - 200.0f + 273.15f)) || (parts[i].carbons >= 12 && t > (14.3f * sqrt(parts[i].carbons - 12)) + 273.15f))
	{
		if (parts[i].carbons < 8) //Low carbon melting
			sim->part_change_type(i, x, y, PT_MWAX);
		else //Medium carbon melting
			sim->part_change_type(i, x, y, PT_DESL);
	}

	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	//Spawns with carbons (15-19)
	sim->parts[i].life = sim->rng.between(15, 19);
	int alkType = sim->rng.between(1, 3);
	sim->parts[i].tmp = (alkType == 1) ? (2 * sim->parts[i].life + 2) : (alkType == 2) ? (2 * sim->parts[i].life) : (2 * sim->parts[i].life - 2);
	if (sim->parts[i].tmp < 2 * sim->parts[i].life + 2)
		sim->parts[i].tmp2 = sim->rng.between(sim->parts[i].life / 2, sim->parts[i].life / 2 + 1);
}
