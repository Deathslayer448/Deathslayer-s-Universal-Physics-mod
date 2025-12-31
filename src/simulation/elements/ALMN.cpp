#include "simulation/ElementCommon.h"
#include "simulation/Air.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_ALMN() {
	Identifier = "DEFAULT_PT_ALMN";
	Name = "ALMN";
	Colour = 0xC0C0C0_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 0;
	Weight = 100;

	HeatConduct = 255;
	Description = "Aluminium. Blocks pressure, shatters when hit with high velocity particles.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = ITH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 993.7f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	// Only solid blocks
	if (parts[i].type == PT_ALMN) {
		sim->air->bmap_blockair[y/CELL][x/CELL] = 1;
		sim->air->bmap_blockairh[y/CELL][x/CELL] = 0x8;
	}

	if (parts[i].tmp2 == 1 && sim->rng.chance(1, 100)) {
		sim->kill_part(i);
		return 1;
	}
	if (parts[i].tmp2 > 0)
		parts[i].tmp2--;

	int rx, ry, r;
	for (rx = -1; rx <= 1; ++rx)
	for (ry = -1; ry <= 1; ++ry)
		if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
			r = pmap[y + ry][x + rx];
			if (!r) {
				if (parts[i].tmp2) {
					int ni = sim->create_part(-1, x + rx, y + ry, PT_FIRE);
					if (ni > -1) {
						parts[ni].dcolour = 0xFFFFFFFF;
						parts[ni].life = sim->rng.between(5, 20);
						parts[ni].temp = parts[i].temp + 200.0f;
					}
				}
				continue;
			}

			// Yes the self type check is needed as powdered ALMN shouldn't transform
			float v = sqrtf(parts[ID(r)].vx*parts[ID(r)].vx + parts[ID(r)].vy*parts[ID(r)].vy);
			if (parts[i].type == PT_ALMN && v > 10.0f) {
				sim->part_change_type(i, x, y, PT_BRKN);
				parts[i].ctype = PT_ALMN;
				return 1;
			}
			else if ((TYP(r) == PT_FIRE || TYP(r) == PT_PLSM) && parts[i].tmp2 == 0 && sim->rng.chance(1, 2500)
					&& parts[i].temp > 550.0f + 273.15f) {
				parts[i].tmp2 = sim->rng.between(10, 2000);
			}
		}
	return 0;
}
