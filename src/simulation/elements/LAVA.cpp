#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_LAVA()
{
	Identifier = "DEFAULT_PT_LAVA";
	Name = "LAVA";
	Colour = 0xE05010_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.0003f	* CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 2;
	PhotonReflectWavelengths = 0x3FF00000;

	Weight = 45;

	DefaultProperties.temp = R_TEMP + 1500.0f + 273.15f;
	HeatConduct = 60;
	Description = "Molten lava. Ignites flammable materials. Generated when metals and other materials melt, solidifies when cold.";

	Properties = TYPE_LIQUID|PROP_LIFE_DEC;
	CarriesTypeIn = 1U << FIELD_CTYPE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = MAX_TEMP;// check for lava solidification at all temperatures
	LowTemperatureTransition = ST;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = cpart->life * 2 + 0xE0;
	*colg = cpart->life * 1 + 0x50;
	*colb = cpart->life / 2 + 0x10;
	if (*colr>255) *colr = 255;
	if (*colg>192) *colg = 192;
	if (*colb>128) *colb = 128;
	*firea = 40;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD;
	*pixel_mode |= PMODE_BLUR;
	//Returning 0 means dynamic, do not cache
	return 0;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &elements = sim->elements();
	
	// Handle molten thermite - call THRM's update function
	// This allows molten THRM to continue burning and transfer heat
	if (parts[i].ctype == PT_THRM && elements[PT_THRM].Update)
	{
		elements[PT_THRM].Update(sim, i, x, y, surround_space, nt, parts, pmap);
	}
	
	float pres = sim->pv[y / CELL][x / CELL];
	if (parts[i].ctype == PT_ROCK)
	{			
		if (pres <= -9)
		{
			parts[i].ctype = PT_STNE;
			return 0;
		}

		if (pres >= 25 && sim->rng.chance(1, 12500))
		{
			if (pres <= 50)
			{
				if (sim->rng.chance(1, 2))
					parts[i].ctype = PT_BRMT;
				else
					parts[i].ctype = PT_CNCT;
			}
			else if (pres <= 75)
			{
				if (pres >= 73 || sim->rng.chance(1, 8))
					parts[i].ctype = PT_GOLD;
				else
					parts[i].ctype = PT_QRTZ;
			}
			else if (pres <= 100 && parts[i].temp >= 5000)
			{
				if (sim->rng.chance(1, 5)) // 1 in 5 chance IRON to TTAN
					parts[i].ctype = PT_TTAN;
				else
					parts[i].ctype = PT_IRON;
			}
			else if (parts[i].temp >= 5000 && sim->rng.chance(1, 5))
			{
				if (sim->rng.chance(1, 5))
					parts[i].ctype = PT_URAN;
				else if (sim->rng.chance(1, 5))
					parts[i].ctype = PT_PLUT;
				else
					parts[i].ctype = PT_TUNG;
			}
		}
	}
	else if ((parts[i].ctype == PT_STNE || !parts[i].ctype) && pres >= 30.0f && (parts[i].temp > elements[PT_ROCK].HighTemperature || pres < elements[PT_ROCK].HighPressure)) // Form ROCK with pressure, if it will stay molten or not immediately break
	{
		parts[i].tmp2 = sim->rng.between(0, 10); // Provide tmp2 for color noise
		parts[i].ctype = PT_ROCK;
	}
	
	// LAVA interactions with nearby particles
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);

				if ((rt==PT_COAL) || (rt==PT_BCOL))
				{
					if (parts[i].ctype == PT_IRON && sim->rng.chance(1, 500))
					{
						parts[i].ctype = PT_METL;
						sim->kill_part(ID(r));
						continue;
					}
					if ((parts[i].ctype == PT_STNE || parts[i].ctype == PT_NONE) && sim->rng.chance(1, 60))
					{
						parts[i].ctype = PT_SLCN;
						sim->kill_part(ID(r));
						continue;
					}
				}

				// LAVA(CLST) + LAVA(PQRT) + high enough temp = LAVA(CRMC) + LAVA(CRMC)
				if (parts[i].ctype == PT_QRTZ && rt == PT_LAVA && parts[ID(r)].ctype == PT_CLST)
				{
					float pres = std::max(sim->pv[y/CELL][x/CELL]*10.0f, 0.0f);
					if (parts[i].temp >= pres+elements[PT_CRMC].HighTemperature+50.0f)
					{
						parts[i].ctype = PT_CRMC;
						parts[ID(r)].ctype = PT_CRMC;
					}
				}
				else if (rt == PT_O2 && parts[i].ctype == PT_SLCN)
				{
					switch (sim->rng.between(0, 2))
					{
					case 0:
						parts[i].ctype = PT_SAND;
						break;

					case 1:
						parts[i].ctype = PT_CLST;
						// avoid creating CRMC.
						if (parts[i].temp >= elements[PT_PQRT].HighTemperature * 3)
						{
							parts[i].ctype = PT_PQRT;
						}
						break;

					case 2:
						parts[i].ctype = PT_STNE;
						break;
					}
					parts[i].tmp = 0;
					sim->kill_part(ID(r));
					continue;
				}
				else if (rt == PT_LAVA && (parts[ID(r)].ctype == PT_METL || parts[ID(r)].ctype == PT_BMTL) && parts[i].ctype == PT_SLCN)
				{
					parts[i].tmp = 0;
					parts[i].ctype = PT_NSCN;
					parts[ID(r)].ctype = PT_PSCN;
				}
				else if (rt == PT_HEAC && parts[i].ctype == PT_HEAC)
				{
					if (parts[ID(r)].temp > elements[PT_HEAC].HighTemperature)
					{
						sim->part_change_type(ID(r), x+rx, y+ry, PT_LAVA);
						parts[ID(r)].ctype = PT_HEAC;
					}
				}
				else if (parts[i].ctype == PT_ROCK && rt == PT_LAVA && parts[ID(r)].ctype == PT_GOLD && parts[ID(r)].tmp == 0 &&
					sim->pv[y / CELL][x / CELL] >= 50 && sim->rng.chance(1, 10000)) // Produce GOLD veins/clusters
				{
					parts[i].ctype = PT_GOLD;
					if (rx > 1 || rx < -1) // Trend veins vertical
						parts[i].tmp = 1;
				}
				else if (parts[i].ctype == PT_SALT && rt == PT_GLAS && parts[ID(r)].life < 234 * 120)
				{
					parts[ID(r)].life++;
				}
			}
		}
	}
	
	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.between(240, 359);
}
