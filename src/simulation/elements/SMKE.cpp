#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_SMKE()
{
	Identifier = "DEFAULT_PT_SMKE";
	Name = "SMKE";
	Colour = 0x222222_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.20f;
	Collision = 0.0f;
	Gravity = -0.1f;
	Diffusion = 0.75f;
	HotAir = 0.010f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 120.0f + 273.15f;
	HeatConduct = 88;
	Description = "Smoke, created by combustion.";

	Properties = TYPE_GAS|PROP_LIFE_DEC;

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
	if (sim->betterburning_enable && parts[i].life <= 0)
	{
		switch (parts[i].ctype)
		{
		case PT_NONE:
			switch (sim->rng.between(2, 20))
			{
			case 7:
				if (sim->rng.chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_WTRV);
					parts[i].ctype = PT_NONE;
					parts[i].temp += sim->rng.between(0, 2);
					parts[i].life += sim->rng.between(5, 10);
				}
				break;
			case 4:
				if (sim->rng.chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_CO2);
					parts[i].ctype = PT_NONE;
					parts[i].temp += sim->rng.between(0, 2);
					parts[i].life += sim->rng.between(5, 10);
				}
				break;
			case 5:
				if (sim->rng.chance(1, 50))
				{
					sim->part_change_type(i, x, y, PT_GAS);
					parts[i].ctype = PT_NONE;
					parts[i].temp += sim->rng.between(0, 2);
					parts[i].life += sim->rng.between(5, 10);
				}
				break;
			case 6:
				if (sim->rng.chance(1, 50))
				{
					sim->part_change_type(i, x, y, PT_BCOL);
					parts[i].ctype = PT_NONE;
					parts[i].temp += sim->rng.between(0, 2);
					parts[i].life += sim->rng.between(5, 50);
				}
				break;
			}
			break;
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = 55;
	*colg = 55;
	*colb = 55;

	*firea = 75;
	*firer = 55;
	*fireg = 55;
	*fireb = 55;

	*pixel_mode = PMODE_NONE; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_BLEND;
	//Returning 1 means static, cache as we please
	return 1;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.between(70, 134);
}
