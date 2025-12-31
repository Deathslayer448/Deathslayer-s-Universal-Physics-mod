#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_BRKN() {
	Identifier = "DEFAULT_PT_BRKN";
	Name = "BRKN";
	Colour = 0x705060_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.2f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 2;

	Weight = 90;

	HeatConduct = 111;
	Description = "A generic broken solid.";

	Properties = TYPE_PART | PROP_CONDUCTS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	auto &elements = sim->elements();

	if(parts[i].tmp != parts[i].ctype && parts[i].tmp > 0 && parts[i].tmp < PT_NUM)
		parts[i].ctype = parts[i].tmp;
	if (elements[parts[i].ctype].Properties & TYPE_SOLID)
	{	
		if(parts[i].tmpcity[8] == 0)
		{
			sim->part_change_type(i, x, y, parts[i].ctype);
			return 1;
		}
			else if(parts[i].tmpcity[8] == 2)
			{
			sim->part_change_type(i, x, y, PT_LQUD);
			return 1;
			}
		//parts[i].life = 4;
		//parts[i].tmp = parts[i].ctype;
		if (elements[parts[i].ctype].Update)
		 elements[parts[i].ctype].Update(sim, i, x, y, surround_space, nt, parts, pmap);
	//	 return 1;
	
	}
	else
	{
		if(parts[i].tmpcity[8] == 2)
			{
			sim->part_change_type(i, x, y, PT_LQUD);
			return 1;
			}
		parts[i].tmp = parts[i].ctype;

		bool flammable = false;
		bool is_aluminium = parts[i].ctype == PT_ALMN;

		if (parts[i].ctype > 0 && parts[i].ctype < PT_NUM) {
			if (!(elements[parts[i].ctype].Properties & PROP_CONDUCTS))
				parts[i].life = 4; // Prevent spark conducting if not actually conductable
			if (elements[parts[i].ctype].Update)
				elements[parts[i].ctype].Update(sim, i, x, y, surround_space, nt, parts, pmap);
			if (elements[parts[i].ctype].HighTemperatureTransition &&
				parts[i].temp > elements[parts[i].ctype].HighTemperature) {
				sim->part_change_type(i, x, y, elements[parts[i].ctype].HighTemperatureTransition);
					return 1;
			}
			flammable = elements[parts[i].ctype].Flammable > 0;
		}

		for (int rx = -1; rx <= 1; ++rx)
			for (int ry = -1; ry <= 1; ++ry)
				if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
					int r = pmap[y + ry][x + rx];
					if (!r) continue;
					int rt = TYP(r);
					

					if (flammable && (rt == PT_FIRE || rt == PT_PLSM)) {
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].temp += 200.0f;
						return 1;
					}
					else if (parts[i].ctype == PT_SAWD && elements[rt].Properties & TYPE_SOLID) {
						sim->part_change_type(i, x, y, PT_NONE);
						parts[i].life += 10;
						return 1;
					}
					else if (is_aluminium && rt == PT_BRMT && parts[i].temp > 240.0f + 273.15f && sim->rng.chance(1, 100)) {
						sim->part_change_type(ID(r), x + rx, y + ry, PT_THRM);
						sim->part_change_type(i, x, y, PT_THRM);
						return 1;
					}
				}

		if (parts[i].ctype == PT_NONE)
			parts[i].dcolour = 0;
	}
	if(elements[parts[i].ctype].Properties & PROP_LIFE_DEC)
	parts[i].life--;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	auto &elements = SimulationData::CRef().elements;
	if (cpart->ctype > 0 && cpart->ctype < PT_NUM) {
		auto col = elements[cpart->ctype].Colour;
		*colr = col.Red;
		*colg = col.Green;
		*colb = col.Blue;

		if (elements[cpart->ctype].Graphics)
			elements[cpart->ctype].Graphics(gfctx, cpart, nx, ny, pixel_mode, cola,
				colr, colg, colb, firea, firer, fireg, fireb);
	}
	return 0;
}
