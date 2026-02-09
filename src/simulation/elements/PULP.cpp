#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_PULP() {
	Identifier = "DEFAULT_PT_PULP";
	Name = "PULP";
	Colour = PIXPACK(0xEAEAEA);
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
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

	Flammable = 50;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Mass = 31/1000.0f;

	DefaultProperties.temp = R_TEMP - 2.0f + 273.15f;
	DefaultProperties.tmp3 = 40;

	HeatConduct = 70;
	Description = "Paper pulp. Slowly turns into paper.";
	Properties = TYPE_LIQUID | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH; // 451 F
	HighTemperatureTransition = NT;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {

	int tmp3_dec_bonus = std::max(0, (int)((parts[i].temp - 273.15f) / 5.0f));

	if (parts[i].tmp3 <= 0) {
		sim->part_change_type(i, x, y, PT_PAPR);
		parts[i].tmp3 = 0;
		return 1;
	}
	for (int rx = -1; rx <= 1; ++rx)
	for (int ry = -1; ry <= 1; ++ry)
		if (BOUNDS_CHECK && (rx || ry)) {
			int r = pmap[y + ry][x + rx];
			if (!r && tmp3_dec_bonus < 10) continue;

			int rt = TYP(r);
			bool is_water = rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_SWTR || rt == PT_WTRV;
		
			if (parts[i].tmp3 < 60 && is_water) {
				parts[i].tmp3 += 10;
				sim->kill_part(ID(r));
			}
		}

	
	if (tmp3_dec_bonus >= 10 && sim->pmap_count[y][x] < 3)
	{

		parts[i].tmp3 -= 10;
		sim->create_part(-3, x, y, PT_WATR);
	}
	else if (tmp3_dec_bonus >= 10)
	{
		parts[i].tmp3 -= 10;
		sim->create_part(-3, x, y, PT_WTRV);
	}
	return 0;
}
