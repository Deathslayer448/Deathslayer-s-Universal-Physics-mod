#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element_CLST_create(ELEMENT_CREATE_FUNC_ARGS);
void Element_RDMD_create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_CRBN() {
	Identifier = "DEFAULT_PT_CRBN";
	Name = "CRBN";
	Colour = 0x444444_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
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

	Flammable = 200;
	Explosive = 0;
	Meltable = 0;
	Hardness = 2;
	PhotonReflectWavelengths = 0x00000000;

	Weight = 90;

	HeatConduct = 10;
	Description = "Carbon dust. Turns into RDMD under heat and pressure, superconducts when cold.";

	Properties = TYPE_PART | PROP_CONDUCTS | PROP_NEUTPASS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 3600.0f + 273.15f;
	HighTemperatureTransition = PT_CO2;

	Update = &update;
	Graphics = &graphics;
	Create = &Element_CLST_create;
}

static int update(UPDATE_FUNC_ARGS) {
	// Stay still if tmp2
	if (parts[i].tmp2)
		parts[i].vx = parts[i].vy = 0;

	// Solidify into realistic diamond
	if (parts[i].temp > 273.15f + 2500.0f && sim->pv[y/CELL][x/CELL] > 1000.0f && sim->rng.chance(1, 20)) {
		sim->part_change_type(i, parts[i].x, parts[i].y, PT_DMND);
	//	Element_RDMD_create(sim, i, x, y, PT_RDMD, -1);
		return 0;
	}

	int rx, ry, r, rt;
	bool seen_h2 = false, seen_fire = false, seen_sprk = false;
	int crbn_count = 0;
	int xor_check = 0;

	for (rx = -1; rx <= 1; rx++)
	for (ry = -1; ry <= 1; ry++)
		if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
			r = pmap[y + ry][x + rx];
			if (!r) r = sim->photons[y + ry][x + rx];
			if (!r) continue;
			rt = TYP(r);

			// Solidify
			if (rt == PT_PSTE) {
				parts[i].tmp2 = 1;
				sim->kill_part(ID(r));
			}

			// Purify water
			else if (rt == PT_WATR)
				sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y, PT_DSTW);
			else if (rt == PT_SLTW)
				sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y,
					sim->rng.chance(1, 2) ? PT_SALT : PT_DSTW);

			// Convert molten IRON into METL (steel)
			else if (rt == PT_LAVA && parts[ID(r)].ctype == PT_IRON) {
				parts[ID(r)].ctype = PT_METL;
				if (sim->rng.chance(1, 5)) {
					sim->kill_part(i);
					return 0;
				}
			}

			// Slow down neutrons
			else if (rt == PT_NEUT) {
				parts[ID(r)].vx *= 0.8f;
				parts[ID(r)].vy *= 0.8f;
			}
		}

	// Prevent conduction if conducted from PSCN and NSCN at same time
	for (rx = -2; rx <= 2; rx++)
	for (ry = -2; ry <= 2; ry++)
		if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
			r = pmap[y + ry][x + rx];
			if (!r) continue;
			rt = TYP(r);

			if (rt == PT_FIRE || rt == PT_PLSM)
				seen_fire = true;
			else if (rt == PT_H2)
				seen_h2 = true;
			else if (rt == PT_SPRK) {
				seen_sprk = true;
				if (parts[ID(r)].ctype == PT_PSCN)
					xor_check |= 1;
				else if (parts[ID(r)].ctype == PT_NSCN)
					xor_check |= 2;
			}
			else if (rt == PT_CRBN)
				++crbn_count;
		}
	
	// Prevent conduction if both PSCN and NSCN
	if (xor_check == 3)
		parts[i].life = 1;
	// H2 + FIRE + CRBN = GAS
	if (seen_h2 && (seen_fire || (parts[i].temp > 10.0f + 273.15f && sim->rng.chance(1, 200) ))) {
		parts[i].temp += 5.0f;
		sim->part_change_type(i, parts[i].x, parts[i].y, PT_GAS);
	}
	// FIRE + CRBN = CO2
	else if (seen_fire && sim->rng.chance(1, 20))
		sim->part_change_type(i, parts[i].x, parts[i].y, PT_CO2);
	// Thin wires superconduct
	else if (parts[i].temp < 100.0f && seen_sprk && crbn_count <= 2 && xor_check != 3)
		sim->FloodINST(x, y);

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	int z = (cpart->tmp - 5) * 5; // Speckles!
	*colr += z;
	*colg += z;
	*colb += z;

	return 0;
}
