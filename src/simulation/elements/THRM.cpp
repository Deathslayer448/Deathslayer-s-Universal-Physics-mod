#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static void changeType(ELEMENT_CHANGETYPE_FUNC_ARGS);

void Element::Element_THRM()
{
	Identifier = "DEFAULT_PT_THRM";
	Name = "THRM";
	Colour = 0xA08090_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
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

	Flammable = 0;
	Explosive = 0;
	Meltable = 2; // Thermite can melt normally
	Hardness = 2;

	Weight = 90;

	HeatConduct = 211;
	Description = "Thermite. Burns into extremely hot molten metal.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1838.15f; // ~1565°C - iron oxide melting point (thermite is a mixture, doesn't fully melt until iron oxide melts)
	HighTemperatureTransition = PT_LAVA;
	
	DefaultProperties.quantity = 100; // Quantity - starts at 100, consumed as it burns
	
	Update = &update;
	ChangeType = &changeType;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Real thermite ignites at around 1600°C (1873K)
	// Can be ignited by sparks or high temperature
	const float ignitionTemp = 1873.15f; // ~1600°C - ignition threshold
	
	// Initialize quantity if not set
	if (parts[i].quantity <= 0)
		parts[i].quantity = 100;
	
	// Check for sparks nearby (like C4 does)
	bool ignited = false;
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				if (rt == PT_SPRK)
				{
					ignited = true;
					break;
				}
			}
		}
		if (ignited)
			break;
	}
	
	// Also ignites at high temperature
	if (!ignited && parts[i].temp >= ignitionTemp)
		ignited = true;
	
	if (ignited && parts[i].quantity > 0)
	{
		// Thermite is burning - consume quantity and continuously generate heat
		parts[i].quantity--;
		// Real thermite burns at 2000-2500°C (2273-2773K)
		// Generate heat at realistic rate: ~15°C per frame = ~900°C per second at 60fps
		// This allows it to reach and maintain burning temperature while heat conducts away
		const float heatGeneration = 15.0f; // Heat generated per frame while burning
		const float targetBurningTemp = 2473.15f; // ~2200°C - typical thermite burning temperature
		
		// Generate heat, but don't exceed realistic maximum
		if (parts[i].temp < targetBurningTemp)
		{
			parts[i].temp = restrict_flt(parts[i].temp + heatGeneration, MIN_TEMP, targetBurningTemp);
		}
		else
		{
			// At or above burning temp, generate just enough to maintain it (accounting for heat loss)
			parts[i].temp = restrict_flt(parts[i].temp + heatGeneration * 0.5f, MIN_TEMP, MAX_TEMP);
		}
		
		// Spawn sparks periodically while burning (like C4 does)
		if (sim->rng.chance(2, 3))
		{
			int nb = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_EMBR);
			if (nb != -1) {
				parts[nb].tmp = 0;
				parts[nb].life = 30;
				parts[nb].vx = float(sim->rng.between(-10, 10));
				parts[nb].vy = float(sim->rng.between(-10, 10));
				parts[nb].temp = restrict_flt(parts[i].temp - 273.15f + 400.0f, MIN_TEMP, MAX_TEMP);
			}
		}
		else
		{
			sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_FIRE);
		}
		
		// When quantity runs out, convert to regular lava
		if (parts[i].quantity <= 0)
		{
			sim->part_change_type(i, x, y, PT_LAVA);
			parts[i].ctype = PT_BMTL; // Regular molten metal
			parts[i].temp = restrict_flt(parts[i].temp, MIN_TEMP, MAX_TEMP);
		}
	}
	
	return 0;
}

static void changeType(ELEMENT_CHANGETYPE_FUNC_ARGS)
{
	// When THRM melts to LAVA, preserve quantity and set ctype so we can track it
	if (from == PT_THRM && to == PT_LAVA)
	{
		sim->parts[i].ctype = PT_THRM;
		// quantity is already preserved since it's part of the Particle struct
	}
}
