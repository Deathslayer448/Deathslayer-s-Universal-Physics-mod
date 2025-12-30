#include "simulation/ElementCommon.h"

int Element_FLSH_update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_FLSH() {
	Identifier = "DEFAULT_PT_FLSH";
	Name = "FLSH";
	Colour = PIXPACK(0xF05B5B);
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
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

	Flammable = 1;
	Explosive = 0;
	Meltable = 0;
	Hardness = 15;

	Weight = 100;

	DefaultProperties.oxygens = 100;
	DefaultProperties.carbons = 100;
	DefaultProperties.water = 50;
	DefaultProperties.tmp3 = 100;
	DefaultProperties.tmp4 = 100;
	DefaultProperties.capacity = 800;
	DefaultProperties.tmpcity[3] = 100;
	DefaultProperties.tmpcity[9] = 0;
	DefaultProperties.metabolism = 50;

	HeatConduct = 54;
	Description = "Flesh. Can be cooked.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE | PROP_EDIBLE | PROP_ORGANISM | PROP_ANIMAL;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &Element_FLSH_update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	int gx = x / 50 * 50;
	int gy = y / 50 * 50;
	float theta = x + y;

	int newx = cos(theta) * (x - gx) - sin(theta) * (y - gy) + x;
	int newy = sin(theta) * (x - gx) + cos(theta) * (y - gy) + y;
	x = newx, y = newy;

	int thickness = (x / 4 + y / 4) % 5;
	if ((x + y) % 25 < thickness || abs(x - y) % 25 < thickness) {
		sim->parts[i].life = 1; // White part
	}
}

int Element_FLSH_update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties
	 * life:  Graphics
	 * oxygens:   Oxygen stored
	 * carbons:  Nutrients stored
	 * tmp2: Highest temperature
	 * co2: CO2
	 * pavg[0]: Type 0 = inside, 1 = skin, 2 = dead
	 * tmp3: health of part 0-100
	 * tmpcity[3]----- energy: energy stored for use
	 * tmpcity[8]:  != 0 breaks meat
	 * water: water needed for life
	 * lcapacity: lcapacity for stuff
	 * tmpcity[9]: age by updates I guess
	 * tmpville[2]? sweat?
	 * nitrogens: something in blood
	 */


