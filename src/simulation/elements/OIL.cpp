#include "simulation/ElementCommon.h"
#include "../ModTools.h"

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);
static  int graphics(GRAPHICS_FUNC_ARGS);
void Element::Element_OIL()
{
	Identifier = "DEFAULT_PT_OIL";
	Name = "OIL";
	Colour = 0x404010_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.1;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 30;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 20;

	HeatConduct = 42;
	Description = "Flammable, turns into GAS at low pressure or high temperature.";

	Properties = TYPE_LIQUID | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;


	Update = &update;
	Create = &create;
	Graphics = &graphics;
}
static int update(UPDATE_FUNC_ARGS)
{
	
	
	if (parts[i].capacity == 0 && parts[i].tmp4 == 0)
	{
		parts[i].tmp4 = 100;
		parts[i].capacity = 400;


		//Cyens toy
		//Spawns with carbons (20-60), temp must be min 42 ?C 
		parts[i].carbons = RNG::Ref().between(20, 60);
		parts[i].co2 = makeAlk(parts[i].carbons);
		//parts[i].carbons = rand() % 41 + 20;
		if (parts[i].co2 < 2 * parts[i].carbons + 2)parts[i].tmp3 = getBondLoc(parts[i].carbons);
		parts[i].life = parts[i].carbons + parts[i].co2;
	}

	if (parts[i].tmp4 <= 0)
	{
		if (parts[i].oxygens > 0 || parts[i].carbons > 0 || parts[i].co2 > 0 || parts[i].water > 0 || parts[i].nitrogens > 0)
		{
				sim->part_change_type(i, x, y, PT_DUST);
				
		}
		else
			sim->kill_part(i);
		return 0;

	}

	
	//OIL is a high carbon liquid, it should not have any less than 20 carbons.
	if (parts[i].carbons < 20 && RNG::Ref().chance(1, 1000))sim->part_change_type(i, x, y, PT_DESL);

	int t = parts[i].temp - sim->pv[y / CELL][x / CELL];	//Pressure affects state transitions
	//Freezing into PRFN
	//if (t <= (14.3f * sqrt((parts[i].life - 12))) + 273.15)
	//	sim->part_change_type(i, x, y, PT_PRFN);
	//Boiling into GAS
	if (t > (4 * sqrt(500 * (parts[i].carbons - 4))) + 273.15 && RNG::Ref().chance(1, restrict_flt(100 - (sim->pv[y / CELL][x / CELL] + parts[i].temp) * surround_space / 10, 1, MAX_TEMP)) || RNG::Ref().chance(1, restrict_flt(2000 - surround_space * 10, 1, MAX_TEMP)))
		sim->part_change_type(i, x, y, PT_GAS);

	



	//better burning by Silent the god

	if (sim->betterburning_enable)
	{
		
		int mintemp = (sim->elements[parts[i].type].Flammable * 10+ 273.15) - sim->pv[y / CELL][x / CELL];

		for (int rx = -2; rx < 3; rx++)
			for (int ry = -2; ry < 3; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y + ry][x + rx];
					if (!r)
						continue;
					int rt = TYP(r);
					
						if ((rt == PT_FIRE || rt == PT_EMBR || rt == PT_PLSM || rt == PT_LIGH && parts[r].temp > mintemp / 2  || parts[i].temp > mintemp) && (surround_space > 0) && parts[i].life > 0)
						{

							
								if (RNG::Ref().chance(1, restrict_flt(100 - surround_space * 4, 1, MAX_TEMP)))
								{
						


									float angle, magnitude;
									int n, np;

									//for (n = 0; n < 2; n++)
								////	{
									np = sim->create_part(-3, x, y, PT_FIRE);
									if (np > -1)
									{
										magnitude = RNG::Ref().between(40, 99) * 0.05f * surround_space;
										angle = RNG::Ref().between(0, 6283) * 0.001f; //(in radians, between 0 and 2*pi)
										parts[np].vx = parts[i].vx + cosf(angle) * magnitude;
										parts[np].vy = parts[i].vy + sinf(angle) * magnitude;
										//	parts[np].tmp2 = RNG::Ref().between(70, 109);
										parts[np].life += RNG::Ref().between(parts[i].life / 10, (int)restrict_flt(parts[i].life / 4, parts[i].life / 10, parts[i].life));
										parts[np].ctype = parts[i].type;
									
										if (sim->pv[y / CELL][x / CELL] >= 0)
										{
											parts[np].temp += parts[i].temp + RNG::Ref().between(0, (int)restrict_flt(5 * surround_space + sim->pv[y / CELL][x / CELL], 1, MAX_TEMP));
											parts[i].life -= 1 * surround_space + sim->pv[y / CELL][x / CELL] / 100;
										}
										else
										{
											parts[np].temp += parts[i].temp + RNG::Ref().between(0, (int)restrict_flt(5 * surround_space + sim->pv[y / CELL][x / CELL], 1, MAX_TEMP));
											parts[i].life -= 1 * surround_space;
						
										
										
										}
										sim->pv[y / CELL][x / CELL] += RNG::Ref().between(0, (int)restrict_flt(parts[i].temp * surround_space / 2000, 0, MAX_TEMP));;
									}
									//	}
									
								}
								else if (RNG::Ref().chance(1, restrict_flt(20 - surround_space * 2, 1, MAX_TEMP)))
								{
							
									if (sim->pv[y / CELL][x / CELL] >= 0)
									{
										parts[i].temp += RNG::Ref().between(0, (int)restrict_flt(sim->pv[y / CELL][x / CELL] * surround_space, 1, MAX_TEMP));
										parts[i].life -= 1 * surround_space * 2 + sim->pv[y / CELL][x / CELL] / 10 + parts[i].temp / 100;
									}
									else
									{
										parts[i].temp += RNG::Ref().between((int)restrict_flt(sim->pv[y / CELL][x / CELL], O_MIN_TEMP, MAX_TEMP), 0);
										parts[i].life -= 1;
									}
									sim->pv[y / CELL][x / CELL] +=  RNG::Ref().between(0, (int)restrict_flt(parts[i].temp * surround_space / 2000, 1, MAX_TEMP));
								}
							


						}






					
				}
		if (parts[i].life <= 0)
		{
			sim->part_change_type(i, x, y, PT_FIRE);
		}
	
				}
		
	

	return 0;
}

