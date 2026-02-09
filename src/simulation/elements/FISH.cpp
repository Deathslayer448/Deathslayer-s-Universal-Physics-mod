#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_FISH() {
	Identifier = "DEFAULT_PT_FISH";
	Name = "FISH";
	Colour = PIXPACK(0x8DAFBA);
	MenuVisible = 0;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.1f;
	AirDrag = 0.003f * CFDS;
	AirLoss = 0.94f;
	Loss = 1.0f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 30;
	Explosive = 1;
	Meltable = 0;
	Hardness = 2;

	Mass = 30/1000.0f;

	HeatConduct = 150;
	Description = "Freshwater fish, swims in water and eats ANT and SEED.";

	Properties = TYPE_PART | PROP_EDIBLE;
	//FoodValue = 3;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 6.0f;
	HighPressureTransition = PT_DUST;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 273.15f + 90.0f;
	HighTemperatureTransition = PT_DUST;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	/** 
	 * Literally just move around randomly
	 * Flock like behavior is difficult with liquid swapping
	 * 
	 * pavg = velocity vector
	 */

	// Update new velocity if still or no new velocity
	if (!(parts[i].pavg[0] || parts[i].pavg[1]) || RNG::Ref().chance(1, 3)) {
		if (RNG::Ref().chance(1, 2)) {
			parts[i].pavg[0] = RNG::Ref().chance(1, 2) ? -1 : 1;
			parts[i].pavg[1] = 0;
		}
		else {
			parts[i].pavg[1] = RNG::Ref().chance(1, 2) ? -1 : 1;
			parts[i].pavg[0] = 0;
		}
	}

	bool touching_water = false;
	int rx, ry, r, rt;

	for (rx = -2; rx < 3; rx++)
		for (ry = -2; ry < 3; ry++)
			if (BOUNDS_CHECK && (rx || ry)) {
				r = pmap[y + ry][x + rx];
				if (!r) {
					// Redirect velocity away from air
					parts[i].pavg[0] = -isign(rx);
					parts[i].pavg[1] = -isign(ry);
					continue;
				}
				rt = TYP(r);

				// Redirect velocity towards SEED and ANT
				if (rt == PT_SEED || rt == PT_ANT || rt == PT_POTO) {
					parts[i].pavg[0] = isign(rx);
					parts[i].pavg[1] = isign(ry);
				}

				if (abs(rx) < 2 && abs(ry) < 2) {
					// Water touch check
					if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_SWTR || rt == PT_WTRV)
						touching_water = true;

					// Eat SEED and ANT
					else if ((rt == PT_ANT || rt == PT_SEED || rt == PT_POTO) && RNG::Ref().chance(1, 40))
						sim->kill_part(ID(r));
				}
			}

	// Chance to die if not touching water
	if (!touching_water && RNG::Ref().chance(1, 1000)) {
		sim->part_change_type(i, parts[i].x, parts[i].y, PT_DUST);
		return 0;
	}

	// Try to swap with liquid at new velocity
	rx = round(parts[i].pavg[0]), ry = round(parts[i].pavg[1]);
	if (BOUNDS_CHECK && (rx || ry)) {
		r = pmap[y + ry][x + rx];
		if (!r) { // Cannot move there, pick new velocity next frame by resetting pavgs to 0
			parts[i].pavg[0] = parts[i].pavg[1] = 0;
			return 0;
		}
		rt = TYP(r);
		
		if (sim->elements[rt].Properties & TYPE_LIQUID) {
			parts[i].x = parts[ID(r)].x;
			parts[i].y = parts[ID(r)].y;
			parts[ID(r)].x = x;
			parts[ID(r)].y = y;
			pmap[y][x] = r;
			pmap[y+ry][x+rx] = PMAP(i, parts[i].type);
			return 0;
		}
	}

	return 0;
}
