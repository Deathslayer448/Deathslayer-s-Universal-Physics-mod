#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
int Element_WATR_update(UPDATE_FUNC_ARGS);

void Element::Element_DSTW()
{
	Identifier = "DEFAULT_PT_DSTW";
	Name = "DSTW";
	Colour = 0x1020C0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
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

	Weight = 30;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	HeatConduct = 23;
	Description = "Distilled water, does not conduct electricity.";

	Properties = TYPE_LIQUID | PROP_NEUTPASS | PROP_WATER;

	DefaultProperties.water = 100;
	DefaultProperties.tmpcity[7] = 400;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	Element_WATR_update(sim, i, x, y, surround_space, nt, parts, pmap);
	if(parts[i].ctype != 0 || parts[i].hydrogens > 5 || parts[i].oxygens > 5 || parts[i].carbons > 5 || parts[i].tmp4 > 5 || parts[i].nitrogens > 5 || parts[i].water > 5)
		sim->part_change_type(i, x, y, PT_WATR);

	int r, rx, ry;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
			{
				r = pmap[y+ry][x+rx];
				if (!r) continue;
				switch (TYP(r))
				{
				case PT_SALT:
					if (sim->rng.chance(1, 50))
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						// on average, convert 3 DSTW to SLTW before SALT turns into SLTW
						if (sim->rng.chance(1, 3))
							sim->part_change_type(ID(r), x+rx, y+ry, PT_SLTW);
					}
					break;
				case PT_SLTW:
					if (sim->rng.chance(1, 2000))
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						break;
					}
				case PT_WATR:
					if (sim->rng.chance(1, 100))
					{
						sim->part_change_type(i, x, y, PT_WATR);
					}
					break;
				case PT_RBDM:
				case PT_LRBD:
					if ((sim->legacy_enable || parts[i].temp > 12.0f) && sim->rng.chance(1, 100))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 4;
					}
					break;
				case PT_FIRE:
					sim->kill_part(ID(r));
					if (sim->rng.chance(1, 30))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_SMKE: //DSTW + SMKE = BASE
					if (parts[ID(r)].temp > (40.0f + 273.15f) && parts[ID(r)].temp < (60.0f + 273.15f) &&
						parts[i].temp > (40.0f + 273.15f) && parts[i].temp < (60.0f + 273.15f))
					{
						if (sim->rng.chance(1, 100))
						{
							sim->part_change_type(i, x, y, PT_BASE);
							parts[i].life = 1;
							sim->kill_part(ID(r));
						}
					}
					break;
				default:
					continue;
				}
			}
	return 0;
}
