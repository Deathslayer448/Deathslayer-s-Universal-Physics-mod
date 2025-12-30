#include "simulation/ElementCommon.h"

static int graphics(GRAPHICS_FUNC_ARGS);
static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);


void Element::Element_SMKE()
{
	Identifier = "DEFAULT_PT_SMKE";
	Name = "SMKE";
	Colour = 0x222222_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.20f;
	Collision = 0.0f;
	Gravity = -0.1f;
	Diffusion = 0.75f;
	HotAir = 0.010f	* CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 120.0f + 273.15f;
	HeatConduct = 88;
	Description = "Smoke, created by combustion.";

	Properties = TYPE_GAS|PROP_LIFE_DEC;

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
	if (sim->betterburning_enable && parts[i].life <= 0)
	{
		switch (parts[i].ctype)
		{
		case PT_NONE:
			switch (RNG::Ref().between(2, 20))
			{
			case 7:
				if (RNG::Ref().chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_WTRV);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);

				}
				break;
			case 4:
				if (RNG::Ref().chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_CO2);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);

				}
				break;
			case 5:
				if (RNG::Ref().chance(1, 50))
				{
				sim->part_change_type(i, x, y, PT_GAS);
				parts[i].ctype = PT_NONE;
				parts[i].temp += RNG::Ref().between(0, 2);
				parts[i].life += RNG::Ref().between(5, 10);
				}
				break;
			case 6:
				if (RNG::Ref().chance(1, 50))
				{
					sim->part_change_type(i, x, y, PT_BCOL);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 50);
				}
				break;




			default:
				sim->kill_part(i);
				break;
			}



			break;









			//	case PT_POIL:
		case PT_WAX:
		case PT_MWAX:
		case PT_BCOL:
		case PT_DESL:
		case PT_COAL:
		case PT_GAS:
		case PT_OIL:
			//if (RNG::Ref().chance(1, 50))
			//{
			switch (RNG::Ref().between(2, 15))
			{

			case 8:
				if (RNG::Ref().chance(1, 20))
				{
					sim->part_change_type(i, x, y, PT_NBLE);
					parts[i].ctype = PT_GAS;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);
				
				}
				break;
			case 7:
				if (RNG::Ref().chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_WTRV);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);
					
				}
				break;
			case 4:
				if (RNG::Ref().chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_CO2);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);
				
				}
				break;
			case 5:
				if (RNG::Ref().chance(1, 20))
				{
					sim->part_change_type(i, x, y, PT_GAS);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);
					
				}
				break;
			case 6:
				if (RNG::Ref().chance(1, 10))
				{
					sim->part_change_type(i, x, y, PT_BCOL);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);
				
				}
				break;




			default:
				sim->kill_part(i);
				break;
			}


			//}
			//sim->kill_part(i);
		//	break;






		case PT_H2:
			if(RNG::Ref().chance(1, 2))
			{

				sim->part_change_type(i, x, y, PT_WTRV);
				parts[i].ctype = PT_NONE;
				parts[i].temp += RNG::Ref().between(0, 2);
				parts[i].life += RNG::Ref().between(5, 10);


			}



			break;
























		default:

			switch (RNG::Ref().between(2, 50))
			{
			case 7:
				if (RNG::Ref().chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_WTRV);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);

				}
				break;
			case 4:
				if (RNG::Ref().chance(1, 5))
				{
					sim->part_change_type(i, x, y, PT_CO2);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);

				}
				break;
			case 5:
				if (RNG::Ref().chance(1, 50))
				{
					sim->part_change_type(i, x, y, PT_GAS);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 10);
				}
				break;
			case 6:
				if (RNG::Ref().chance(1, 50))
				{
					sim->part_change_type(i, x, y, PT_BCOL);
					parts[i].ctype = PT_NONE;
					parts[i].temp += RNG::Ref().between(0, 2);
					parts[i].life += RNG::Ref().between(5, 50);
				}
				break;

			default:
				sim->kill_part(i);
				break;
			}


			break;

		}
	
				
	
	}
	else if (parts[i].life <= 0)
	{
		sim->kill_part(i);
	}
	return 0;
}
static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = RNG::Ref().between(20, 50);
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = 55;
	*colg = 55;
	*colb = 55;

	*firea = 75;
	*firer = 55;
	*fireg = 55;
	*fireb = 55;

	*pixel_mode = PMODE_NONE; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_BLEND;
	//Returning 1 means static, cache as we please
	return 1;
}
