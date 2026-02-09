#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);
//void Element_CLST_create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_POTO() {
	Identifier = "DEFAULT_PT_POTO";
	Name = "POTO";
	Colour = PIXPACK(0xB08464);
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Mass = 50/1000.0f;

	HeatConduct = 150;
	Description = "Potato. The essence of life";

	

	Properties = TYPE_PART | PROP_EDIBLE;
	//FoodValue = 10;

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
	Create = &create;
	//Create = &Element_CLST_create;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties:
	 * tmp4   - food
	 * tmpcity[5] - graphics
	 * 
	 */
	//if (parts[i].temp > parts[i].pavg[0])
		//parts[i].pavg[0] = parts[i].temp;

	if (parts[i].tmp4 <= 0)
		sim->kill_part(i);
	//if (parts[i].temp > 100.0f + 273.15f && parts[i].tmp2 > 0) {
		
	/*	for (int ry = -1; ry <= 1; ++ry)
			for (int rx = -1; rx <= 1; ++rx)
			if (BOUNDS_CHECK && (rx || ry)) {
				int r = pmap[y + ry][x + rx];
				if (!r) {
					parts[i].life--;
					sim->create_part(-1, x + rx, y + ry, PT_WTRV);
					return 0;
				}
			}*/
	//}

	return 0;
}




static void create(ELEMENT_CREATE_FUNC_ARGS) {

	sim->parts[i].tmp4 = 100;
	sim->parts[i].carbons = 100;
	sim->parts[i].tmpville[6] = RNG::Ref().between(0, 6);
}





static int graphics(GRAPHICS_FUNC_ARGS) {
	int z = (cpart->tmpville[6] - 5) * 8; // Speckles!
	*colr += z;
	*colg += z;
	*colb += z;

	return 0;
}
