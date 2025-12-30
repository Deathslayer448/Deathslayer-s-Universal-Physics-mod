#include "simulation/ElementCommon.h"
#include "../ModTools.h"

static int update(UPDATE_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);


void Element::Element_GAS()
{
	Identifier = "DEFAULT_PT_GAS";
	Name = "GAS";
	Colour = 0xE0FF20_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 3.0f;
	HotAir = 0.001f	* CFDS;
	Falldown = 0;

	Flammable = 60;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 2.0f + 273.15f;
	HeatConduct = 42;
	Description = "Diffuses quickly and is flammable. Liquefies into OIL under pressure.";

	Properties = TYPE_GAS | PROP_NEUTPASS;

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
}
// RNG::Ref().between(20, 61)
static int update(UPDATE_FUNC_ARGS) {
	//carbons=num carbons
	//tmp=num tmp3
	//tmp3=location of double/triple bond if alkene/alkyne (tmp<2*carbons+2)
	// 
	// 
	//Cyens toy
	//Condensation
	
	if (parts[i].capacity == 0 && parts[i].tmp4 == 0)
	{
		parts[i].tmp4 = 100;
		parts[i].capacity = 400;
		//Cyens toy
	//Spawns with carbons (1-4)
		parts[i].carbons = RNG::Ref().between(1, 4);
		if (parts[i].carbons == 1) { //Creation of methane, can only be CH4 as a pure hydrocarbon
			parts[i].co2 = 4;
			parts[i].tmp3 = 0;
		}
		else { //Creating any other type of hydrocarbon
			parts[i].co2 = makeAlk(parts[i].carbons);
			if (parts[i].co2 < 2 * parts[i].carbons + 2)parts[i].tmp3 = getBondLoc(parts[i].carbons);
		}
		parts[i].life = parts[i].carbons + parts[i].co2 * 5;
	}
	int t = parts[i].temp - sim->pv[y / CELL][x / CELL];	//Pressure affects state transitions

	if (((parts[i].carbons <= 4 && t < -230 + parts[i].carbons * 50 + 273.15f) || (parts[i].carbons > 4 && t < (4 * sqrt(500 * (parts[i].carbons - 4))) + 273.15)) && RNG::Ref().chance(1, (int)restrict_flt(100 - (sim->pv[y / CELL][x / CELL] + parts[i].temp) * surround_space / 10, 1, MAX_TEMP)) && parts[i].tmpcity[3] <= 0)
	{
		//&& RNG::Ref().chance(1, (int)restrict_flt(1000 - (sim->pv[y / CELL][x / CELL] + parts[i].temp) * surround_space / 10, 1, MAX_TEMP)) 
		//|| (parts[i].carbons == 2 && t <= -100 + 273.15) || (parts[i].carbons == 3 && t <= -50 + 273.15


		if (parts[i].carbons < 8)//Low carbon condensation
			sim->part_change_type(i, x, y, PT_MWAX);
		else if (parts[i].carbons >= 8 && parts[i].carbons < 20) //Medium carbon condensation
			sim->part_change_type(i, x, y, PT_DESL);
		else //High carbon condensation
			sim->part_change_type(i, x, y, PT_OIL);

		parts[i].tmpcity[3] = RNG::Ref().between(100, 1000);
	}
	
	//Update
	if (!sim->betterburning_enable)
	{
		int r, rx, ry;
		for (rx = -1; rx < 2; rx++)
		{
			for (ry = -1; ry < 2; ry++)
			{
				if (BOUNDS_CHECK)
				{
					r = pmap[y + ry][x + rx];
					if (!r)
						continue;


					if ((parts[i].temp >= -10 * parts[i].carbons / 2 + 673.15f || TYP(r) == PT_FIRE || TYP(r) == PT_PLSM || TYP(r) == PT_LAVA) && RNG::Ref().chance(1, 36)) {

									
									sim->create_part(-3, x, y, PT_FIRE);
									sim->pv[y / CELL][x / CELL] = parts[i].carbons > 45 ? 5 : 50.0f - parts[i].carbons;

									sim->part_change_type(i, x, y, PT_CO2);
								}
				//		if (RNG::Ref().between(1, parts[i].carbons))sim->part_change_type(i, x, y, RNG::Ref().between(10, 3) ? PT_CO2 : PT_SMKE);
					}


				
			}
		}


	}



	//better burning by Silent the god

	else
	{


		int mintemp = (sim->elements[parts[i].type].Flammable * 6 + 273.15) - sim->pv[y / CELL][x / CELL];
					
		int oxyg = 0;
		for (int rx = -2; rx < 3; rx++)
			for (int ry = -2; ry < 3; ry++)
				if (BOUNDS_CHECK && (rx || ry))
				{
					int r = pmap[y + ry][x + rx];
					if (!r)
						continue;
					int rt = TYP(r);



					if (parts[ID(r)].oxygens > 0 && (parts[ID(r)].type == PT_O2 || parts[ID(r)].type == PT_LO2))
					{
						
						oxyg = r; 

					}

					if (((t == PT_FIRE || rt == PT_EMBR || rt == PT_PLSM || rt == PT_LIGH && parts[r].temp > mintemp / 2 ) || parts[i].temp > mintemp) && (surround_space > 0  || parts[ID(oxyg)].oxygens > 0) && parts[i].life > 0)
					{
						if (parts[ID(oxyg)].oxygens > 0)
						{

							
								surround_space += parts[ID(r)].oxygens / 5;
							parts[ID(oxyg)].oxygens--;

						}
						

							if (RNG::Ref().chance(1, (int)restrict_flt(200 - surround_space * 20, 1, MAX_TEMP)))
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
									parts[np].life += RNG::Ref().between(parts[i].life / 10, (int)restrict_flt(parts[i].life/ 4, parts[i].life / 10, parts[i].life));
									parts[np].ctype = parts[i].type;
									if (sim->pv[y / CELL][x / CELL] >= 0)
									{
										parts[np].temp += parts[i].temp + RNG::Ref().between(0, (int)restrict_flt(50 * surround_space + sim->pv[y / CELL][x / CELL], 1, MAX_TEMP));
										parts[i].life -= 1 + (sim->pv[y / CELL][x / CELL] + parts[i].temp) / 100;
									}
									else
									{
										parts[np].temp += parts[i].temp + RNG::Ref().between(0, (int)restrict_flt(50 * surround_space + sim->pv[y / CELL][x / CELL], 1, MAX_TEMP));
										parts[i].life -= 1;
									}
									sim->pv[y / CELL][x / CELL] += RNG::Ref().between(0, (int)restrict_flt(parts[i].temp * surround_space / 1200, 1, MAX_TEMP));
									}
									//	}
								
							
									}
									else if (RNG::Ref().chance(1, restrict_flt(20 - surround_space * 2, 1, MAX_TEMP)))
									{
										if (sim->pv[y / CELL][x / CELL] >= 0)
										{
											parts[i].temp += RNG::Ref().between(0, (int)restrict_flt(sim->pv[y / CELL][x / CELL] * surround_space, 1, MAX_TEMP));
											parts[i].life -= 1 + sim->pv[y / CELL][x / CELL] / 10;
										}
										else
										{
											 parts[i].temp += RNG::Ref().between((int)restrict_flt(sim->pv[y / CELL][x / CELL], O_MIN_TEMP, MAX_TEMP), 0);
											parts[i].life -= 1;
										}
										sim->pv[y / CELL][x / CELL] += RNG::Ref().between(0, (int)restrict_flt(parts[i].temp * surround_space / 1200, 1, parts[i].temp));
									}

					}







				}
		if (parts[i].life <= 0)
		{
			sim->part_change_type(i, x, y, PT_FIRE);
		}

	}


	if (parts[i].tmpcity[3] > 0)
		parts[i].tmpcity[3]--;

	return 0;
}
static void create(ELEMENT_CREATE_FUNC_ARGS) {
	//Spawns with carbons (1-4)
	sim->parts[i].carbons = RNG::Ref().between(1, 4);
	if (sim->parts[i].carbons == 1) { //Creation of methane, can only be CH4 as a pure hydrocarbon
		sim->parts[i].co2= 4;
		sim->parts[i].tmp3 = 0;
	}
	else { //Creating any other type of hydrocarbon
		sim->parts[i].co2= makeAlk(sim->parts[i].carbons);
		if (sim->parts[i].co2< 2 * sim->parts[i].carbons + 2)
		sim->parts[i].tmp3 = getBondLoc(sim->parts[i].carbons);
	}
	sim->parts[i].life = sim->parts[i].carbons + sim->parts[i].co2 * 5;
}