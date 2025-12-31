#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

#define MAX_WEB_DISTANCE 15

void Element::Element_WEB() {
	Identifier = "DEFAULT_PT_WEB";
	Name = "WEB";
	Colour = PIXPACK(0x333333);
	MenuVisible = 0;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.05f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.0f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 0;

	Flammable = 20;
	Explosive = 0;
	Meltable = 0;
	Hardness = 12;

	Weight = 100;

	HeatConduct = 75;
	Description = "Spider web. Sticky, captures light powders and prey.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	DefaultProperties.tmp2 = -1;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 500.0f + 273.15f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	// Randomize color
	sim->parts[i].tmp = RNG::Ref().between(1, 4);
	if (RNG::Ref().chance(1, 300))
		sim->parts[i].tmp = 100;
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties:
	 * - ctype: fake trapped particle
	 * - tmp:  random color (1 - 4, if 100 glistens)
	 * - tmp2: ID of next connection
	 */

	bool touching = false;
	int particles_supporting = 0;

	// Break connection if too far or no longer exists
	if (parts[i].tmp2 >= 0) {
		if (!parts[parts[i].tmp2].type)
			parts[i].tmp2 = -1;
		else {
			int dx = parts[parts[i].tmp2].x - x;
			int dy = parts[parts[i].tmp2].y - y;
			if (dx * dx + dy * dy > MAX_WEB_DISTANCE * MAX_WEB_DISTANCE)
				parts[i].tmp2 = -1;
		}
	}

	// Ctype is fake web, doesn't actually stick
	if (parts[i].ctype)
		return 0;

	int rx, ry, r, rt;
	for (rx = -1; rx < 2; ++rx)
		for (ry = -1; ry < 2; ++ry)
			if (BOUNDS_CHECK) {
				r = pmap[y + ry][x + rx];
				if (!r) continue;
				rt = TYP(r);

				// Same space as another web or element
				if ((!(rx || ry) && rt == PT_WEB && ID(r) != i) || sim->pmap_count[y][x] > 3) {
					sim->kill_part(i);
					return 1;
				}

				// Connect to first web or solid it sees
				// If connecting to web must be a connected web
				if ((rt != PT_WEB && sim->sim->elements()[rt].Properties & TYPE_SOLID) || (rt == PT_WEB && parts[ID(r)].tmp2 >= 0)) {
					if (parts[i].tmp2 < 0) 
						parts[i].tmp2 = ID(r);
				}

				// Set touching property for decay
				if (sim->sim->elements()[rt].Properties & TYPE_SOLID)
					touching = true;

				bool is_alive = rt == PT_ANT || rt == PT_SPDR;

				// Stop particles if possible
				if (particles_supporting < 2 && 
					(sim->sim->elements()[rt].Properties & TYPE_PART ||
						sim->sim->elements()[rt].Properties & TYPE_LIQUID) &&
						sim->sim->elements()[rt].Weight < 50 && !is_alive) {
					++particles_supporting;
					parts[ID(r)].vx = 0;
					parts[ID(r)].vy = 0;
				}
				
				// Capture BIRD, ANT and BEE as WEB randomly
				if (RNG::Ref().chance(1, 100) && is_alive && rt != PT_SPDR) {
					sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y, PT_WEB);
					parts[ID(r)].ctype = rt;
					parts[ID(r)].vx = 0;
					parts[ID(r)].vy = 0;
				}
			}

	if (!touching && RNG::Ref().chance(1, 10)) {
		if (parts[i].ctype)
			sim->part_change_type(i, x, y, parts[i].ctype);
		else
			sim->kill_part(i);
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	// Mimic ctype color
	bool valid_ctype = cpart->ctype > 0 && cpart->ctype < PT_NUM && ren->sim->sim->elements()[cpart->ctype].Enabled;
	if (valid_ctype) {
		*colr = PIXR(ren->sim->sim->elements()[cpart->ctype].Colour);
		*colg = PIXG(ren->sim->sim->elements()[cpart->ctype].Colour);
		*colb = PIXB(ren->sim->sim->elements()[cpart->ctype].Colour);
		return 0;
	}
	
	*cola = 10;
	if (cpart->tmp == 100) { // The one glistening piece of web you see
		*colr = *colg = *colb = 155;
		*pixel_mode |= PMODE_SPARK;
	} else {
		*colr *= cpart->tmp / 3.0f;
		*colg *= cpart->tmp / 3.0f;
		*colb *= cpart->tmp / 3.0f;
	}

	// Connect web strands if far apart
	int ox = ren->sim->parts[cpart->tmp2].x;
	int oy = ren->sim->parts[cpart->tmp2].y;
	if (cpart->tmp2 >= 0 && (abs(cpart->x - ox) > 1 || abs(cpart->y - oy) > 1)) {
		ren->draw_line(cpart->x, cpart->y, ox, oy,
			*colr, *colg, *colb, 255);
	}
	return 0;
}
