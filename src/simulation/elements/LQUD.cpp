#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void change_type(ELEMENT_CHANGETYPE_FUNC_ARGS);

void Element::Element_LQUD() {
	Identifier = "DEFAULT_PT_LQUD";
	Name = "LQUD";
	Colour = 0xFFFFFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
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
	Hardness = 0;

	Weight = 10;
	HeatConduct = 34;
	Description = "Generic liquid state.";
	Properties = TYPE_LIQUID;

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
	ChangeType = &change_type;
}

static int update(UPDATE_FUNC_ARGS) {
	auto &elements = sim->elements();
	if(parts[i].tmpcity[8] == 0 && parts[i].ctype > 0 && parts[i].ctype < PT_NUM)
	{
		sim->part_change_type(i, x, y, parts[i].ctype);
		return 1;
	}
	else if(parts[i].tmpcity[8] == 1)
	{
		sim->part_change_type(i, x, y, PT_BRKN);
		return 1;
	}
	if (parts[i].ctype == PT_WSTE && parts[i].water < 10)
	{
		parts[i].tmpcity[8] = 0;
			sim->part_change_type(i, x, y, parts[i].ctype);
			return 1;
	}
	if (parts[i].ctype < 0 || parts[i].ctype > PT_NUM || parts[i].ctype == PT_LQUD)
		parts[i].ctype = 0;

	if (parts[i].ctype) {
		// Pressure transitions override temperature
		if (elements[parts[i].ctype].HighPressureTransition == PT_LQUD &&
			sim->pv[y / CELL][x / CELL] > elements[parts[i].ctype].HighPressure &&
			elements[parts[i].ctype].HighPressure != IPH) {}
		else if (elements[parts[i].ctype].LowPressureTransition == PT_LQUD &&
			sim->pv[y / CELL][x / CELL] > elements[parts[i].ctype].LowPressure &&
			elements[parts[i].ctype].LowPressure != IPL) {}

		else if (parts[i].temp > elements[parts[i].ctype].BoilingPoint &&
				elements[parts[i].ctype].BoilingPoint > 0) {
			sim->part_change_type(i, x, y, parts[i].ctype);
			parts[i].ctype = 0;
			return 1;
		}
		else if (parts[i].temp < elements[parts[i].ctype].MeltingPoint) {
			sim->part_change_type(i, x, y, PT_ICEI);
			return 1;
		}

		if (elements[parts[i].ctype].Update)
			elements[parts[i].ctype].Update(sim, i, x, y, surround_space, nt, parts, pmap);
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	*pixel_mode |= PMODE_BLUR;
	
	if (cpart->ctype > 0 && cpart->ctype < PT_NUM) {
		auto &elements = SimulationData::CRef().elements;
		auto col = elements[cpart->ctype].Colour;
		*colr = col.Red;
		*colg = col.Green;
		*colb = col.Blue;

		if (elements[cpart->ctype].Graphics)
			 elements[cpart->ctype].Graphics(gfctx, cpart, nx, ny, pixel_mode,
				cola, colr, colg, colb, firea, firer, fireg, fireb);
	}
	return 0;
}

static void change_type(ELEMENT_CHANGETYPE_FUNC_ARGS) {
	int ct = sim->parts[i].ctype;
	if (to == PT_LQUD && (!ct || ct < 0 || ct >= PT_NUM || ct == PT_LQUD))
		sim->parts[i].ctype = from;
}