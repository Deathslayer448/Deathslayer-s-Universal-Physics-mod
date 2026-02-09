#include "simulation/ElementCommon.h"
#include <iostream>

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

#define PI 3.14159f
#define SPOKES 32
#define NOTEMPTY(x, y) (pmap[y - 1][x - 1] || pmap[y - 1][x] || pmap[y - 1][x + 1] || pmap[y][x + 1] ||\
					    pmap[y][x - 1] || pmap[y + 1][x - 1] || pmap[y + 1][x] || pmap[y + 1][x + 1])

void Element::Element_SPDR() {
	Identifier = "DEFAULT_PT_SPDR";
	Name = "SPDR";
	Colour = PIXPACK(0x444444);
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

	Mass = 32/1000.0f;

	HeatConduct = 70;
	Description = "Spider. Spins webs and eats captured creatures.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 5.0f;
	HighPressureTransition = PT_BCTR;
	LowTemperature = 273.15f - 5.0f;
	LowTemperatureTransition = PT_SNOW;
	HighTemperature = 273.15f + 60.0f;
	HighTemperatureTransition = PT_DUST;

	Update = &update;
	Create = &create;
}

void Element_SPDR_intersect_line(Simulation *sim, int sx, int sy, float vx, float vy, int &x, int &y, int type=1, int type2=0) {
	/**
	 * Calculate line intersect starting from sx and sy, drawing a line
	 * along velocity vx, vy, the first object it intersects and returns coordinates
	 * of the intersection, storing in x and y. Ignores walls that don't block solids.
	 * 
	 * If no intersection is found and it reaches the edge of the screen, returns -1 
	 * for each parameter.
	 * 
	 * Type:
	 * 	1: Ignore type2 (by default NONE is always ignored)
	 * 	2: Ignore liquid
	 * 	3. Ignore gas
	 * 	4. Solid and powder only
	 * 	5. Solid only
	 *  6. Ignore type2 if within 10px of start
	 *  7. Seek out ctype
	 *  8. Seek out ctype if not within 10px of start, otherwise ignore all
	 *  9. Solid only, return edge location
	 * FFLD always blocks regardless of setting
	 * 
	 * Type2: Type parameter for type, ie PT_SPDR
	 */
	float tx = sx, ty = sy;
	int px, py, r;

	// No movement
	if (vx == 0 && vy == 0) {
		x = y = -1;
		return;
	}

	// Reduce velocity magnitudes so neither > 1
	float larger = abs(vx) < abs(vy) ? abs(vy) : abs(vx);
	if (larger > 1) {
		vx /= larger;
		vy /= larger;
	}

	while (true) {
		px = (int)tx, py = (int)ty;
		tx += vx, ty += vy;

		// Out of bounds
		if ((int)tx < 0 || (int)ty < 0 || (int)tx >= XRES || (int)ty >= YRES) {
			if (type == 9) { // Return location
				if ((int)tx < 0) x = 0;
				else if ((int)tx >= XRES) x = XRES - 1;
				else x = (int)tx;

				if ((int)ty < 0) y = 0;
				else if ((int)ty >= YRES) y = YRES - 1;
				else y = (int)ty;
				return;
			}
			x = y = -1;
			return;
		}
		// Same position as before we can skip check
		if ((int)tx == px && (int)ty == py)
			continue;

		// Too far from start, change type from 6 to 5 and 8 to 7
		int dis = (tx - x) * (tx - x) + (ty - y) * (ty - y);
		if (dis > 100 && type == 6)
			type = 5;
		if (dis > 100 && type == 8)
			type = 7;

		// Found particle
		r = sim->pmap[(int)ty][(int)tx];
		if (r && (
			(type == 1 && TYP(r) != type2) ||
			(type == 2 && !(sim->elements[TYP(r)].Properties & TYPE_LIQUID)) ||
			(type == 3 && !(sim->elements[TYP(r)].Properties & TYPE_GAS)) ||
			(type == 4 && (sim->elements[TYP(r)].Properties & TYPE_SOLID || sim->elements[TYP(r)].Properties & TYPE_PART)) ||
			((type == 5 || type == 9) && sim->elements[TYP(r)].Properties & TYPE_SOLID) ||
			(type == 6 && TYP(r) != type2) ||
			(type == 7 && TYP(r) == type2))) {
			x = (int)tx, y = (int)ty;
			return;
		}

		// Blocking wall
		if (sim->IsWallBlocking((int)tx, (int)ty, PT_DMND)) {
			x = (int)tx, y = (int)ty;
			return;
		}
	}
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	sim->parts[i].tmp = RNG::Ref().chance(1, 2);
	sim->parts[i].pavg[0] = -1;
	sim->parts[i].pavg[1] = -1;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties:
	 * tmp: dir (0 = left / up, 1 = right / down)
	 * tmp2: web stage
	 * 	0 = looking to construct
	 *  1 = first line and second line
	 *  2 = spokes and rings
	 * 	3 = done
	 * pavg[0], pavg[1]: Target location to move
	 */
	int rx, ry, r, rt;
	bool touching = false;

	for (rx = -1; rx < 2; ++rx)
		for (ry = -1; ry < 2; ++ry)
			if (BOUNDS_CHECK && (rx || ry)) {
				r = pmap[y + ry][x + rx];
				if (!r) continue;
				rt = TYP(r);

				if (sim->elements[rt].Properties & TYPE_SOLID)
					touching = true;

				// Eat food on web
				if (rt == PT_WEB && parts[ID(r)].ctype)
					sim->kill_part(ID(r));

				// Randomly move along webs if its tmp2 is 3
				else if (rt == PT_WEB && parts[i].tmp2 == 3 && RNG::Ref().chance(1, 8)) {
					parts[i].vx = rx;
					parts[i].vy += ry;
					return 0;
				}
			}

	// Dont move if touching a solid
	if (touching) {
		parts[i].vx = 0;
		parts[i].vy = 0;
	}
	// Reset target location if "falls off" web
	else {
		parts[i].tmp2 = 0;
		parts[i].pavg[0] = parts[i].pavg[1] = -1;
	}

	// At target, reset
	if (abs(x - parts[i].pavg[0]) < 2 && abs(y - parts[i].pavg[1]) < 2)
		parts[i].pavg[0] = parts[i].pavg[1] = -1;

	// Move to target
	if (parts[i].pavg[0] >= 0 && parts[i].pavg[1] >= 0) {
		parts[i].vx += (parts[i].pavg[0] - x) / 200.0f;
		parts[i].vy += (parts[i].pavg[1] - y) / 200.0f;
	}
	// Move along solids
	else if (parts[i].tmp2 == 0) {
		// Move left and up, randomize which one to avoid loops
		if (parts[i].tmp == 0) {
			if (RNG::Ref().chance(1, 2) && !pmap[y][x - 1] && NOTEMPTY(x - 1, y))
				--parts[i].x;
			else if (!pmap[y - 1][x] && NOTEMPTY(x, y - 1))
				--parts[i].y;
			else
				parts[i].tmp = 1; // Swap move dir
		} 
		// Move down and right, randomize which one to avoid loops
		else if (parts[i].tmp == 1) {
			if (RNG::Ref().chance(1, 2) && !pmap[y][x + 1] && NOTEMPTY(x + 1, y))
				++parts[i].x;
			else if (!pmap[y + 1][x] && NOTEMPTY(x, y + 1))
				++parts[i].y;
			else
				parts[i].tmp = 0; // Swap move dir
		}
	}
	


	// ------ WEB SPINING -------- //

	// Randomly spin a web in a direction
	// tmp2 = 0 means not started yet, randomly decide to start
	// tmp2 = 1 means started, make the 2nd line
	if ((parts[i].tmp2 == 1 && parts[i].pavg[0] == -1) ||
		(parts[i].tmp2 == 0 && parts[i].pavg[0] == -1 && RNG::Ref().chance(1, 2000) && touching)) {
		int x1, y1;
		float angle = 0.0f;

		// Attempt to web 5 times
		for (unsigned char k = 0; k < 5; ++k) {
			angle = RNG::Ref().uniform01() * 2 * PI;
			Element_SPDR_intersect_line(sim, x, y, cos(angle), sin(angle), x1, y1, 5);
			if (x1 == -1 && y1 == -1)
				Element_SPDR_intersect_line(sim, x, y, -cos(angle), -sin(angle), x1, y1, 5);
			if (x1 != -1 && y1 != -1) {
				// Web must be a certain distance, 30 - 350px
				int dis = (x1 - x) * (x1 - x) + (y1 - y) * (y1 - y);
				if (dis > 30 * 30 && dis < 350 * 350) {
					sim->CreateLine(x, y, x1, y1, PT_WEB);
					parts[i].pavg[0] = (x + x1) / 2;
					parts[i].pavg[1] = (y + y1) / 2;
					++parts[i].tmp2;
					return 0;
				}
			}
		}
	}
	// Make the spokes and rings
	else if (parts[i].tmp2 == 2 && parts[i].pavg[0] == -1) {
		// Attempt 32 spokes from a random starting point
		float start = RNG::Ref().uniform01() * 2 * PI;
		float angle;
		int x1, y1, x2, y2, x3, y3, webs = 0;

		for (unsigned int k = 0; k < SPOKES; ++k) {
			angle = start + (float)k / SPOKES * 2 * PI;
			Element_SPDR_intersect_line(sim, x, y, cos(angle), sin(angle), x1, y1, 6, PT_WEB);
			if (x1 != -1 && y1 != -1) {
				// Web must be a certain distance. Spoke webs can be shorter
				// between 3 - 350px long
				int dis = (x1 - x) * (x1 - x) + (y1 - y) * (y1 - y);
				if (dis > 3 * 3 && dis < 350 * 350) {
					sim->CreateLine(x, y, x1, y1, PT_WEB);
					++webs;

					// Cheat on the rings since its very hard to implement
					for (float ringdis = 0.4f; ringdis < 1; ringdis += 0.2f) {
						x3 = x + (x1 - x) * ringdis;
						y3 = y + (y1 - y) * ringdis;
						Element_SPDR_intersect_line(sim, x3, y3, cos(angle - PI / 2), sin(angle - PI / 2), x2, y2, 6, PT_WEB);

						if (x2 != -1 && y2 != -1) {
							int dis = (x2 - x3) * (x2 - x3) + (y2 - y3) * (y2 - y3);
							if (dis < 100 * 100 && dis > 3 * 3)
								sim->CreateLine(x2, y2, x3, y3, PT_WEB);
						}
					}
				}
			}
		}

		// Advance to next stage
		if (webs) ++parts[i].tmp2;
	}

	return 0;
}
