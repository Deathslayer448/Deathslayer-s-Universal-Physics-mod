#include "simulation/ElementCommon.h"
#include "simulation/ModTools.h"

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_GAS()
{
	Identifier = "DEFAULT_PT_GAS";
	Name = "GAS";
	Colour = 0xE0FF20_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 3.0f;
	HotAir = 0.001f	* CFDS;
	Falldown = 0;

	Flammable = 60;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 2.0f + 273.15f;
	HeatConduct = 42;
	Description = "Diffuses quickly and is flammable. Liquefies into OIL under pressure.";

	Properties = TYPE_GAS | PROP_NEUTPASS;

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
	//carbons=num carbons
	//tmp=num tmp3
	//tmp3=location of double/triple bond if alkene/alkyne (tmp<2*carbons+2)
	
	//Cyens toy
	//Condensation
	if (parts[i].carbons == 0)
	{
		//Spawns with carbons (1-4)
		parts[i].carbons = sim->rng.between(1, 4);
		if (parts[i].carbons == 1) { //Creation of methane, can only be CH4 as a pure hydrocarbon
			parts[i].hydrogens = 4;
			parts[i].tmp3 = 0;
		}
		else { //Creating any other type of hydrocarbon
			int alkType = sim->rng.between(1, 3);
			parts[i].hydrogens = (alkType == 1) ? (2 * parts[i].carbons + 2) : (alkType == 2) ? (2 * parts[i].carbons) : (2 * parts[i].carbons - 2);
			if (parts[i].hydrogens < 2 * parts[i].carbons + 2)
				parts[i].tmp3 = sim->rng.between(parts[i].carbons / 2, parts[i].carbons / 2 + 1);
		}
		parts[i].life = parts[i].carbons + parts[i].hydrogens * 5;
	}
	int t = parts[i].temp - sim->pv[y / CELL][x / CELL];	//Pressure affects state transitions

	if (((parts[i].carbons <= 4 && t < -230.0f + parts[i].carbons * 50.0f + 273.15f) || (parts[i].carbons > 4 && t < (4.0f * sqrt(500.0f * (parts[i].carbons - 4))) + 273.15f)) && sim->rng.chance(1, (int)restrict_flt(100.0f - (sim->pv[y / CELL][x / CELL] + parts[i].temp) * surround_space / 10.0f, 1.0f, MAX_TEMP)) && parts[i].tmpcity[3] <= 0)
	{
		if (parts[i].carbons < 8) //Low carbon condensation
			sim->part_change_type(i, x, y, PT_MWAX);
		else if (parts[i].carbons >= 8 && parts[i].carbons < 20) //Medium carbon condensation
			sim->part_change_type(i, x, y, PT_DESL);
		else //High carbon condensation
			sim->part_change_type(i, x, y, PT_OIL);

		parts[i].tmpcity[3] = sim->rng.between(100, 1000);
	}
	
	//Update
	if (!sim->betterburning_enable)
	{
		int r, rx, ry;
		for (rx = -1; rx < 2; rx++)
			for (ry = -1; ry < 2; ry++)
				if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					r = pmap[y + ry][x + rx];
					if (!r)
						continue;
					int rt = TYP(r);
					if (rt == PT_FIRE || rt == PT_PLSM)
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
	//Spawns with carbons (1-4)
	sim->parts[i].carbons = sim->rng.between(1, 4);
	if (sim->parts[i].carbons == 1) { //Creation of methane, can only be CH4 as a pure hydrocarbon
		sim->parts[i].hydrogens = 4;
		sim->parts[i].tmp3 = 0;
	}
	else { //Creating any other type of hydrocarbon
		int alkType = sim->rng.between(1, 3);
		sim->parts[i].hydrogens = (alkType == 1) ? (2 * sim->parts[i].carbons + 2) : (alkType == 2) ? (2 * sim->parts[i].carbons) : (2 * sim->parts[i].carbons - 2);
		if (sim->parts[i].hydrogens < 2 * sim->parts[i].carbons + 2)
			sim->parts[i].tmp3 = sim->rng.between(sim->parts[i].carbons / 2, sim->parts[i].carbons / 2 + 1);
	}
	sim->parts[i].life = sim->parts[i].carbons + sim->parts[i].hydrogens * 5;
}