if (parts[i].capacity == 0)
	{
			 
	int typec = parts[i].type;
	 if(parts[i].ctype != 0)
	 typec = parts[i].ctype;

		switch(typec)
		{
			case PT_FLSH:

			parts[i].oxygens = 100;
			parts[i].carbons = 100;
			parts[i].water = 50;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 800;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpcity[9] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;
			break;

			case PT_LUNG:
			parts[i].carbons = 50;
			parts[i].oxygens = 50;
			parts[i].co2 = 10;
			parts[i].water = 50;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 800;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpcity[9] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;
			break;
			
			case PT_STMH:
			parts[i].oxygens = 100;
			parts[i].carbons = 100;
			parts[i].co2 = 100;
			parts[i].water = 50;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 800;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpcity[9] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;
			break;

			case PT_POPS:
			parts[i].oxygens = 100;
			parts[i].carbons = 100;
			parts[i].co2 = 20;
			parts[i].water = 50;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 800;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpcity[9] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;

			break;

			case PT_UDDR:
			parts[i].oxygens = 100;
			parts[i].carbons = 100;
			parts[i].co2 = 20;
			parts[i].water = 50;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 800;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpcity[9] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;
			break;

			case PT_BVSL:
			parts[i].oxygens = 100;
			parts[i].carbons = 100;
			parts[i].co2 = 20;
			parts[i].water = 50;
			parts[i].tmp2 = 2;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 1000;
			parts[i].tmpcity[9] = 0;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpville[3] = 0;
			parts[i].tmpville[4] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;
			break;

			default:
			parts[i].oxygens = 100;
			parts[i].carbons = 100;
			parts[i].water = 50;
			parts[i].tmp3 = 100;
			parts[i].tmp4 = 100;
			parts[i].capacity = 800;
			parts[i].tmpcity[3] = 100;
			parts[i].tmpcity[9] = 0;
			parts[i].metabolism = 50;
			parts[i].pavg[0] = 0;
			break;

			

		}
	}


	int r = 0;
	int lcapacity = 0;
	if (parts[i].tmp3 <= 0 && parts[i].pavg[0] != 2)
	{
		parts[i].pavg[0] = 2;
		parts[i].tmp3 = 0;
	}


	if (parts[i].temp - sim->pv[y / CELL][x / CELL] < 258.15f && RNG::Ref().chance(1, restrict_flt(parts[i].temp - sim->pv[y / CELL][x / CELL], 2, 100 + 273.15f))) {
		parts[i].tmp = parts[i].ctype;
		parts[i].ctype = parts[i].type;
    	sim->part_change_type(i, x, y, PT_ICEI);
		
		parts[i].dcolour = sim->elements[parts[i].ctype].Colour + 0x77000000;
	return 1;
	}

	if (parts[i].pavg[0] != 2 && sim->timer % (int)restrict_flt(parts[i].metabolism, 1, MAX_TEMP) == 0)
	{

			if (parts[i].oxygens > 0 && parts[i].carbons > 0 && parts[i].nitrogens < 70 && parts[i].tmpcity[3] < 100-1)
			{
				parts[i].oxygens--;

				parts[i].carbons--;


				parts[i].co2++;
				parts[i].tmpcity[3]+=2;

				if (RNG::Ref().chance(1, 10))
				
					parts[i].temp++;



				//temporary healing
					if (RNG::Ref().chance(1, 1000) && parts[i].tmp3 < 100)
					
						parts[i].tmp3++;
						
					
			}
			else if(parts[i].tmpcity[3] > 0 && parts[i].water > 0)
			{
				parts[i].tmpcity[3]--;
				
				if (RNG::Ref().chance(1, 10))
					parts[i].temp++;
				if (RNG::Ref().chance(1, 20))
				{
					
					parts[i].water--;
					parts[i].nitrogens++;
				}
			}
			else
			{
				parts[i].tmp3--;
			}
			
		
	}
	

	// The randomization is to avoid easily burning meat and creatingwha
	// a more realistic crystallization effect
	if (parts[i].temp > 110.0f + 273.15f && RNG::Ref().chance(1, 100))
		parts[i].tmp2 = parts[i].temp;
	if (parts[i].temp > parts[i].tmp2 && (parts[i].temp < 110.0f + 273.15f ||
			parts[i].temp > 150.0f + 273.15f))
		parts[i].tmp2 = parts[i].temp;
	
	// Rot if dead and not cooked or frozen
	if (parts[i].temp > 3.0f + 273.15f && parts[i].tmp2 < 120.0f + 273.15f && parts[i].pavg[0] == 2
			&& RNG::Ref().chance(1, 10000 / (surround_space + 1))) {
		if (RNG::Ref().chance(1, 20))
		{
			int bctr = sim->create_part(-3, x, y, PT_BCTR);
			parts[bctr].ctype = RNG::Ref().chance(1, 8) ? (parts[i].ctype ^ (1 << RNG::Ref().between(0, 32))) : (512 ^ (1 << RNG::Ref().between(0, 32))); // Meat eating gene
			parts[bctr].tmp = 0;
			parts[bctr].tmp2 = 0;
			parts[bctr].tmp3 = 420;
			parts[bctr].carbons += std::min(5, parts[i].carbons);
			parts[bctr].oxygens += std::min(5, parts[i].oxygens);
			parts[bctr].water += std::min(5, parts[i].water);
			parts[i].carbons -= std::min(5, parts[i].carbons);
			parts[i].oxygens -= std::min(5, parts[i].oxygens);
			parts[i].water -= std::min(5, parts[i].water);
			parts[i].tmp4 -= 20;
			//parts[i].life = 110;
			return 1;
		}
		else if (parts[i].tmpcity[8] == 0)
			parts[i].tmpcity[8] = 1;
		
	}



	if (((fabs(sim->pv[y / CELL][x / CELL]) > 5.0f && RNG::Ref().chance(1, 300)) || parts[i].tmpcity[8] == 1) && parts[i].type != PT_BRKN) {
		parts[i].tmp = parts[i].ctype;
		parts[i].ctype = parts[i].type;
		sim->part_change_type(i, x, y, PT_BRKN);
		return 1;
	}
	else if(parts[i].tmpcity[8] == 2 && parts[i].type != PT_LQUD)
	{
	//	parts[i].tmp = parts[i].ctype;
		parts[i].ctype = parts[i].type;
		sim->part_change_type(i, x, y, PT_LQUD);
		return 1;
	}
	if (parts[i].pavg[0] != 2)
	{	
		//damages 
		
		// Death
	// Radiation burns
		int r = sim->photons[y][x];
	if (r) {

		if (parts[ID(r)].temp > 273.15f + 110.0f)
		{
			parts[i].tmp2 += 10;
			parts[i].tmp3 --;
	
		}
	}

		// Pressure
		if (fabs(sim->pv[y / CELL][x / CELL]) > 10.0f)
		{
			parts[i].tmp3 --;
			
		}
		// Temperature
		if ((parts[i].temp < 273.15f - 5.0f || parts[i].temp > 52.0f + 273.15f ) && RNG::Ref().chance(restrict_flt(parts[i].temp, 1, 1249.0f + 273.15f), 1250.0f + 273.15f))
			parts[i].tmp3--;
	 



	

//signal decay
	if (sim->timer % 500 == 0 && parts[i].tmpville[2] > 0)
		parts[i].tmpville[2]--;
	// if(sim->timer % 500 == 0 && parts[i].tmpville[14] > 0)
	// 	parts[i].tmpville[14]--;
	// if(sim->timer % 500 == 0 && parts[i].tmpville[15] > 0)
	// 	parts[i].tmpville[15]--;
	// if(sim->timer % 500 == 0 && parts[i].tmpville[16] > 0)
	// 	parts[i].tmpville[16]--;
	if(RNG::Ref().chance(1, 1000) && parts[i].tmpville[14] > 0)
		parts[i].tmpville[14]--;
	if(RNG::Ref().chance(1, 1000) && parts[i].tmpville[15] > 0)
		parts[i].tmpville[15]--;
	if(RNG::Ref().chance(1, 1000) && parts[i].tmpville[16] > 0)
		parts[i].tmpville[16]--;

	if (parts[i].temp > 42.0f + 273.15f && RNG::Ref().chance(restrict_flt(parts[i].temp, 1, 49.0f + 273.15f), 50.0f + 273.15f) && parts[i].tmpville[2] < 2)
		parts[i].tmpville[2]++;

	








	int rx, ry, rt;

	
	for (ry = -1; ry < 2; ry++)
	for (rx = -1; rx < 2; rx++)
		if (BOUNDS_CHECK && (rx || ry)) 
		{
			r = pmap[y + ry][x + rx];
			int partnum = 10;
			int tmpt, tmps = 0;
			if (!r) 
			{

				if (parts[i].tmpville[2] > 0 && parts[i].water > 40 && RNG::Ref().chance(1, 8000))
				{
					int sweat = RNG::Ref().between(20, 60);
					int ee = sim->create_part(-1, x + rx, y + ry, PT_WATR);
					parts[i].water -= 100;
					parts[ee].water += 100;
					parts[i].temp -= sweat;
					parts[ID(ee)].temp += sweat;
					parts[i].tmpville[2]--;

				}
				// Alive flesh
				if (parts[i].pavg[0] == 0 && RNG::Ref().chance(1, 1000) && parts[i].type == PT_FLSH) {
					// Create skin
					parts[i].pavg[0] = 1;
				}
				continue;
			}
			//if (!r)
		//		continue;
			rt = TYP(r);


			// Alive flesh
			

				if (sqrtf(parts[ID(r)].vx * parts[ID(r)].vx + parts[ID(r)].vy * parts[ID(r)].vy) > 16.0f)
				{
					if (sqrtf(parts[ID(r)].vx * parts[ID(r)].vx + parts[ID(r)].vy * parts[ID(r)].vy) > parts[i].tmp3 && RNG::Ref().chance(1, restrict_flt(20 - sqrtf(parts[ID(r)].vx * parts[ID(r)].vx + parts[ID(r)].vy * parts[ID(r)].vy), 1, MAX_TEMP)))
						parts[i].tmpcity[8] = 1;
					parts[i].tmp3 -= sqrtf(parts[ID(r)].vx * parts[ID(r)].vx + parts[ID(r)].vy * parts[ID(r)].vy);

				}

				//nerve endings
					
				// if (rt == PT_NRVE && parts[i].tmpville[3] == 0 && RNG::Ref().chance(1, 8))
				// 		parts[i].tmpville[3] = parts[ID(r)].tmpville[3] + 1;
				// if((rt == PT_FLSH || rt == PT_UDDR || rt == PT_LUNG || rt == PT_POPS || rt == PT_STMH || rt == PT_BRIN || rt == PT_NRVE) && parts[i].tmpville[12] == 0 )
				// 		parts[i].tmpville[3] = 1;
				


				// signals
				if (parts[i].type == PT_FLSH && rt == PT_FLSH && parts[i].tmpville[2] > 0 && parts[ID(r)].tmpville[2] < 3 && parts[i].tmpville[2] > parts[ID(r)].tmpville[2])
				{
					parts[ID(r)].tmpville[2]++;
					parts[i].tmpville[2]--;
				}
				//signals
				// if(parts[i].tmpville[14] > 0 && parts[ID(r)].tmpville[14] < 30 && RNG::Ref().chance(1, 8) && (rt == PT_POPS ||  rt == PT_NRVE || rt == PT_BRIN) && parts[i].type != PT_NRVE)
				// {
				// 	parts[i].tmpville[14]--;
				// 	parts[ID(r)].tmpville[14]++;
				// }
				// if(parts[i].tmpville[15] > 0 && parts[ID(r)].tmpville[15] < 30  && RNG::Ref().chance(1, 8)&& (rt == PT_POPS ||  rt == PT_NRVE || rt == PT_BRIN) && parts[i].type != PT_NRVE)
				// {
				// 	parts[i].tmpville[15]--;
				// 	parts[ID(r)].tmpville[15]++;
				// }
				// if(parts[i].tmpville[16] > 0 && parts[ID(r)].tmpville[16] < 30  && RNG::Ref().chance(1, 8) && (rt == PT_POPS ||  rt == PT_NRVE || rt == PT_BRIN) && parts[i].type != PT_NRVE)
				// {
				// 	parts[i].tmpville[16]--;
				// 	parts[ID(r)].tmpville[16]++;
				// }	



				if((rt == PT_POPS ||  rt == PT_BRIN) && (parts[i].type == PT_POPS ||  parts[i].type == PT_BRIN))
				{
						if (rt == PT_BRIN)
								partnum = 5;
						else 
								partnum = 2;
					//give
					if(RNG::Ref().chance(1, 2))
					{
						if(parts[i].tmpville[14] > 0 && parts[i].tmpville[14] > parts[ID(r)].tmpville[14]  * 1.5f && parts[ID(r)].tmpville[14] <  420 && !(parts[i].type != PT_POPS && rt == PT_POPS) && RNG::Ref().chance(1, 8))
						{
							parts[ID(r)].tmpville[14] += std::min(partnum, (int)parts[i].tmpville[14]);
							parts[i].tmpville[14] -= std::min(partnum, (int)parts[i].tmpville[14]);
						}
						if(parts[i].tmpville[15] > 0 && parts[i].tmpville[15] > parts[ID(r)].tmpville[15]  * 1.5f&& parts[ID(r)].tmpville[15] < 420   && RNG::Ref().chance(1, 8))
						{
							
							parts[ID(r)].tmpville[15] += std::min(partnum, (int)parts[i].tmpville[15]);
							parts[i].tmpville[15] -= std::min(partnum, (int)parts[i].tmpville[15]);
						}
						if(parts[i].tmpville[16] > 0 && parts[i].tmpville[16] > parts[ID(r)].tmpville[16] * 1.5f	  && parts[ID(r)].tmpville[16] < 420 && RNG::Ref().chance(1, 8))
						{
							parts[ID(r)].tmpville[16] += std::min(partnum, (int)parts[i].tmpville[16]);
							parts[i].tmpville[16] -= std::min(partnum, (int)parts[i].tmpville[16]);
						}	
					}
					else
					{
					//take
						if(parts[ID(r)].tmpville[14] > 0 && parts[ID(r)].tmpville[14] > parts[i].tmpville[14]  * 1.5f  && parts[i].tmpville[14] < 420  && RNG::Ref().chance(1, 8) && !(parts[i].type == PT_POPS && rt != PT_POPS))
						{
							
							parts[i].tmpville[14] += std::min(partnum, (int)parts[ID(r)].tmpville[14]);
							parts[ID(r)].tmpville[14] -= std::min(partnum, (int)parts[ID(r)].tmpville[14]);
						}
						if(parts[ID(r)].tmpville[15] > 0 && parts[ID(r)].tmpville[15] > parts[i].tmpville[15]  * 1.5f && parts[i].tmpville[15] < 420 &&  RNG::Ref().chance(1, 8) && !(parts[i].type != PT_POPS && rt == PT_POPS))
						{
							parts[i].tmpville[15] += std::min(partnum, (int)parts[ID(r)].tmpville[15]);
							parts[ID(r)].tmpville[15] -= std::min(partnum, (int)parts[ID(r)].tmpville[15]);
						}
						if(parts[ID(r)].tmpville[16] > 0 && parts[ID(r)].tmpville[16] > parts[i].tmpville[16]  * 1.5f && parts[i].tmpville[16] < 420 && RNG::Ref().chance(1, 8))
						{
							parts[i].tmpville[16] += std::min(partnum, (int)parts[ID(r)].tmpville[16]);
							parts[ID(r)].tmpville[16] = std::min(partnum, (int)parts[ID(r)].tmpville[16]);
						}	
					}
					
						
						
					
					if(RNG::Ref().chance(1, 2))
					{
						//give
						if(parts[i].tmpville[14] > 0 && parts[i].tmpville[14] > parts[ID(r)].tmpville[14] * 1.5f  && parts[ID(r)].tmpville[14] < 420  && !(parts[i].type != PT_POPS && rt == PT_POPS) && RNG::Ref().chance(1, 8))
						{
							tmps = parts[i].tmpville[14];
							parts[i].tmpville[14] = parts[ID(r)].tmpville[14];
							parts[ID(r)].tmpville[14]= tmps;
						}
						if(parts[i].tmpville[15] > 0 && parts[i].tmpville[15] > parts[ID(r)].tmpville[15] * 1.5f  && parts[ID(r)].tmpville[15] < 420   && RNG::Ref().chance(1, 8))
						{
							tmps = parts[i].tmpville[15];
							parts[i].tmpville[15] = parts[ID(r)].tmpville[15];
							parts[ID(r)].tmpville[15]= tmps;
						}if(parts[i].tmpville[16] > 0 && parts[i].tmpville[16] > parts[ID(r)].tmpville[16]	* 1.5f  && parts[ID(r)].tmpville[16] < 420  && RNG::Ref().chance(1, 8))
						{
							tmps = parts[i].tmpville[16];
							parts[i].tmpville[16] = parts[ID(r)].tmpville[16];
							parts[ID(r)].tmpville[16]= tmps;
						}	
					}
					else
					{
					//take
						if(parts[ID(r)].tmpville[14] > 0 && parts[ID(r)].tmpville[14] > parts[i].tmpville[14] * 1.5f  && parts[i].tmpville[14] < 420  && RNG::Ref().chance(1, 8) && !(parts[i].type == PT_POPS && rt != PT_POPS))
						{

							tmps = parts[ID(r)].tmpville[14];
							parts[ID(r)].tmpville[14] = parts[i].tmpville[14];
							parts[i].tmpville[14]= tmps;
						}
						if(parts[ID(r)].tmpville[15] > 0 && parts[ID(r)].tmpville[15] > parts[i].tmpville[15] * 1.5f  && parts[i].tmpville[15] < 420 &&  RNG::Ref().chance(1, 8) && !(parts[i].type != PT_POPS && rt == PT_POPS))
						{
							tmps = parts[ID(r)].tmpville[15];
							parts[ID(r)].tmpville[15] = parts[i].tmpville[15];
							parts[i].tmpville[15]= tmps;
						}
						if(parts[ID(r)].tmpville[16] > 0 && parts[ID(r)].tmpville[16] > parts[i].tmpville[16] * 1.5f  && parts[i].tmpville[16] < 420 && RNG::Ref().chance(1, 8))
						{
							tmps = parts[ID(r)].tmpville[16];
							parts[ID(r)].tmpville[16] = parts[i].tmpville[16];
							parts[i].tmpville[16]= tmps;
						}	
					}
				}






				if(parts[i].pavg[0] != 2)
				{

					// Distribute nutrients



					if (sim->elements[rt].Properties & PROP_ANIMAL && parts[ID(r)].pavg[0] != 2 && RNG::Ref().chance(1, 8))
						{
							if (rt == PT_BVSL || rt == PT_BLOD)
								partnum = 10;
							else
								partnum = 5;
							
						// 	//take
						// 	lcapacity = parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;
						// 	if (RNG::Ref().chance(1, 8) && lcapacity + 10 < parts[i].capacity)
						// 	{
									
								if (parts[i].oxygens < parts[i].capacity / 3 && parts[ID(r)].oxygens >= 10 + 10 && parts[i].oxygens < parts[ID(r)].oxygens)
								{
									parts[i].oxygens += std::min(partnum, parts[ID(r)].oxygens - 10);
									parts[ID(r)].oxygens -= std::min(partnum, parts[ID(r)].oxygens - 10);
								}

								if (parts[i].carbons < parts[i].capacity / 3 && parts[ID(r)].carbons >= 10 + 10 && parts[i].carbons < parts[ID(r)].carbons)
								{
									parts[i].carbons += std::min(partnum, parts[ID(r)].carbons - 10);
									parts[ID(r)].carbons -= std::min(partnum, parts[ID(r)].carbons - 10);
								}
								if (parts[i].co2 < parts[i].capacity / 3 && parts[ID(r)].co2 >= 10 + 10 && parts[i].co2 < parts[ID(r)].co2)
								{
									parts[i].co2 += std::min(partnum, parts[ID(r)].co2 - 10);
									parts[ID(r)].co2 -= std::min(partnum, parts[ID(r)].co2 - 10);
								}	
								if (parts[i].nitrogens < parts[i].capacity / 3 && parts[ID(r)].nitrogens >= 10 + 10 && parts[i].nitrogens < parts[ID(r)].nitrogens && !(parts[i].type != PT_POPS && rt == PT_POPS))
								{
									parts[i].nitrogens += std::min(partnum, parts[ID(r)].nitrogens - 10);
									parts[ID(r)].nitrogens -= std::min(partnum, parts[ID(r)].nitrogens - 10);
								}
									if (parts[i].water < parts[i].capacity / 3 && parts[ID(r)].water >= 10 + 10 && parts[i].water < parts[ID(r)].water)
								{
									parts[i].water += std::min(partnum, parts[ID(r)].water - 10);
									parts[ID(r)].water -= std::min(partnum, parts[ID(r)].water - 10);
								}
								
						// 		if (parts[i].water < parts[i].capacity / 3 && parts[ID(r)].water >= 10 + 10 && parts[i].water < parts[ID(r)].water)
						// 		{
						// 			parts[i].water += std::min(partnum, parts[ID(r)].water - 10);
						// 			parts[ID(r)].water -= std::min(partnum, parts[ID(r)].water - 10);
					 		
					
						 	//give
					//	 	lcapacity = parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].co2 + parts[ID(r)].water + parts[ID(r)].nitrogens;
						// 	if (RNG::Ref().chance(1, 8) && lcapacity + 10 < parts[ID(r)].capacity)
						 //	{
					 		if (parts[ID(r)].oxygens < parts[ID(r)].capacity / 3 && parts[i].oxygens >= 10 + 10 && parts[ID(r)].oxygens < parts[i].oxygens)
						 		{
						 			parts[ID(r)].oxygens += std::min(partnum, parts[i].oxygens - 10);
						 			parts[i].oxygens -= std::min(partnum, parts[i].oxygens - 10);
						 		}
						 		if (parts[ID(r)].carbons < parts[ID(r)].capacity / 3 && parts[i].carbons >= 10 + 10 && parts[ID(r)].carbons < parts[i].carbons)
						 		{
						 			parts[ID(r)].carbons += std::min(partnum, parts[i].carbons - 10);
						 			parts[i].carbons -= std::min(partnum, parts[i].carbons - 10);
						 		}
						 		if (parts[ID(r)].co2 < parts[ID(r)].capacity / 3 && parts[i].co2 >= 10 + 10 && parts[ID(r)].co2 < parts[i].co2)
						 		{
						 			parts[ID(r)].co2 += std::min(partnum, parts[i].co2 - 10);
						 			parts[i].co2 -= std::min(partnum, parts[i].co2 - 10);
						 		}
						 		if (parts[ID(r)].nitrogens < parts[ID(r)].capacity / 3 && parts[i].nitrogens >= 10 + 10 && parts[ID(r)].nitrogens < parts[i].nitrogens && !(parts[i].type == PT_POPS && rt != PT_POPS))
						 		{
						 			parts[ID(r)].nitrogens += std::min(partnum, parts[i].nitrogens - 10);
						 			parts[i].nitrogens -= std::min(partnum, parts[i].nitrogens - 10);
					 		}
						 		if (parts[ID(r)].water < parts[ID(r)].capacity / 3 && parts[i].water >= 10 + 10 && parts[ID(r)].water < parts[i].water)
						 		{
						 			parts[ID(r)].water += std::min(partnum, parts[i].water - 10);
						 			parts[i].water -= std::min(partnum, parts[i].water - 10);
					 		}
						 	

								if ((rt == PT_BVSL || rt == PT_BLOD || parts[i].type == rt))
								{
									//take
									if(RNG::Ref().chance(1, 2))
									{

								if (parts[i].carbons < parts[i].capacity / 3 && parts[ID(r)].carbons >= 10 + 10 && parts[i].carbons * 1.5f < parts[ID(r)].carbons)
								{
									tmpt = parts[i].carbons;
									parts[i].carbons = parts[ID(r)].carbons;
									parts[ID(r)].carbons = tmpt;
								//	parts[i].carbons += std::min(partnum, parts[ID(r)].carbons - 10);
								//	parts[ID(r)].carbons -= std::min(partnum, parts[ID(r)].carbons - 10);
								}
								if (parts[i].co2 < parts[i].capacity / 3 && parts[ID(r)].co2 >= 10 + 10 && parts[i].co2 * 1.5f < parts[ID(r)].co2)
								{
									tmpt = parts[i].co2;
									parts[i].co2 = parts[ID(r)].co2;
									parts[ID(r)].co2 = tmpt;
								//	parts[i].co2 += std::min(partnum, parts[ID(r)].co2 - 10);
								//	parts[ID(r)].co2 -= std::min(partnum, parts[ID(r)].co2 - 10);
								}
								if (parts[i].nitrogens < parts[i].capacity / 3 && parts[ID(r)].nitrogens >= 10 + 10 && parts[i].nitrogens * 1.5f < parts[ID(r)].nitrogens && !(parts[i].type == PT_POPS && rt != PT_POPS))
								{	
									tmpt = parts[i].nitrogens;
									parts[i].nitrogens = parts[ID(r)].nitrogens;
									parts[ID(r)].nitrogens = tmpt;
								//	parts[i].nitrogens += std::min(partnum, parts[ID(r)].nitrogens - 10);
								//	parts[ID(r)].nitrogens -= std::min(partnum, parts[ID(r)].nitrogens - 10);
								}
								if (parts[i].oxygens < parts[i].capacity / 3 && parts[ID(r)].oxygens >= 10 + 10 && parts[i].oxygens * 1.5f < parts[ID(r)].oxygens)
								{
									tmpt = parts[i].oxygens;
									parts[i].oxygens = parts[ID(r)].oxygens;
									parts[ID(r)].oxygens = tmpt;	
								//	parts[i].oxygens += std::min(partnum, parts[ID(r)].oxygens - 10);
								//	parts[ID(r)].oxygens -= std::min(partnum, parts[ID(r)].oxygens - 10);
								}

								if (parts[i].co2 < parts[i].capacity / 3 && parts[ID(r)].co2 >= 10 + 10 && parts[i].co2 * 1.5f < parts[ID(r)].co2)
								{
									tmpt = parts[i].co2;
									parts[i].co2 = parts[ID(r)].co2;
									parts[ID(r)].co2 = tmpt;
								//	parts[i].co2 += std::min(partnum, parts[ID(r)].co2 - 10);
								//	parts[ID(r)].co2 -= std::min(partnum, parts[ID(r)].co2 - 10);
								}
								if (parts[i].hydrogens < parts[i].capacity / 3 && parts[ID(r)].hydrogens >= 10 + 10 && parts[i].hydrogens * 1.5f < parts[ID(r)].hydrogens)
								{	
									tmpt = parts[i].hydrogens;
									parts[i].hydrogens = parts[ID(r)].hydrogens;
									parts[ID(r)].hydrogens = tmpt;
								//	parts[i].nitrogens += std::min(partnum, parts[ID(r)].nitrogens - 10);
								//	parts[ID(r)].nitrogens -= std::min(partnum, parts[ID(r)].nitrogens - 10);
								}
								}
								//give
								else
								{
								if (parts[ID(r)].co2 < parts[ID(r)].capacity / 3 && parts[i].co2 >= 10 + 10 && parts[ID(r)].co2 * 1.5f < parts[i].co2)
								{
									tmpt = parts[ID(r)].co2;
									parts[ID(r)].co2 = parts[i].co2;
									parts[i].co2 = tmpt;
								//	parts[ID(r)].co2 += std::min(partnum, parts[i].co2 - 10);
								//	parts[i].co2 -= std::min(partnum, parts[i].co2 - 10);
								}
								if (parts[ID(r)].nitrogens < parts[ID(r)].capacity / 3 && parts[i].nitrogens >= 10 + 10 && parts[ID(r)].nitrogens * 1.5f < parts[i].nitrogens && !(parts[i].type == PT_POPS && rt != PT_POPS))
								{	
									tmpt = parts[ID(r)].nitrogens;
									parts[ID(r)].nitrogens = parts[i].nitrogens;
									parts[i].nitrogens = tmpt;
								//	parts[ID(r)].nitrogens += std::min(partnum, parts[i].nitrogens - 10);
								//	parts[i].nitrogens -= std::min(partnum, parts[i].nitrogens - 10);
								}
								if (parts[ID(r)].oxygens < parts[ID(r)].capacity / 3 && parts[i].oxygens >= 10 + 10 && parts[ID(r)].oxygens * 1.5f < parts[i].oxygens)
								{
									tmpt = parts[ID(r)].oxygens;
									parts[ID(r)].oxygens = parts[i].oxygens;
									parts[i].oxygens = tmpt;	
								//	parts[ID(r)].oxygens += std::min(partnum, parts[i].oxygens - 10);
								//	parts[i].oxygens -= std::min(partnum, parts[i].oxygens - 10);
								}
								if (parts[ID(r)].carbons < parts[ID(r)].capacity / 3 && parts[i].carbons >= 10 + 10 && parts[ID(r)].carbons * 1.5f < parts[i].carbons)
								{
									tmpt = parts[ID(r)].carbons;
									parts[ID(r)].carbons = parts[i].carbons;
									parts[i].carbons = tmpt;
								//	parts[ID(r)].carbons += std::min(partnum, parts[i].carbons - 10);
								//	parts[i].carbons -= std::min(partnum, parts[i].carbons - 10);
								}
								if (parts[ID(r)].co2 < parts[ID(r)].capacity / 3 && parts[i].co2 >= 10 + 10 && parts[ID(r)].co2 * 1.5f < parts[i].co2)
								{
									tmpt = parts[ID(r)].co2;
									parts[ID(r)].co2 = parts[i].co2;
									parts[i].co2 = tmpt;
								//	parts[ID(r)].co2 += std::min(partnum, parts[i].co2 - 10);
								//	parts[i].co2 -= std::min(partnum, parts[i].co2 - 10);
								}
								if (parts[ID(r)].hydrogens < parts[ID(r)].capacity / 3 && parts[i].hydrogens >= 10 + 10 && parts[ID(r)].hydrogens * 1.5f < parts[i].hydrogens)
								{	
									tmpt = parts[ID(r)].hydrogens;
									parts[ID(r)].hydrogens = parts[i].hydrogens;
									parts[i].hydrogens = tmpt;
								//	parts[ID(r)].nitrogens += std::min(partnum, parts[i].nitrogens - 10);
								//	parts[i].nitrogens -= std::min(partnum, parts[i].nitrogens - 10);
								}
								}
								}

								
						 }
				}

					/*int diff = parts[i].oxygens - parts[ID(r)].oxygens;
					parts[i].oxygens -= diff / 2;
					parts[ID(r)].oxygens += (diff + 1) / 2;
					diff = parts[i].carbons - parts[ID(r)].carbons;
					parts[i].carbons -= diff / 2;
					parts[ID(r)].carbons += (diff + 1) / 2;
					diff = parts[i].co2 - parts[ID(r)].co2;
					parts[i].co2 -= diff / 2;
					parts[ID(r)].co2 += (diff + 1) / 2;*/


					// Take damage if touching toxic chemicals
					if ((rt == PT_H2O2 || rt == PT_ACID || rt == PT_CAUS || rt == PT_PLUT || rt == PT_URAN || rt == PT_ISOZ || rt == PT_ISZS || rt == PT_POLO || rt == PT_MERC || sim->elements[rt].Properties & PROP_DEADLY || (rt == PT_HCL && parts[i].type != PT_STMH)) && parts[i].pavg[0] != 2 && RNG::Ref().chance(1, 20)) 
					{
						parts[i].tmp3--;
					}



				
			}
		}
		
		

			
		
	if (fabs(sim->pv[y / CELL][x / CELL]) > 10.0f)
	{
		parts[i].vx += 0.10f * sim->vx[y / CELL][x / CELL];
		parts[i].vy += 0.10f * sim->vy[y / CELL][x / CELL];
	}
	if (parts[i].tmp2 > R_TEMP && RNG::Ref().chance(restrict_flt(parts[i].tmp2, 1, 1999), 2000))
		parts[i].tmp2--;



	parts[i].tmpcity[9]++;
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	if (cpart->pavg[0] == 1) { // Skin
		*colr = 255;
		*colg = 226;
		*colb = 204;
	}
	else if (cpart->life == 1) { // White part
		*colr = 255;
		*colg = 238;
		*colb = 230;
	}
	else { // Redden if oxygenated, green for waste, blue for nutrients
		int red = std::min(40, cpart->oxygens / 10);
		int green = std::min(40, cpart->co2 / 10);
		int blue = std::min(40, cpart->carbons / 10);
		*colr += red;
		*colg += green;
		*colb += blue;
		*colg -= red + blue;
		*colb -= red + green;
		*colr -= green + blue;
	}


	if (cpart->pavg[0] == 2) { // death
		float percent_fade = 1;
		percent_fade += ((abs(nx - ny) * (nx + ny) + nx) % 5) / 10.0f; // Noise

		*colr -= (*colr - 160) * percent_fade;
		*colg -= (*colg - 96) * percent_fade;
		*colb -= (*colb - 69) * percent_fade;
	}

	// Cooking
	// Well done (Around 70 - 80 C)
	if (cpart->carbons > 273.15f + 40.0f) {
		float percent_fade = std::min(cpart->tmp2 - 273.15f, 80.0f) / 80.0f;
		percent_fade += ((abs(nx - ny) * (nx + ny) + nx) % 5) / 10.0f; // Noise

		*colr -= (*colr - 160) * percent_fade;
		*colg -= (*colg - 96) * percent_fade;
		*colb -= (*colb - 69) * percent_fade;

		// Grill lines
		if ((nx + ny) % 30 < 3) {
			*colr *= 0.9f, * colg *= 0.9f, * colb *= 0.9f;
		}
	}
	// Burnt (Above 110 C)
	if (cpart->tmp2 > 273.15f + 110.0f) {
		float m = 1.0f - std::min(cpart->tmp2 - 273.15f + 90.0f, 200.0f) / 200.0f;
		m = 0.2 + 0.8 * m; // Prevent 100% black
		*colr *= m, * colg *= m, * colb *= m;
	}
	// Blue when cold
	if (cpart->temp < 273 && cpart->tmp2 < 273.15f + 110.0f) {
		*colr -= restrict_flt((273.15f - cpart->temp) / 5, 0, 80);
		*colg += restrict_flt((273.15f - cpart->temp) / 4, 0, 40);
		*colb += restrict_flt((273.15f - cpart->temp) / 1.5, 0, 100);
	}


	

	return 0;
}
