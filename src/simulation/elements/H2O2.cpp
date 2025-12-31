#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
int Element_WATR_update(UPDATE_FUNC_ARGS);

void Element::Element_H2O2() {
	Identifier = "DEFAULT_PT_H2O2";
	Name = "H2O2";
	Colour = 0x2738E6_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.12f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 1;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 35;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	HeatConduct = 29;
	Description = "Hydrogen Peroxide. Powerful oxidizer and desinfectant.";
		
	Properties = TYPE_LIQUID|PROP_CONDUCTS|PROP_LIFE_DEC|PROP_NEUTPASS|PROP_DEADLY;


	DefaultProperties.water = 80;
	DefaultProperties.oxygens = 100;
	DefaultProperties.capacity = 400;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f - 0.4f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f + 50.2f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {


	Element_WATR_update(sim, i, x, y, surround_space, nt, parts, pmap);
	if (parts[i].oxygens < 10)
		sim->part_change_type(i, x, y, PT_WATR);
	int rx, ry, r, rt;

	if (sim->rng.chance(1, restrict_flt(30000 - 10 * (int)parts[i].temp, 1, MAX_TEMP))) {
		int otwo = sim->create_part(-3, x, y, PT_O2);
		parts[otwo].oxygens = std::min(20, parts[i].water);
		parts[i].oxygens -= std::min(20, parts[i].water);
	//	sim->part_change_type(i, x, y, sim->rng.chance(1, 2) ? PT_DSTW : PT_O2);
		//return 1;
	}

	//for (rx = -1; rx < 2; ++rx)
	//for (ry = -1; ry < 2; ++ry)
	//	if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
	//		r = pmap[y + ry][x + rx];
	//		if (!r) continue;
	//		rt = TYP(r);

	//		// Break down VIRS and BCTR
	//		//if (rt == PT_VIRS || rt == PT_VRSG || rt == PT_VRSS || rt == PT_BCTR) {
	//			//sim->kill_part(ID(r));
	//			//continue;
	//		//}

	//	/*	else if (rt == PT_SUFR) {
	//			sim->part_change_type(ID(r), x + rx, y + ry, PT_ACID);
	//			sim->part_change_type(i, x, y, PT_ACID);
	//			return 1;
	//		}*/

	//		//if (elements[rt].Hardness && sim->rng.between(1, 100) > elements[rt].Hardness &&
	//		//		sim->rng.chance(1, 500) && !(elements[rt].Properties & TYPE_LIQUID)) {
	//		//	sim->kill_part(ID(r));
	//			//sim->kill_part(i);
	//			//return 1;
	//		}
		

	return 0;
}
