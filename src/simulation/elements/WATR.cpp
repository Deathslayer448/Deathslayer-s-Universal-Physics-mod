#include "simulation/ElementCommon.h"

int Element_WATR_update(UPDATE_FUNC_ARGS);

void Element::Element_WATR()
{
	Identifier = "DEFAULT_PT_WATR";
	Name = "WATR";
	Colour = 0x2030D0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.1f;
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
	HeatConduct = 29;
	Description = "Water. Conducts electricity, freezes, essencial for life.";

	Properties = TYPE_LIQUID | PROP_CONDUCTS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.15f;
	HighTemperatureTransition = PT_WTRV;

	Update = &Element_WATR_update;
}

int Element_WATR_update(UPDATE_FUNC_ARGS)
{
	auto &elements = sim->elements();
	
	// Initialize water content if needed
	if (parts[i].tmpcity[7] == 0 && (parts[i].type == PT_WATR || parts[i].type == PT_WTRV || parts[i].type == PT_DSTW || parts[i].type == PT_SLTW || parts[i].type == PT_CBNW || parts[i].type == PT_H2O2))
	{
		if(parts[i].water > 0)
			parts[i].tmpcity[7] = 400;
		else
		{
			switch (parts[i].type)
			{
			case PT_DSTW:
			case PT_WTRV:
			case PT_WATR:
				parts[i].water = 100;
				parts[i].tmpcity[7] = 400;
				break;

			case PT_CBNW:
				parts[i].water = 10;
				parts[i].tmp4 = 100;
				parts[i].ctype = PT_CO2;
				parts[i].tmpcity[7] = 400;
				break;
				
			case PT_SLTW:
				parts[i].water = 100;
				parts[i].tmp4 = 100;
				parts[i].ctype = PT_SALT;
				parts[i].tmpcity[7] = 400;
				break;
				
			case PT_H2O2:
				parts[i].water = 80;
				parts[i].oxygens = 100;
				parts[i].tmpcity[7] = 400;
				break;

			default:
				parts[i].water = 100;
				parts[i].tmpcity[7] = 400;
				break;
			}
		}
	}

	// Handle water content transitions
	if (parts[i].tmp4 <= 0 && parts[i].water > 0 && parts[i].type != PT_WATR && parts[i].type != PT_H2O2)
		sim->part_change_type(i, x, y, PT_WATR);
	else if (parts[i].tmp4 <= 0 && parts[i].water <= 0 && parts[i].type != PT_H2O2)
		sim->part_change_type(i, x, y, PT_DUST);
	if (parts[i].ctype == parts[i].type)
		parts[i].ctype = 0;
	if (parts[i].ctype != 0 && parts[i].tmp4 <= 0)
		parts[i].ctype = 0;
	if (parts[i].ctype == 0 && parts[i].tmp4 > 0 && parts[i].type == PT_WATR)
		parts[i].tmp4 = 0;

	// Freezing - dynamic temperature transition based on ctype
	if ((elements[parts[i].ctype].LowTemperature > 0 && parts[i].temp - (sim->pv[y / CELL][x / CELL] / 2.0f) < elements[parts[i].ctype].LowTemperature && sim->rng.chance(1, restrict_flt(parts[i].temp - sim->pv[y / CELL][x / CELL], 1.0f, elements[parts[i].ctype].LowTemperature))) ||
	 (elements[parts[i].ctype].LowTemperature <= 0 && parts[i].temp - (sim->pv[y / CELL][x / CELL] / 2.0f) < 273.15f && sim->rng.chance(1, restrict_flt(parts[i].temp - sim->pv[y / CELL][x / CELL], 1.0f, 273.15f)))) 
	{
		if(elements[parts[i].ctype].LowTemperatureTransition > 0)
			sim->part_change_type(i, x, y, elements[parts[i].ctype].LowTemperatureTransition);
		else
			sim->part_change_type(i, x, y, PT_ICEI);
		return 0;
	}

	// Boiling - dynamic temperature transition based on ctype
	if ((elements[parts[i].ctype].HighTemperature > 0 && parts[i].temp - (sim->pv[y / CELL][x / CELL] / 2.0f) > elements[parts[i].ctype].HighTemperature && sim->rng.chance(restrict_flt(parts[i].temp - sim->pv[y / CELL][x / CELL], 1.0f, elements[parts[i].ctype].HighTemperature), elements[parts[i].ctype].HighTemperature)) ||
	(elements[parts[i].ctype].HighTemperature <= 0 && parts[i].temp - (sim->pv[y / CELL][x / CELL] / 2.0f) > 373.15f && sim->rng.chance(restrict_flt(parts[i].temp - sim->pv[y / CELL][x / CELL], 1.0f, 373.15f), 373.15f)))
	{
		if(elements[parts[i].ctype].HighTemperatureTransition > 0)
			sim->part_change_type(i, x, y, elements[parts[i].ctype].HighTemperatureTransition);
		else
			sim->part_change_type(i, x, y, PT_WTRV);
		return 0;
	}

	// Standard interactions
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				if (TYP(r)==PT_SALT && sim->rng.chance(1, 50))
				{
					sim->part_change_type(i,x,y,PT_SLTW);
					// on average, convert 3 WATR to SLTW before SALT turns into SLTW
					if (sim->rng.chance(1, 3))
						sim->part_change_type(ID(r),x+rx,y+ry,PT_SLTW);
				}
				else if ((TYP(r)==PT_RBDM||TYP(r)==PT_LRBD) && (sim->legacy_enable||parts[i].temp>(273.15f+12.0f)) && sim->rng.chance(1, 100))
				{
					sim->part_change_type(i,x,y,PT_FIRE);
					parts[i].life = 4;
					parts[i].ctype = PT_WATR;
				}
				else if (TYP(r)==PT_FIRE && parts[ID(r)].ctype!=PT_WATR)
				{
					sim->kill_part(ID(r));
					if (sim->rng.chance(1, 30))
					{
						sim->kill_part(i);
						return 1;
					}
				}
				else if (TYP(r)==PT_SLTW && sim->rng.chance(1, 2000))
				{
					sim->part_change_type(i,x,y,PT_SLTW);
				}
				else if (TYP(r)==PT_ROCK && fabs(parts[i].vx)+fabs(parts[i].vy) >= 0.5 && sim->rng.chance(1, 1000)) // ROCK erosion
				{
					if (sim->rng.chance(1,3))
						sim->part_change_type(ID(r),x+rx,y+ry,PT_SAND);
					else
						sim->part_change_type(ID(r),x+rx,y+ry,PT_STNE);
				}
			}
		}
	}
	return 0;
}
