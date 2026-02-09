#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
int Element_WATR_update(UPDATE_FUNC_ARGS);

void Element::Element_SWTR() {
	Identifier = "DEFAULT_PT_SWTR";
	Name = "SWTR";
	Colour = PIXPACK(0x5362F5);
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
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

	Mass = 35/1000.0f;

	HeatConduct = 35;
	Description = "Sugar water.";


	Properties = TYPE_LIQUID | PROP_CONDUCTS | PROP_EDIBLE | PROP_WATER;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.15f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {








	Element_WATR_update(sim, i, x, y, surround_space, nt, parts, pmap);


	// if (parts[i].temp + sim->pv[y / CELL][x / CELL] > 383.15f)
	// {
		
	// 	parts[sim->create_part(-3, x, y, PT_SUGR)].tmp4 = parts[i].tmp4;
	// 	parts[i].tmp4 = 0;
	// 	parts[i].ctype = parts[i].type;
	// 	sim->part_change_type(i, x, y, PT_WTRV);
	// 	return 1;
	// }
	// if (parts[i].temp - sim->pv[y / CELL][x / CELL] < 252.05f)
	// {

	// 	parts[i].ctype = parts[i].type;
	// 	sim->part_change_type(i, x, y, PT_ICEI);
	// 	return 1;
	// }


	int r, rx, ry;
	for (rx=-1; rx<2; rx++)
		for (ry=-1; ry<2; ry++)
			if (BOUNDS_CHECK && (rx || ry)) {
				r = pmap[y+ry][x+rx];
				switch TYP(r) {
				case PT_SUGR: // Grow sugar crystals
					if (RNG::Ref().chance(1, 200 + pow(parts[i].water, 2)))
						sim->part_change_type(i, parts[i].x, parts[i].y, PT_SUGR);
					break;
				case PT_PLNT:
					if (RNG::Ref().chance(1, 2000))
						sim->part_change_type(i, parts[i].x, parts[i].y, PT_PLNT);
					break;
				case PT_RBDM:
				case PT_LRBD:
					if ((sim->legacy_enable || parts[i].temp>(273.15f+12.0f)) && RNG::Ref().chance(1, 100)) {
						sim->part_change_type(i,x,y,PT_FIRE);
						parts[i].life = 4;
						parts[i].ctype = PT_WATR;
					}
					break;
				case PT_FIRE:
					if (parts[ID(r)].ctype != PT_WATR) {
						sim->kill_part(ID(r));
						if (RNG::Ref().chance(1, 30)) {
							sim->kill_part(i);
							return 1;
						}
					}
					break;
				case PT_YEST:
					if (RNG::Ref().chance(1, 3000)) {
						sim->part_change_type(i, parts[i].x, parts[i].y, PT_YEST);
						return 0;
					}
				case PT_NONE:
					break;
				default:
					continue;
				}
			}
	return 0;
}