static  void create(ELEMENT_CREATE_FUNC_ARGS)
{


	//Cyens toy
	//Spawns with carbons (20-60), temp must be min 42 ?C
	sim->parts[i].carbons = RNG::Ref().between(20, 60);
	sim->parts[i].co2 = makeAlk(sim->parts[i].carbons);
	//sim->parts[i].carbons = rand() % 41 + 20;
	if (sim->parts[i].co2 < 2 * sim->parts[i].carbons + 2)sim->parts[i].tmp3 = getBondLoc(sim->parts[i].carbons);
	//Essentially this is creating PRFN then melting it right away
	//sim->parts[i].temp = 14.3f * sqrt(sim->parts[i].carbons - 12) + 273.15f;
	sim->parts[i].life = sim->parts[i].carbons + sim->parts[i].co2;
}




//cyens toy hydrocarbons port
//type, carbons, co2,   tmp3,				ctype
//type,  life,     tmp,       tmp2,            ctype 


static int graphics(GRAPHICS_FUNC_ARGS)
{
//	 *colr += (int)restrict_flt((cpart->carbons) / 10, 0, 255);
	// *colb += (int)restrict_flt((cpart->co2) / 10, 0, 255);
//	*firea = *cola;
//	*firer = *colr;
//	*fireg = *colb;
//	*fireb = *colg;
	//*pixel_mode |= EFFECT_DBGLINES;
//	*pixel_mode |= EFFECT_GRAVIN;
//	*pixel_mode &= ~PMODE;
//	*pixel_mode |= PMODE_ADD;
	return 1;
}
