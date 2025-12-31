#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_ICEI()
{
	Identifier = "DEFAULT_PT_ICEI";
	Name = "ICE";
	Colour = 0xA0C0FF_rgb;
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
	HotAir = -0.0003f* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 100;

	DefaultProperties.temp = R_TEMP - 50.0f + 273.15f;
	HeatConduct = 46;
	Description = "Crushes under pressure. Cools down air.";

	Properties = TYPE_SOLID|PROP_LIFE_DEC|PROP_NEUTPASS | PROP_WATER;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 0.8f;
	HighPressureTransition = PT_SNOW;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 252.05f;
	HighTemperatureTransition = ST;

	DefaultProperties.ctype = PT_WATR;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &elements = sim->elements();
	int r, rx, ry;
	int ctaype = parts[i].ctype;
	if (parts[i].ctype == PT_BRKN && parts[i].tmp != 0)
		parts[i].ctype = parts[i].tmp;
	if (parts[i].tmp != parts[i].ctype && parts[i].tmp > 0 && parts[i].tmp < PT_NUM)
		parts[i].ctype = parts[i].tmp;
	
	// Handle custom water types
	if(parts[i].ctype == PT_SUGR)
	{
		// Look up SWTR dynamically
		for (int j = 0; j < PT_NUM; j++) {
			if (elements[j].Enabled && elements[j].Identifier == "DEFAULT_PT_SWTR") {
				ctaype = j;
				break;
			}
		}
	}
	else if(parts[i].ctype == PT_SALT)
		ctaype = PT_SLTW;

	if (ctaype != 0 && elements[ctaype].LowTemperature != ITL && elements[ctaype].LowTemperature != ST && parts[i].temp - (sim->pv[y / CELL][x / CELL] / 2.0f) > elements[ctaype].LowTemperature && sim->rng.chance(restrict_flt(parts[i].temp - sim->pv[y / CELL][x / CELL], 1.0f, elements[ctaype].LowTemperature), elements[ctaype].LowTemperature + 100.0f))
	{
		sim->part_change_type(i, x, y, ctaype);
		return 1;
	}

	if (parts[i].ctype == PT_FRZW) //get colder if it is from FRZW
	{
		parts[i].temp = restrict_flt(parts[i].temp - 1.0f, MIN_TEMP, MAX_TEMP);
	}

	for (ry=-1; ry<2; ry++)
		for (rx = -1; rx < 2; rx++)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
			{
				r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r) == PT_SALT || TYP(r) == PT_SLTW)
				{
					if (parts[i].temp - (sim->pv[y / CELL][x / CELL] / 2.0f) > elements[PT_SLTW].LowTemperature && sim->rng.chance(1, 200))
					{
						sim->part_change_type(i, x, y, PT_SLTW);
						sim->part_change_type(ID(r), x+rx, y+ry, PT_SLTW);
						return 0;
					}
				}
				else if ((TYP(r) == PT_FRZZ) && sim->rng.chance(1, 200))
				{
					sim->part_change_type(ID(r), x+rx, y+ry, PT_ICEI);
					parts[ID(r)].ctype = PT_FRZW;
				}
			}
	return 0;
}
