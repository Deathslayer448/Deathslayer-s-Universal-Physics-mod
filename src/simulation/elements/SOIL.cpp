#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
void Element_CLST_create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_SOIL() {
	Identifier = "DEFAULT_PT_SOIL";
	Name = "SOIL";
	Colour = PIXPACK(0x9F7616);
	MenuVisible = 0;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.2f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 2;
	Hardness = 2;

	Mass = 55/1000.0f;

	HeatConduct = 70;
	Description = "Soil. Turns into mud when wet, great for growing seeds.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 273.15f + 60.0f;
	HighTemperatureTransition = PT_CLST;

	Update = &update;
	Graphics = &graphics;
	Create = &Element_CLST_create;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * If tmp2 = 1 is a tunnel, don't move
	 * If tmp2 = 2 is a tunnel, allow particles
	 */
	if (parts[i].temp < 273.15f || parts[i].tmp2 > 0) { // Permafrost or tunnel, dont move
		parts[i].vx = parts[i].vy = 0;
	}

	int rx, ry, r, rt;
	for (rx = -1; rx <= 2; ++rx)
		for (ry = -1; ry <= 2; ++ry)
			if (BOUNDS_CHECK && (rx || ry)) {
				r = pmap[y + ry][x + rx];
				if (!r) continue;
				rt = TYP(r);

				// Absorb water
				if (RNG::Ref().chance(1, 3)) {
					if (rt == PT_SLTW && RNG::Ref().chance(1, 5)) {
						sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y, PT_SALT);
						sim->part_change_type(i, parts[i].x, parts[i].y, PT_MUD);
						return 0;
					} 
					else if (rt == PT_SWTR && RNG::Ref().chance(1, 5)) {
						sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y, PT_SUGR);
						sim->part_change_type(i, parts[i].x, parts[i].y, PT_MUD);
						return 0;
					}
					else if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_SWTR || rt == PT_WTRV) {
						sim->kill_part(ID(r));
						sim->part_change_type(i, parts[i].x, parts[i].y, PT_MUD);
						return 0;
					}
				}
			}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	int z = (cpart->tmp - 5) * 8;//speckles!
	*colr += z;
	*colg += z;
	*colb += z;

	// Turn blueish if cold
	if (cpart->temp < 273.15f) {
		*colg += (273.15f - cpart->temp) / 8;
		*colb += (273.15f - cpart->temp) / 3;
	}

	// Darken if part of tunnel
	if (cpart->tmp2 == 2) {
		*colr *= 0.6f;
		*colg *= 0.6f;
		*colb *= 0.6f;
	}
	return 0;
}