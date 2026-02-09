#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_ANT() {
	Identifier = "DEFAULT_PT_ANT";
	Name = "ANT";
	Colour = PIXPACK(0xD4C4BA);
	MenuVisible = 0;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.1f;
	AirDrag = 0.09f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.3f;
	Diffusion = 0.60f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 30;
	Explosive = 1;
	Meltable = 0;
	Hardness = 2;

	Mass = 90/1000.0f;

	HeatConduct = 150;
	Description = "Ants, digs tunnels in soil and eats SUGR. Flammable.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 5.0f;
	HighPressureTransition = PT_BCTR;
	LowTemperature = 273.15f - 5.0f;
	LowTemperatureTransition = PT_SNOW;
	HighTemperature = 50.0f + 273.15f;
	HighTemperatureTransition = PT_DUST;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * tmp: dir: 0 = up, 1 = right, 2 = down, 3 = left
	 * tmp2: state (0 or 1)
	 * life: is tunneling? 1 = yes, 0 = no
	 */

	// Instantly die if risk of creating BHOL
	if (sim->pmap_count[y][x] >= 5) {
		sim->kill_part(i);
		return 1;
	}

	int rx, ry, r;
	int dir = parts[i].tmp;
	int state = parts[i].tmp2;
	int soil_count = 0;

	// Normal checking
	for (rx = -1; rx <= 1; ++rx)
		for (ry = -1; ry <= 1; ++ry)
			if (BOUNDS_CHECK && (rx || ry)) {
				r = pmap[y + ry][x + rx];
				if (!r) continue;

				if (TYP(r) == PT_SOIL && (rx == 0 || ry == 0)) {
					++soil_count;

					// Set dig dir
					if (parts[i].life == 0) {
						if (rx == -1) parts[i].tmp = 3;
						else if (rx == 1) parts[i].tmp = 1;
						else if (ry == -1) parts[i].tmp = 0;
						else parts[i].tmp = 2;
					}
				}

				// Eat sugar, HONY and SEED
				if (TYP(r) == PT_SUGR || TYP(r) == PT_SEED || TYP(r) == PT_SWTR)
					sim->kill_part(ID(r));
				// Drown
				else if (sim->elements[TYP(r)].Properties & TYPE_LIQUID) {
					sim->kill_part(i);
					return 1;
				}
				
				// Transform nearby soil particles to tunnel walls
				if (TYP(r) == PT_SOIL && parts[ID(r)].tmp2 == 0) {
					parts[ID(r)].tmp2 = 1;
				}
			}

	// Is tunneling if surrounded by soil
	parts[i].life = soil_count >= 1 ? 1 : 0;

	// Stop here if not tunneling
	if (parts[i].life == 0)
		return 0;

	// Tunneling ants don't move, directly update pos
	parts[i].vx = parts[i].vy = 0;

	if (dir == 0) rx = 0, ry = -1;
	else if (dir == 1) rx = 1, ry = 0;
	else if (dir == 2) rx = 0, ry = 1;
	else if (dir == 3) rx = -1, ry = 0;

	// 8 directions, messy
	// if (dir == 0) rx = 0, ry = -1;
	// else if (dir == 1) rx = 1, ry = -1;
	// else if (dir == 2) rx = 1, ry = 0;
	// else if (dir == 3) rx = 1, ry = 1; // Bottom-down
	// else if (dir == 4) rx = 0, ry = 1;
	// else if (dir == 5) rx = -1, ry = 1;
	// else if (dir == 6) rx = -1, ry = 0;
	// else if (dir == 7) rx = -1, ry = -1;

	// Tunnel logic
	if (BOUNDS_CHECK) {
		r = pmap[y + ry][x + rx];
		if (!r) { // Stop doing tunneling if leaving soil
			parts[i].life = 0;
			return 0;
		}
		if (TYP(r) != PT_SOIL || sim->pmap_count[y + ry][x + rx] >= 3) { // Turn 180 if into a wall or will cause excessive stacking
			parts[i].tmp = (parts[i].tmp + 2) % 4;
			return 0;
		}

		parts[i].vx = rx;
		parts[i].vy = ry;

		// Turmite logic, what tmp to toggle depends on self state
		int orgtmp = parts[ID(r)].tmp2;
		if (!state) { // Turn randomly left or right
			parts[ID(r)].tmp2 = 2;
			parts[i].tmp = (parts[i].tmp + (RNG::Ref().chance(1, 2) ? 3 : 5)) % 4;
		} else { // Continue straight
			parts[ID(r)].tmp2 = 1;
		}

		// New state depends on original tile value
		parts[i].tmp2 = orgtmp == 1 ? 0 : 1;
	}

	return 0;
}
