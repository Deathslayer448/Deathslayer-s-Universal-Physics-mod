#include "simulation/ElementCommon.h"
//#include <iostream>
//#include <time.h>
#include <stdlib.h>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
int Element_FLSH_update(UPDATE_FUNC_ARGS);

void Element::Element_BVSL() {
	Identifier = "DEFAULT_PT_BVSL";
	Name = "BVSL";
	Colour = 0xF05F5B_rgb;
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

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 100;

	HeatConduct = 250;
	Description = "Blood vessel. Transports nutrients, co2 and oxygen.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE ;

	DefaultProperties.oxygens = 100;
	DefaultProperties.carbons = 100;
	DefaultProperties.hydrogens = 20;
	DefaultProperties.water = 50;
	DefaultProperties.tmp2 = 2;
	DefaultProperties.tmp3 = 100;
	DefaultProperties.tmp4 = 100;
	DefaultProperties.tmpcity[7] = 1000;
	DefaultProperties.tmpcity[9] = 0;
	DefaultProperties.tmpcity[3] = 100;
	DefaultProperties.tmpville[3] = 0;
	DefaultProperties.tmpville[4] = 0;
	DefaultProperties.metabolism = 50;

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
}

static int update(UPDATE_FUNC_ARGS) {
	/**
	 * Properties:
	 * oxygens - How much oxygen stored
	 * carbons - nutrients
	 * tmp - How many particles it can still stain
	 * tmp2 - Visocity, slowly increases randomly with contact to air
	 * pavg[0]: Type 0 = inside, 1 = skin, 2 = dead
	 * tmp3: health of part 0-100
	 * tmpcity[3]: energy stored for useasassdasddas
	 * hydrogens: carbon dioxide & waste
	 * tmpcity[7]: capacity for stuff
	 * nitrogens: waste from food and other things
	 * tmpville[3]: s other check to check where its walls
	 */
	Element_FLSH_update(sim, i, x, y, surround_space, nt, parts, pmap);
	if (parts[i].tmp3 != 2)
	{
		int rx, ry, r, rt;// e, f;
		
		/*int arraye[9];
		int arraynumbrs[9];
		int givestuff[9];
		int rndtmp = 0;*/
		int lcapacity = 0;

	/*	for (ry = -1; ry < 2; ++ry)
				{
			for (rx = -1; rx < 2; ++rx)
			{
				
				if ((rx || ry))
				{
					arraye[rndtmp] = pmap[y + ry][x + rx];

					//std::shuffle(std::begin(array), std::end(array), RNG::Ref());
					rndtmp++;
				}
			}
		}
		for (e = 0; e < 8; e++)
		{
			rndtmp = sim->rng.between(0, 8);
			if (std::find(std::begin(arraynumbrs), std::end(arraynumbrs), rndtmp) != std::end(arraynumbrs))
			{
				e--;

			}
			else
			{
				givestuff[rndtmp] = arraye[e];
				arraynumbrs[e] = rndtmp;
			}
		}*/
		int turntoblod = 0;
		for (ry = -1; ry < 2; ++ry)
			for (rx = -1; rx < 2; ++rx)

				//for (f = 0; f < 8; f++)
			{
				if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
					r = pmap[y + ry][x + rx];//givestuff[f]
					//	if (!r) continue;
					int partnum = 10;
					
					rt = TYP(r);
					//	if (rt == PT_BRKN)
						//	rt = parts[ID(r)].ctype;
					if(!r && parts[i].tmpville[3] == 0)
						parts[i].tmpville[3] = 1;
					else if(!r)
						continue;
						//r = sim->photons[y + ry][x + rx];
					//	if (surround_space && !r && TYP(parts[i].ctype))  //creating at end
					//	{
							//int np =
						//		sim->create_part(-1, x + rx, y + ry, PT_BLOD);
							//if (np != -1)
						//	{
							//	Element_PIPE_transfer_pipe_to_part(sim, parts + i, parts + np, false);
						//	}
					//	}
					if((rt == PT_FLSH || rt == PT_UDDR || rt == PT_LUNG || rt == PT_POPS || rt == PT_STMH || rt == PT_BRIN) && parts[i].tmpville[3] == 0 )
						parts[i].tmpville[3] = 1;
					
					if (rt == PT_BVSL && parts[i].tmpville[3] == 0 && parts[ID(r)].tmpville[3] > 0 && sim->rng.chance(1, 8))
						parts[i].tmpville[3] = parts[ID(r)].tmpville[3] + 1;
					
					if (turntoblod > 6 && parts[i].tmpville[3] > 2 && sim->rng.chance(1, 8))
					{
						sim->part_change_type(i, x, y, PT_BLOD);
						parts[i].tmp2 = restrict_flt(parts[i].water / 10, 0, 100);
					}
					if ((rt == PT_BVSL || rt == PT_BLOD) && parts[i].tmpville[3] > 2 && parts[ID(r)].tmpville[3] > 0)
					{
						turntoblod++;
					}

					










					//if (rt == PT_POPS && (parts[ID(r)].hydrogens > parts[ID(r)].tmpcity[7] / 3 || parts[ID(r)].oxygens > parts[ID(r)].tmpcity[7] / 3 || parts[ID(r)].carbons > parts[ID(r)].tmpcity[7] / 3 || parts[ID(r)].water > parts[ID(r)].tmpcity[7] / 3))
					//	parts[ID(r)].tmpcity[5]++;
					//



					//if (rt == PT_FLSH || rt == PT_STMH || rt == PT_UDDR || rt == PT_LUNG || rt == PT_POPS)
					//{
					//	capacity = parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].hydrogens + parts[ID(r)].water + parts[ID(r)].nitrogens;
					//	if (sim->rng.chance(1, 8) && capacity + partnum < parts[ID(r)].tmpcity[7])
					//	{
					//		
					//		//give stuff
					//		if (parts[ID(r)].oxygens + partnum < parts[ID(r)].tmpcity[7] / 2 && parts[i].oxygens >= partnum + 10 && parts[i].oxygens > parts[ID(r)].oxygens)
					//		{
					//			parts[ID(r)].oxygens += partnum;
					//			parts[i].oxygens -= partnum;

					//		}
					//		if (parts[ID(r)].carbons < parts[ID(r)].tmpcity[7] / 2 && parts[i].carbons >= partnum + 10 && parts[i].carbons > parts[ID(r)].carbons)
					//		{
					//			parts[ID(r)].carbons += partnum;
					//			parts[i].carbons -= partnum;

					//		}
					//		
					//		if ((parts[ID(r)].hydrogens < parts[ID(r)].tmpcity[7] / 5 && rt != PT_LUNG) || (parts[ID(r)].hydrogens < parts[ID(r)].tmpcity[7] / 2 && rt == PT_LUNG) || (parts[ID(r)].hydrogens < parts[ID(r)].tmpcity[7] / 2 && (rt == PT_BVSL || rt == PT_BLOD)) && parts[i].co2 >= partnum + 10 && parts[i].co2 > parts[ID(r)].hydrogens)
					//		{
					//			parts[ID(r)].hydrogens += partnum;
					//			parts[i].co2 -= partnum;
					//		}
					//		if (parts[ID(r)].water < parts[ID(r)].tmpcity[7] / 3 && parts[i].water >= partnum + 10 && parts[i].water > parts[ID(r)].water)
					//		{
					//			parts[ID(r)].water += partnum;
					//			parts[i].water -= partnum;

					//		}
					//		////give carbon waste to lungs
					//		//else if (parts[ID(r)].hydrogens < parts[ID(r)].tmpcity[7] / 2 && parts[i].co2 >= partnum && parts[i].co2 > parts[ID(r)].hydrogens && rt == PT_LUNG)
					//		//{
					//		//	parts[ID(r)].hydrogens += partnum;
					//		//	parts[i].co2 -= partnum;

					//		//}
					//	

					//		
					//	}
					//	

					//	capacity = parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;
					//	if (sim->rng.chance(1, 8) && capacity + partnum < parts[i].capacity)
					//	{
					//		
					//		//take stuff
					//		if (parts[i].carbons + partnum < parts[i].capacity / 2 && parts[ID(r)].oxygens >= partnum + 10 && parts[ID(r)].oxygens > parts[i].oxygens)
					//		{
					//			parts[i].oxygens += partnum;
					//			parts[ID(r)].oxygens -= partnum;

					//		}
					//		if (parts[i].carbons + partnum < parts[i].capacity / 2 && parts[ID(r)].carbons >= partnum + 10 && parts[ID(r)].carbons > parts[i].carbons)
					//		{
					//			parts[i].carbons += partnum;
					//			parts[ID(r)].carbons -= partnum;

					//		}
					//		if (((rt == PT_LUNG && parts[ID(r)].hydrogens > parts[i].capacity / 2 && parts[i].co2 + partnum < parts[i].capacity / 3) || (rt != PT_LUNG && parts[i].co2 + partnum < parts[i].capacity / 3)) && parts[ID(r)].hydrogens >= partnum + 10 && parts[ID(r)].hydrogens > parts[i].co2)
					//		{
					//			parts[i].co2 += partnum;
					//			parts[ID(r)].hydrogens -= partnum;

					//		}
					//		if (parts[i].water + partnum < parts[i].capacity / 3 && parts[ID(r)].water >= partnum + 10 && parts[ID(r)].water > parts[i].water)
					//		{
					//			parts[i].water += partnum;
					//			parts[ID(r)].water -= partnum;

					//		}
					//	}

					//}


					//else if (rt == PT_BLOD || rt == PT_BVSL)
					//{
					//	
					//	partnum += 10;
					//	capacity = parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].hydrogens + parts[ID(r)].water + parts[ID(r)].nitrogens;
					//	if (sim->rng.chance(1, 8) && capacity + partnum < parts[i].capacity)
					//	{
					//	

					//		//give stuff to blood
					//		if (parts[ID(r)].oxygens + partnum < parts[ID(r)].tmpcity[7] / 2 && parts[i].oxygens >= partnum + 10 && parts[i].oxygens > parts[ID(r)].oxygens)
					//		{
					//			parts[ID(r)].oxygens += partnum;
					//			parts[i].oxygens -= partnum;

					//		}
					//		if (parts[ID(r)].carbons + partnum < parts[ID(r)].tmpcity[7] / 2 && parts[i].carbons >= partnum + 10 && parts[i].carbons > parts[ID(r)].carbons)
					//		{
					//			parts[ID(r)].carbons += partnum;
					//			parts[i].carbons -= partnum;

					//		}
					//			
					//		if (parts[ID(r)].hydrogens + partnum < parts[ID(r)].tmpcity[7] / 3 && parts[i].co2 >= partnum + 10 && parts[i].co2 > parts[ID(r)].hydrogens)
					//		{
					//			parts[ID(r)].hydrogens += partnum;
					//			parts[i].co2 -= partnum;

					//		}
					//		if (parts[ID(r)].water + partnum < parts[ID(r)].tmpcity[7] / 3 && parts[i].water >= partnum + 10 && parts[i].water > parts[ID(r)].water)
					//		{
					//			parts[ID(r)].water += partnum;
					//			parts[i].water -= partnum;

					//		}
					//	}
					//	capacity = parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;
					//	if (sim->rng.chance(1, 8) && capacity + partnum < parts[i].capacity)
					//	{
					//		

					//		//take stuff from blood
					//		if (parts[i].oxygens + partnum < parts[ID(r)].tmpcity[7] / 2 && parts[ID(r)].oxygens >= partnum + 10 && parts[ID(r)].oxygens > parts[i].oxygens)
					//		{
					//			parts[i].oxygens += partnum;
					//			parts[ID(r)].oxygens -= partnum;

					//		}
					//		if (parts[i].carbons + partnum < parts[ID(r)].tmpcity[7] / 2 && parts[ID(r)].carbons >= partnum + 10 && parts[ID(r)].carbons > parts[i].carbons)
					//		{
					//			parts[i].carbons += partnum;
					//			parts[ID(r)].carbons -= partnum;

					//		}
					//		if (parts[i].co2 + partnum < parts[ID(r)].tmpcity[7] / 3 && parts[ID(r)].hydrogens >= partnum && parts[ID(r)].hydrogens > parts[i].co2)
					//		{
					//			parts[i].co2 += partnum;
					//			parts[ID(r)].hydrogens -= partnum;

					//		}
					//		if (parts[i].water + partnum < parts[ID(r)].tmpcity[7] / 3 && parts[ID(r)].water >= partnum && parts[ID(r)].water > parts[i].water)
					//		{
					//			parts[i].water += partnum;
					//			parts[ID(r)].water -= partnum;

					//		}
					//	}

					//}
					//if(sim->rng.chance(1, 1000)


					//sim->kill_part(ID(r));
				}
			}


		//if (rt == PT_BLOD && parts[ID(r)].oxygens < 100 && parts[i].oxygens >= 10)
		//{

		//	//	int diff = parts[i].tmp - parts[ID(r)].oxygens;
		//	parts[i].oxygens -= 10;
		//	parts[ID(r)].oxygens += 10;
		//}

		/*if (rt == PT_LUNG && parts[ID(r)].oxygens < 100 && parts[i].oxygens >= 10)
		{


			parts[i].oxygens -= 10;
			parts[ID(r)].oxygens += 10;
		}*/

		//}

//	if (parts[i].tmp4 == 1) // Override skin formation
	//	parts[i].tmp4 = 0;

	//if (parts[i].tmp4 != 2) {
		//int rx, ry, r, rt;
		//for (rx = -1; rx < 2; ++rx)
		//for (ry = -1; ry < 2; ++ry)
		//	if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
		//		r = pmap[y + ry][x + rx];
		//		if (!r) continue;
		//		rt = TYP(r);
		//		if (rt == PT_BRKN)
		//			rt = parts[ID(r)].ctype;

		//		// Consume food
		///*		if (rt == PT_PLNT || rt == PT_VINE || rt == PT_SEED || rt == PT_POTO ||
		//			rt == PT_SPDR || rt == PT_ANT || rt == PT_SUGR) {
		//			if (sim->rng.chance(1, 500)) {
		//				parts[i].tmp += 50;
		//				parts[i].tmp2 += 500;
		//				sim->kill_part(ID(r));
		//			}
		//		}*/
		//	}
	//}
	}
	return 0;	
}

