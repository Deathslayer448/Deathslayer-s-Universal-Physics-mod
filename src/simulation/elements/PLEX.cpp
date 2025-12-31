#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_PLEX()
{
	Identifier = "DEFAULT_PT_PLEX";
	Name = "C-4";
	Colour = 0xB58B1D_rgb; // Brown/beige like real C4
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
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
	Meltable = 50;
	Hardness = 1;
	PhotonReflectWavelengths = 0x1F00003E;

	Weight = 100;

	HeatConduct = 88;
	Description = "Primary plastic explosive, works best with another explosive.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static int update(UPDATE_FUNC_ARGS)
{
	int r, rx, ry;
	for (rx = -1; rx < 2; rx++)
		for (ry = -1; ry < 2; ry++)
			if (x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES && (rx || ry))
			{
				r = pmap[y + ry][x + rx];
				if (!r)
					continue;
				// Spark detection
				if (TYP(r) == PT_SPRK) {
					sim->pv[y / CELL][x / CELL] += 10;
					sim->vx[(y) / CELL][(x) / CELL] *= 1.2;
					sim->vy[(y) / CELL][(x) / CELL] *= 1.2;

					parts[i].life += 500;
					parts[i].temp += 500;
					sim->part_change_type(i, x, y, PT_EMBR);
					return 1;
				}
				else if (TYP(r) == PT_EMBR || (TYP(r) == PT_FIRE && parts[i].temp > 800.0f - sim->pv[y / CELL][x / CELL] && sim->rng.chance(1, 20))) {
					sim->pv[y / CELL][x / CELL] += 20;
					sim->vx[(y) / CELL][(x) / CELL] *= 1.1;
					sim->vy[(y) / CELL][(x) / CELL] *= 1.1;
					parts[i].temp += 500;
					parts[i].life += 100;
					sim->part_change_type(i, x, y, sim->rng.chance(1, 5) ? PT_EMBR : PT_FIRE);
					return 1;
				}
			}
	if (parts[i].temp > 2000.0f - sim->pv[y / CELL][x / CELL]) {
		sim->pv[y / CELL][x / CELL] += 5;
		sim->vx[(y) / CELL][(x) / CELL] *= 1.1;
		sim->vy[(y) / CELL][(x) / CELL] *= 1.1;
		parts[i].temp += 400;
		parts[i].life += 100;

		sim->part_change_type(i, x, y, sim->rng.chance(1, 5) ? PT_EMBR : PT_FIRE);
		return 1;
	}
	if (parts[i].temp > 320.0f - sim->pv[y / CELL][x / CELL])
	{
		parts[i].vx += restrict_flt(parts[i].temp / 1000.0f * sim->vx[y / CELL][x / CELL], 0.0f, 0.7f);
		parts[i].vy += restrict_flt(parts[i].temp / 1000.0f * sim->vy[y / CELL][x / CELL], 0.0f, 0.7f);
	}
	if (sim->pv[y / CELL][x / CELL] - parts[i].temp / 10.0f > 150.0f)
	{
		sim->pv[y / CELL][x / CELL] += 5;
		sim->vx[(y) / CELL][(x) / CELL] *= 1.5;
		sim->vy[(y) / CELL][(x) / CELL] *= 1.5;
		parts[i].temp += 500;
		parts[i].life += 100;
		sim->part_change_type(i, x, y, sim->rng.chance(1, 5) ? PT_EMBR : PT_FIRE);
		return 1;
	}
	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	sim->parts[i].tmp4 = 100;
	sim->parts[i].tmpville[6] = sim->rng.between(0, 6);
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	int z = (cpart->tmpville[6] - 5) * 10; // Speckles!
	*colr += z;
	*colg += z;
	*colb += z;

	return 0;
}
