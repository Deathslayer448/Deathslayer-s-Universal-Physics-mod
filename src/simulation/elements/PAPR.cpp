#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PAPR() {
	Identifier = "DEFAULT_PT_PAPR";
	Name = "PAPR";
	Colour = PIXPACK(0xFAFAFA);
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
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 50;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Mass = 100/1000.0f;

	HeatConduct = 40;
	Description = "Paper. Can be stained, dissolves in water, burns quickly.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH; // 451 F
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties:
	 * - tmp3:    how much water it has
	 * - dcolour: current stained color
	 */

	// Chance to dissolve if too wet
	if (parts[i].tmp3 > 40 && RNG::Ref().chance(1, 100)) {
		parts[i].dcolour = 0;
		sim->part_change_type(i, x, y, PT_PULP);
		return 1;
	}

	int tmp3_dec_bonus = (int)((parts[i].temp - 273.15f) / 5.0f);
	if(RNG::Ref().chance(1, 10000 - parts[i].temp) && sim->pmap_count[y][x] < 3)
	{
		parts[i].tmp3 -= tmp3_dec_bonus;
		if (tmp3_dec_bonus > 10)
		{

			sim->create_part(-3, x, y, PT_WATR);
		//	if (np > -1)
			//{
			//	int magnitude = RNG::Ref().between(40, 99) * 0.05f * surround_space;
			//	int angle = RNG::Ref().between(0, 6283) * 0.001f; //(in radians, between 0 and 2*pi)
			//	parts[np].vx = parts[i].vx + cosf(angle) * magnitude;
			//	parts[np].vy = parts[i].vy + sinf(angle) * magnitude;
		//	}

		}

	}
	if (parts[i].tmp3 <= 0)
		parts[i].tmp3 = 0;

	int rx, ry, r, rt;
	int ri = PIXR(parts[i].dcolour), gi = PIXG(parts[i].dcolour), bi = PIXB(parts[i].dcolour);

	for (rx = -1; rx <= 1; ++rx)
	for (ry = -1; ry <= 1; ++ry)
		if (BOUNDS_CHECK && (rx || ry)) {
			r = pmap[y + ry][x + rx];
			if (!r) continue;
			rt = TYP(r);

			// Diffuse color to neighbours if self alpha is 255
			if (rt == PT_PAPR && parts[i].dcolour && parts[i].tmp3 > 800) {
				int alpha = (parts[i].dcolour & 0xFF000000) / 0x1000000;
				int other_alpha = (parts[ID(r)].dcolour & 0xFF000000) / 0x1000000;
				if (other_alpha < alpha && alpha > 100) {
					int ro = PIXR(parts[ID(r)].dcolour);
					int go = PIXG(parts[ID(r)].dcolour);
					int bo = PIXB(parts[ID(r)].dcolour);
					float diffuse_multi = std::max(std::min(0.9f, parts[i].tmp3 / 6000.0f), 0.3f);
					int nc = (int)(alpha * diffuse_multi * 0x1000000) + 0xFF00 * (ro + ri) / 2 + 0xFF * (go + gi) / 2 + (bo + bi) / 2;
					parts[ID(r)].dcolour = nc;
				}
			}

			bool is_water = rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW || rt == PT_SWTR || rt == PT_WTRV || rt == PT_H2O2;

			// Stain self
			if (rt != PT_PULP && (rx == 0 || ry == 0) && !is_water &&
					(sim->elements[rt].Properties & TYPE_LIQUID || sim->elements[rt].Properties & TYPE_PART)) {
				int color = sim->elements[rt].Colour;
				int ro = PIXR(color), go = PIXG(color), bo = PIXB(color);

				// Either self is not stained, or staining with same color
				if (parts[i].dcolour == 0 || (ro == ri && go == gi && bo == bi)) {
					color = PIXRGB(std::min((int)(ro * 1.5f), 255),
								   std::min((int)(go * 1.5f), 255),
								   std::min((int)(bo * 1.5f), 255));
					parts[i].dcolour = color + 0xFF000000;
				}
			}

			// Get "wetter" with water
			if (is_water && parts[i].tmp3 < 30 && RNG::Ref().chance(1, 50)) {
				parts[i].tmp3 += 10;
			//	sim->kill_part(ID(r));
				continue;
			}

			// Diffuse wetness
			if (RNG::Ref().chance(1, 20) && rt == PT_PAPR && parts[ID(r)].tmp3 < parts[i].tmp3) {
				int lose = RNG::Ref().between(1, parts[i].tmp3);
				parts[i].tmp3 -= lose;
				parts[ID(r)].tmp3 += lose;
			}
		}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	int darker = cpart->tmp3 / 200;
	if (darker > 50)
		darker = 50;
	*colr -= darker;
	*colg -= darker;
	*colb -= darker;

	return 0;
}
