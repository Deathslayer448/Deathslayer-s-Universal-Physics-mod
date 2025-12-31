#include "simulation/ElementCommon.h"

int Element_O2_update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_O2()
{
	Identifier = "DEFAULT_PT_O2";
	Name = "OXYG";
	Colour = 0x80A0FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.1f;
	Diffusion = 10.0f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	HeatConduct = 70;
	Description = "Oxygen gas. Needed for most things to burn.";

	Properties = TYPE_GAS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 90.0f;
	LowTemperatureTransition = PT_LO2;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_O2_update;
	Create = &create;
}

int Element_O2_update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmpcity[7] == 0)
	{
		if(parts[i].oxygens == 0)
			parts[i].oxygens = sim->rng.between(50, 100) * 2;
		parts[i].tmpcity[7] = 400;
	}

	if (parts[i].oxygens <= 0)
	{
		sim->kill_part(i);
	}

	if (!sim->betterburning_enable)
	{
		int r, rx, ry;
		for (rx = -2; rx < 3; rx++)
			for (ry = -2; ry < 3; ry++)
				if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
				{
					r = pmap[y + ry][x + rx];
					if (!r)
						continue;
				}
		if (parts[i].temp > 9973.15f && sim->pv[y / CELL][x / CELL] > 250.0f)
		{
			auto gravx = sim->gravOut.forceX[Vec2{ x, y } / CELL];
			auto gravy = sim->gravOut.forceY[Vec2{ x, y } / CELL];
			if (gravx * gravx + gravy * gravy > 400)
			{
				if (sim->rng.chance(1, 5))
				{
					int j;
					sim->create_part(i, x, y, PT_BRMT);

					j = sim->create_part(-3, x, y, PT_NEUT);
					if (j != -1)
						parts[j].temp = MAX_TEMP;
					j = sim->create_part(-3, x, y, PT_PHOT);
					if (j != -1)
					{
						parts[j].temp = MAX_TEMP;
						parts[j].tmp = 0x1;
					}
					rx = x + sim->rng.between(-1, 1);
					ry = y + sim->rng.between(-1, 1);
					r = TYP(pmap[ry][rx]);
					auto &can_move = sim->sd().can_move;
					if (can_move[PT_PLSM][r] || r == PT_O2)
					{
						j = sim->create_part(-3, rx, ry, PT_PLSM);
						if (j > -1)
						{
							parts[j].temp = MAX_TEMP;
							parts[j].tmp |= 4;
						}
					}
					j = sim->create_part(-3, x, y, PT_GRVT);
					if (j != -1)
						parts[j].temp = MAX_TEMP;
					parts[i].temp = MAX_TEMP;
					sim->pv[y / CELL][x / CELL] = MAX_PRESSURE;
				}
			}
		}
	}

	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].oxygens = sim->rng.between(50, 100) * 2;
	sim->parts[i].tmpcity[7] = 400;
}
