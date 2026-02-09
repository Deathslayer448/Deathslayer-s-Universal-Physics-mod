#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
void Element_CLST_create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_ALGE() {
	Identifier = "DEFAULT_PT_ALGE";
	Name = "ALGE";
	Colour = PIXPACK(0x70E680);
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 20;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;
	PhotonReflectWavelengths = 0x0007C000;

	Mass = 20/1000.0f;

	HeatConduct = 70;
	Description = "Algae. Grows on the surface of water.";

	Properties = TYPE_PART | PROP_EDIBLE;
	//FoodValue = 1;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 300.0f + 273.15f;
	HighTemperatureTransition = PT_CRBN;

	Update = &update;
	Graphics = &graphics;
	Create = &Element_CLST_create;
}

static int update(UPDATE_FUNC_ARGS) {
	for (int rx = -1; rx <= 1; ++rx)
	for (int ry = -1; ry <= 1; ++ry)
		if (BOUNDS_CHECK && (rx || ry)) {
			int r = pmap[y + ry][x + rx];
			// Look for nearby water
			int rt = TYP(r);
			if (RNG::Ref().chance(1, 1000)) {
				if (sim->elements[rt].Properties & PROP_WATER) {
					// Look for empty spot near the water
					for (int rx2 = -1; rx2 <= 1; ++rx2)
					for (int ry2 = -1; ry2 <= 1; ++ry2) {
						r = pmap[y + ry + ry2][x + rx + rx2];
						if (!r) {
							sim->create_part(-1, x + rx + rx2, y + ry + ry2, PT_ALGE);
							return 0;
						}
					}
				}
			}

			else if (rt == PT_SMKE || rt == PT_CO2)
				sim->part_change_type(ID(r), x + rx, y + ry, PT_O2);
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	int z = (cpart->tmp - 5) * 16; // Speckles!
	*colr += z;
	*colg += z;
	*colb += z;
	return 0;
}
