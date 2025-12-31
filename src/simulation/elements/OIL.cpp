#include "simulation/ElementCommon.h"
#include "simulation/ModTools.h"

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_OIL()
{
	Identifier = "DEFAULT_PT_OIL";
	Name = "OIL";
	Colour = 0x404010_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.1f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 30;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 20;

	HeatConduct = 42;
	Description = "Flammable, turns into GAS at low pressure or high temperature.";

	Properties = TYPE_LIQUID | PROP_NEUTPASS;

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
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmpcity[7] == 0 && parts[i].tmp4 == 0)
	{
		parts[i].tmp4 = 100;
		parts[i].tmpcity[7] = 400;

		// Spawns with carbons (20-60)
		parts[i].carbons = sim->rng.between(20, 60);
		int alkType = sim->rng.between(1, 3);
		parts[i].hydrogens = (alkType == 1) ? (2 * parts[i].carbons + 2) : (alkType == 2) ? (2 * parts[i].carbons) : (2 * parts[i].carbons - 2);
		if (parts[i].hydrogens < 2 * parts[i].carbons + 2)
			parts[i].tmp3 = sim->rng.between(parts[i].carbons / 2, parts[i].carbons / 2 + 1);
		parts[i].life = parts[i].carbons + parts[i].hydrogens;
	}

	// OIL is a high carbon liquid, it should not have any less than 20 carbons.
	if (parts[i].carbons < 20 && sim->rng.chance(1, 1000))
		sim->part_change_type(i, x, y, PT_DESL);

	int t = parts[i].temp - sim->pv[y / CELL][x / CELL];	//Pressure affects state transitions
	//Boiling into GAS
	if (t > (4.0f * sqrt(500.0f * (parts[i].carbons - 4))) + 273.15f && (sim->rng.chance(1, restrict_flt(100.0f - (sim->pv[y / CELL][x / CELL] + parts[i].temp) * surround_space / 10.0f, 1.0f, MAX_TEMP)) || sim->rng.chance(1, restrict_flt(2000.0f - surround_space * 10.0f, 1.0f, MAX_TEMP))))
		sim->part_change_type(i, x, y, PT_GAS);

	// Better burning by Silent
	if (sim->betterburning_enable)
	{
		int mintemp = (sim->elements()[parts[i].type].Flammable * 10 + 273.15f) - sim->pv[y / CELL][x / CELL];

		for (int rx = -2; rx < 3; rx++)
			for (int ry = -2; ry < 3; ry++)
				if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					int r = pmap[y + ry][x + rx];
					if (!r)
						continue;
					int rt = TYP(r);
					if ((rt == PT_FIRE || rt == PT_PLSM) && parts[i].temp > mintemp && sim->rng.chance(1, restrict_flt(parts[i].temp - mintemp, 1.0f, MAX_TEMP)))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 4;
						parts[i].temp += 200.0f;
						return 1;
					}
				}
	}

	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	// Spawns with carbons (20-60)
	sim->parts[i].carbons = sim->rng.between(20, 60);
	int alkType = sim->rng.between(1, 3);
	sim->parts[i].hydrogens = (alkType == 1) ? (2 * sim->parts[i].carbons + 2) : (alkType == 2) ? (2 * sim->parts[i].carbons) : (2 * sim->parts[i].carbons - 2);
	if (sim->parts[i].hydrogens < 2 * sim->parts[i].carbons + 2)
		sim->parts[i].tmp3 = sim->rng.between(sim->parts[i].carbons / 2, sim->parts[i].carbons / 2 + 1);
	sim->parts[i].life = sim->parts[i].carbons + sim->parts[i].hydrogens;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	return 0;
}