static int graphics(GRAPHICS_FUNC_ARGS) {
//	if (cpart->tmp2 < CLOT)
		*pixel_mode |= PMODE_BLUR;
	*colr *= 0.2f + 0.4f * cpart->oxygens / 100.0f;
	*colg *= 0.2f + 0.4f * cpart->hydrogens / 100.0f;
	*colb *= 0.2f + 0.4f * cpart->carbons / 100.0f;

	// Cooking
	// Well done (Around 70 - 80 C)
	//if (cpart->carbons > 273.15f + 40.0f) {
	//	float percent_fade = std::min(cpart->tmp2 - 273.15f, 80.0f) / 80.0f;
	//	percent_fade += ((abs(nx - ny) * (nx + ny) + nx) % 5) / 10.0f; // Noise

	//	*colr -= (*colr - 176) * percent_fade;
	//	*colg -= (*colg - 131) * percent_fade;
	//	*colb -= (*colb - 90) * percent_fade;

	//	// Grill lines
	//	if ((nx + ny) % 30 < 3) {
	//		*colr *= 0.9f, *colg *= 0.9f, *colb *= 0.9f;
	//	}
	//}
	// Burnt (Above 110 C)
	if (cpart->tmp2 > 273.15f + 110.0f) {
		float m = 1.0f - std::min(cpart->tmp2 - 273.15f + 90.0f, 200.0f) / 200.0f;
		m = 0.2 + 0.8 * m; // Prevent 100% black
		*colr *= m, *colg *= m, *colb *= m;
	}
	// Blue when cold
	if (cpart->temp < 273 && cpart->tmp2 < 273.15f + 110.0f) {
		*colr -= (int)restrict_flt((273-cpart->temp)/5,0,80);
		*colg += (int)restrict_flt((273-cpart->temp)/4,0,40);
		*colb += (int)restrict_flt((273-cpart->temp)/1.5,0,100);
	}

	return 0;
}
