#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_N2() {
	Identifier = "DEFAULT_PT_N2";
	Name = "N2";
	Colour = PIXPACK(0x343685);
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.10f;
	Gravity = 0.00f;
	Diffusion = 1.50f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Mass = 1/1000.0f;
	HeatConduct = 60;
	Description = "Nitrogen gas. Non-flammable. Puts out fires.";
	PhotonReflectWavelengths = 0xFFF;

	Properties = TYPE_GAS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f - 210.0f;
	LowTemperatureTransition = PT_LNTG;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	int hygn_count = 0;
	int hygn[3];
	bool catalyst = false;

	for (int rx = -1; rx <= 1; rx++)
	for (int ry = -1; ry <= 1; ry++)
		if (BOUNDS_CHECK && (rx || ry)) {
			int r = pmap[y + ry][x + rx];
			if (!r) continue;
			int rt = TYP(r);

			// Put out fire
			if (rt == PT_FIRE)
				sim->kill_part(ID(r));
			// ACID + N2 = Aqua regia
			else if (rt == PT_ACID || rt == PT_CAUS) {
				sim->part_change_type(ID(r), x + rx, y + ry, PT_HCL);
				sim->part_change_type(i, x, y, PT_HCL);
				return 1;
			}
			// N2 + NITR -> GEL
			else if (rt == PT_NITR) {
				sim->part_change_type(ID(r), x + rx, y + ry, PT_GEL);
				sim->part_change_type(i, x, y, PT_GEL);
				return 1;
			}
			// 3 H2 + N2 + heat + pressure -> AMNA
			else if (rt == PT_H2) {
				if (hygn_count < 3)
					hygn[hygn_count] = ID(r);
				hygn_count++;
			}
			// Catalyst for haber process
			else if (rt == PT_IRON || rt == PT_PTNM)
				catalyst = true;
		}

	if (hygn_count == 3 && sim->pv[y / CELL][x / CELL] > 20.0f && parts[i].temp > 100.0f + 273.15f
			&& RNG::Ref().chance(1, catalyst ? 200 : 20000)) {
		sim->part_change_type(i, x, y, PT_AMNA);
		parts[i].temp += 5.0f;
		for (int i = 0; i < 3; i++) {
			parts[hygn[i]].temp += 5.0f;
			sim->part_change_type(hygn[i], (int)(parts[hygn[i]].x + 0.5f), (int)(parts[hygn[i]].y + 0.5f), PT_AMNA);
		}
		return 1;
	}

	return 0;
}
