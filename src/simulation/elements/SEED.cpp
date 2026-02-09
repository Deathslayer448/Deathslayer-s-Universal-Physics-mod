#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
void Element_CLST_create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_SEED() {
	Identifier = "DEFAULT_PT_SEED";
	Name = "SEED";
	Colour = PIXPACK(0xA3B808);
	MenuVisible = 0;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 1;

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;
	PhotonReflectWavelengths = 0x3FFFFFC0;

	Mass = 32/1000.0f;

	HeatConduct = 70;
	Description = "Seed, sprouts into trees when planted in soil.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.15f - 20.0f;
	LowTemperatureTransition = PT_SNOW;
	HighTemperature = 273.15f + 300.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
	Create = &Element_CLST_create;
}

static int update(UPDATE_FUNC_ARGS) {
	/** 
	 * Properties:
	 * tmp  - Graphics speckles
	 * tmp2 - Highest tmp felt
	 */

	bool near_soil = false;
	bool near_wood = false;
	float soil_temp = 0.0f;

	// If temp < 5 C makes a pine tree
	// If temp < 30 C makes an oak like tree
	// If temp < 42 C makes a palm tree
	// Else "die"
	if (parts[i].tmp2 <= 42.0f + 273.15f) {  // If it had been exposed to over 42 C the seed "dies"
		int rx, ry, r;
		for (rx = -2; rx <= 2; ++rx)
			for (ry = -2; ry <= 2; ++ry)
				if (BOUNDS_CHECK && (rx || ry)) {
					r = pmap[y + ry][x + rx];
					if (!r) continue;
					
					// Plant self near soil
					if ((abs(rx) < 2 && abs(ry) < 2) && TYP(r) == PT_SOIL && (rx == 0 || ry == 0)) {
						near_soil = true;
						soil_temp = parts[ID(r)].temp;
					}
					else if (TYP(r) == PT_WOOD)
						near_wood = true;
				}
	}

	// Grow
	if (near_soil) {
		// Consider direction of gravity
		float angle = 0.0f;
		int radius;
		switch (sim->gravityMode) {
			case 0: 
				angle = -3.1415f / 2; // Grow up, normal gravity
				break;
			case 1:
				angle = RNG::Ref().uniform01() * 3.1415f * 2; // No gravity, random
				break;
			case 2:
				angle = atan2(y - YRES / 2, x - XRES / 2); // Radial gravity
				break;
		}

		// Grow a tree
		if (!near_wood) {
			// Try to grow a pine tree
			if (soil_temp < 273.15f + 5.0f) {
				radius = RNG::Ref().between(25, 35);
				sim->CreateLine(x, y, x + radius * cos(angle), y + radius * sin(angle), PT_WOOD);
				// Pine trees have slightly angled up branches
				int sx, sy, branch_length, leaf_length;
				for (int dy = radius; dy >= 0; dy -= 4) {
					sx = x + dy * cos(angle), sy = y + dy * sin(angle);
					branch_length = (1 - (float)dy / radius) * 5;
					leaf_length = branch_length + 4;

					// PLNT on the branches
					sim->CreateLine(sx, sy, sx + round(leaf_length * cos(angle - 3.1415f * 0.4f)), sy - leaf_length * cos(angle - 3.1415f * 0.4f), PT_PLNT);
					sim->CreateLine(sx, sy, sx + round(leaf_length * cos(angle + 3.1415f * 0.4f)), sy - leaf_length * cos(angle - 3.1415f * 0.4f), PT_PLNT);
					sim->CreateLine(sx, sy, sx + round(leaf_length * cos(angle - 3.1415f * 0.7f)), sy - leaf_length * cos(angle - 3.1415f * 0.7f), PT_PLNT);
					sim->CreateLine(sx, sy, sx + round(leaf_length * cos(angle + 3.1415f * 0.7f)), sy - leaf_length * cos(angle - 3.1415f * 0.7f), PT_PLNT);

					// Wood branches
					sim->CreateLine(sx, sy, sx + round(branch_length * cos(angle - 3.1415f * 0.4f)), sy + branch_length * cos(angle - 3.1415f * 0.4f), PT_WOOD);
					sim->CreateLine(sx, sy, sx + round(branch_length * cos(angle + 3.1415f * 0.4f)), sy + branch_length * cos(angle - 3.1415f * 0.4f), PT_WOOD);
				}
			}
			// Try to grow an oak tree
			else if (soil_temp < 273.15f + 30.0f) {
				radius = RNG::Ref().between(5, 10);
				int topx = x + radius * cos(angle);
				int topy = y + radius * sin(angle);
				sim->CreateLine(x, y, topx, topy, PT_WOOD);
				
				float temp = 0.0f;
				int branch_size, tx, ty;
				int radius = RNG::Ref().between(5, 8);

				for (unsigned int k = 0; k < 2; ++k) {
					temp = (k == 1 ? -1 : 1) * RNG::Ref().uniform01() * 3.1415f / 3;
					branch_size = RNG::Ref().between(5, 10);
					tx = topx + cos(angle + temp) * branch_size;
					ty = topy + sin(angle + temp) * branch_size;

					sim->CreateLine(topx, topy, tx, ty, PT_WOOD);
				
					// Leaf
					for (int dx = -radius; dx <= radius; ++dx)
						for (int dy = -radius; dy <= radius; ++dy) {
							if (RNG::Ref().chance(6, 10) && dx * dx + dy * dy < radius * radius)
								sim->create_part(-1, tx + dx, ty + dy, PT_PLNT);
						}
					}
			}
			// Try to grow a palm tree
			else if (soil_temp < 273.15f + 42.0f) {
				radius = RNG::Ref().between(5, 15);
				int topx = x + radius * cos(angle);
				int topy = y + radius * sin(angle);
				int mx = x + (topx - x) / 2;
				int my = y + (topy - y) / 2;
				int diffx = RNG::Ref().between(-3, 3);
				int diffy = RNG::Ref().between(-3, 3);

				// Trunk is "curved"
				sim->CreateLine(x, y, mx + diffx, my + diffy, PT_WOOD);
				sim->CreateLine(mx + diffx, my + diffy, topx, topy, PT_WOOD);

				// Draw 4 "palm" leaves
				float leaf_angle;
				int lx, ly, lmx, lmy;
				int leaf_size = 10;
				for (unsigned int k = 0; k < 4; ++k) {
					leaf_angle = (k / 2 + 1) * (3.1415f / 4) * (k % 2 == 0 ? -1 : 1);
					lx = topx + leaf_size * cos(angle + leaf_angle);
					ly = topy + leaf_size * sin(angle + leaf_angle);
					lmx = topx + leaf_size * cos(angle + leaf_angle * 0.8f);
					lmy = topy + leaf_size * sin(angle + leaf_angle * 0.8f);

					sim->CreateLine(topx, topy, lmx, lmy, PT_PLNT);
					sim->CreateLine(lmx, lmy, lx, ly, PT_PLNT);
				}
			}

			// Planted, commit die
			sim->part_change_type(i, x, y, PT_WOOD);
			return 1;
		}

		// Grow grass
		else {
			radius = RNG::Ref().between(0, 4);
			sim->CreateLine(x, y, x + radius * cos(angle), y + radius * sin(angle), PT_PLNT);

			// Planted, commit die
			parts[i].tmp = 0; // Avoid VINE growth
			sim->part_change_type(i, x, y, PT_PLNT);
			return 1;
		}
	}

	// Save max temp
	if (parts[i].temp > 350 && parts[i].temp > parts[i].tmp2)
		parts[i].tmp2 = (int)parts[i].temp;
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	int z = (cpart->tmp - 5) * 8;//speckles!
	*colr += z;
	*colg += z;
	*colb += z;

	float maxtemp = std::max((float)cpart->tmp2, cpart->temp);
	if (maxtemp > 300) {
		*colr += (int)restrict_flt((maxtemp - 300) / 5, 0, 58);
		*colg -= (int)restrict_flt((maxtemp - 300) / 2, 0, 102);
		*colb += (int)restrict_flt((maxtemp - 300) / 5, 0, 70);
	}
	else if (maxtemp < 273) {
		*colg += (int)restrict_flt((273 - maxtemp) / 4, 0, 255);
		*colb += (int)restrict_flt((273 - maxtemp) / 1.5, 0, 255);
	}
	return 0;
}
