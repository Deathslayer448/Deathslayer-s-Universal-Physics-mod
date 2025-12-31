#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
int Element_FLSH_update(UPDATE_FUNC_ARGS);

void Element::Element_POPS() {
	Identifier = "DEFAULT_PT_POPS";
	Name = "POPS";
	Colour = 0xF4AE95_rgb;
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
HotAir = 0.000f * CFDS;
Falldown = 0;

Flammable = 10;
Explosive = 0;
Meltable = 0;
Hardness = 20;

Weight = 100;

HeatConduct = 104;
Description = "Poop shoot. Releases waste and exess stuff.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

DefaultProperties.oxygens = 100;
DefaultProperties.carbons = 100;
DefaultProperties.co2 = 20;
DefaultProperties.water = 50;
DefaultProperties.tmp3 = 100;
DefaultProperties.tmp4 = 100;
DefaultProperties.capacity = 800;
DefaultProperties.tmpcity[3] = 100;
DefaultProperties.tmpcity[9] = 0;
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
	 * Properties
	 * life:  Graphics
	 * oxygens:   Oxygen stored
	 * carbons:  Nutrients stored
	 * tmp2: Highest temperature
	 * tmp3: Type 0 = inside, 1 = skin, 2 = dead
	 * co2: carbon dioxide
	 * co2:  & waste?
	 * water: water for life
	 * lcapacity: lcapacity for stuff
	 */
	Element_FLSH_update(sim, i, x, y, surround_space, nt, parts, pmap);
	//if (parts[i].tmp3 == 1) // Override skin formation
	//	parts[i].tmp3 = 0;
	if (parts[i].tmp3 != 2) {




		int rx, ry, r, rt;
		//int lcapacity = 0;




		if (surround_space != 0 && parts[i].tmpcity[0] == 0)
			parts[i].tmpcity[0] = 1;
		if (surround_space == 0 && parts[i].tmpcity[0] == 1)
			parts[i].tmpcity[0] = 0;
	//	if (sim->currentTick % 500 == 0 && parts[i].tmpcity[5] > 0)
	//		parts[i].tmpcity[5]--;
		// if(sim->currentTick % 5000 == 0 && parts[i].tmpville[14] > 0)
		// 	parts[i].tmpville[14]--;
		// if(sim->currentTick % 5000 == 0 && parts[i].tmpville[15] > 0)
		// 	parts[i].tmpville[15]--;
		// 	if(sim->currentTick % 5000 == 0 && parts[i].tmpville[16] > 0)
		// 	parts[i].tmpville[16]--;


		if(parts[i].tmpville[14] < 300 && parts[i].nitrogens > 50 && parts[i].tmpville[16] < 2)
		parts[i].tmpville[14]++;
	//	if (parts[i].tmpcity[5] < 3 && sim->rng.chance(1, 8) && (parts[i].co2 > parts[i].capacity / 4 || parts[i].oxygens > parts[i].capacity / 4 || parts[i].carbons > parts[i].capacity / 4 || parts[i].water > parts[i].capacity / 4 || parts[i].nitrogens > 30))
	//		parts[i].tmpcity[5]++;
			//	if (parts[i].co2 > parts[i].capacity / 2)
				//{
		//}
				//if()

		


		for (ry = -1; ry < 2; ++ry)
			for (rx = -1; rx < 2; ++rx)
				if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES) {
					r = pmap[y + ry][x + rx];
					int partnum, tmpt = 0;
					if (!r && parts[i].tmp3 != 2 && parts[i].tmpville[15] > 0 && parts[i].nitrogens > 50 && parts[i].water > 10 && sim->rng.chance(1, 8))
					{
					int wste = sim->create_part(-1, x + rx, y + ry, PT_WSTE);
							parts[wste].nitrogens += 50;
							parts[wste].water += 10;
							parts[i].nitrogens -= 50;
							parts[i].water -= 10;
							parts[i].tmpville[15]--;
							parts[i].tmpville[16]++;
							break;
				
			


				}
				if (!r) continue;
				rt = TYP(r);
				if (rt == PT_BRKN)
					rt = parts[ID(r)].ctype;
				//if(sim->currentTick % 501 == 0 && elements[rt].Properties &  && parts[i].tmpcity[0] != 1)
				//	parts[i].tmpcity[0] = 1;
				if(parts[ID(r)].tmp3 != 2)
				{
				//signals
				// if(parts[i].tmpville[14] > 0 && parts[ID(r)].tmpville[14] < 5 && sim->rng.chance(1, 8))
				// {
				// 	parts[i].tmpville[14]--;
				// 	parts[ID(r)].tmpville[14]++;
				// }
				// if(parts[i].tmpville[15] > 0 && parts[ID(r)].tmpville[15] < 5  && sim->rng.chance(1, 8))
				// {
				// 	parts[i].tmpville[15]--;
				// 	parts[ID(r)].tmpville[15]++;
				// }if(parts[i].tmpville[16] > 0 && parts[ID(r)].tmpville[16] < 5  && sim->rng.chance(1, 8))
				// {
				// 	parts[i].tmpville[16]--;
				// 	parts[ID(r)].tmpville[16]++;
				// }	
			//	if (rt == PT_POPS && parts[i].tmpcity[0] == 0 && parts[ID(r)].tmpcity[0] == 1)
				//	parts[i].tmpcity[0] = 2;
				// Distribute nutrients
					
					// 	if (parts[i].co2 > 20 && parts[i].co2 > parts[i].capacity / 6 && sim->rng.chance(1, 2))
					// 	{
					// 	/*	if (wste == 0 && )
					// 		 wste = sim->create_part(-1, x + rx, y + ry, sim->rng.chance(1, 3) ? PT_WSTE : PT_WATR);*/
					// 		dawaste++;
							


					// 	}
					// 	if (parts[i].carbons > 20 && parts[i].carbons > parts[i].capacity / 4 && sim->rng.chance(1, 2))
					// 	{
					// 	/*	if (wste == 0)
					// 			wste = sim->create_part(-1, x + rx, y + ry, sim->rng.chance(1, 3) ? PT_WSTE : PT_WATR);
					// 		else if (parts[wste].co2 > 0)*/
					// 		dawaste += 2;
							
					// 	}

					// 	 if (parts[i].oxygens > 20 && parts[i].oxygens > parts[i].capacity / 4 && sim->rng.chance(1, 2))
					// 	{
					// 		// if (wste == 0)
					// 		//	 wste = sim->create_part(-1, x + rx, y + ry, sim->rng.chance(1, 3) ? PT_WSTE : PT_WATR);
					// 		 dawaste += 4;
					// 	}
					// 	 if (parts[i].nitrogens > 10 && sim->rng.chance(1, 5))
					// 	{


					// 		 dawaste += 8;

					// 	/*	 if (wste == 0)
					// 			 wste = sim->create_part(-1, x + rx, y + ry, sim->rng.chance(1, 3) ? PT_WSTE : PT_WATR);
					// 		 parts[wste].nitrogens += 10;
					// 		 parts[i].nitrogens -= 10;*/
					// 	}
					// 	if (parts[i].water > 20 && parts[i].water > parts[i].capacity / 6 && sim->rng.chance(1, 2))
					// 	{
					// 		dawaste += 16;
					// 	/*	if (wste == 0)
					// 			wste = sim->create_part(-1, x + rx, y + ry, sim->rng.chance(1, 3) ? PT_WSTE : PT_WATR);
					// 		parts[wste].water += 10;
					// 		parts[i].water -= 10;*/
					// 	}
					// 	//if(wste != 0)
							



					// 	switch(dawaste)
					// 	{ 
						
					// 	case 1:


					// //	parts[wste].co2 += 10;
					// //	parts[i].co2 -= 10;



					// //	parts[i].tmpcity[5]--;
					// 	break;
					// 	case 2:
					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_CRBN);

					// 		parts[wste].carbons += 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].tmpcity[5]--;
					// 	break;
					// 	case 3:
					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_WSTE);


					// 		parts[wste].carbons += 10;
					// 		parts[wste].co2 += 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].co2 -= 10;

					// 		parts[i].tmpcity[5]--;
					// 		break;
					// 	case 4:


					// 	//	parts[wste].oxygens += 10;
					// 	//	parts[i].oxygens -= 10;
					// 	//	parts[i].tmpcity[5]--;
					// 	break;
					// 	case 5:


					// 	//	parts[i].tmpcity[5]--;
					// 		break;
					// 	case 6:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_GAS);
					// 		parts[wste].carbons += 10;
					// 		parts[wste].oxygens += 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].oxygens -= 10;
						



					// 		parts[i].tmpcity[5]--;
					// 		break;

					// 	case 16:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_WATR);
					// 		parts[wste].water += 10;
					// 		parts[i].water -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;

					// 	case 18:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_OIL);
					// 		parts[wste].carbons += 10;
					// 		parts[wste].water += 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].water -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;
					// 	case 19:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_WSTE);
					// 		parts[wste].carbons += 10;
					// 		parts[wste].water += 10;
					// 		parts[wste].co2 += 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].water -= 10;
					// 		parts[i].co2 -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;
					// 	case 20:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_WATR);
					// 		parts[wste].oxygens += 10;
					// 		parts[wste].water += 10;
					// 		parts[i].oxygens -= 10;
					// 		parts[i].water -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;

					// 	case 22:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_DESL);
					// 		parts[wste].carbons += 10;
					// 		parts[wste].water += 10;
					// 		parts[wste].oxygens += 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].water -= 10;
					// 		parts[i].oxygens -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;
					// 	case 24:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_WSTE);
					// 		parts[wste].nitrogens += 10;
					// 		parts[wste].water += 10;
					// 		parts[i].nitrogens -= 10;
					// 		parts[i].water -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;


					// 	case 27:

					// 		wste = sim->create_part(-1, x + rx, y + ry, PT_WSTE);
					// 		parts[wste].nitrogens += 10;
					// 		parts[wste].carbons += 10;
					// 		parts[wste].co2 += 10;
					// 		parts[wste].water += 10;
					// 		parts[i].nitrogens -= 10;
					// 		parts[i].carbons -= 10;
					// 		parts[i].co2 -= 10;
					// 		parts[i].water -= 10;




					// 		parts[i].tmpcity[5]--;
					// 		break;
					// 	default:


					// 		break;
					// 	}




				//	parts[i].carbons > 20 && parts[i].co2 > 20 && parts[i].water > 20 &&
				//	sim->create_part(-1, x + rx, y + ry, PT_OIL);
				
					
					//parts[i].tmpcity[5\]--;
				




					








				//(elements[rt].Properties &  && parts[ID(r)].tmp4 > 0 && parts[i].tmpcity[0] == 1 && parts[i].tmpcity[1] == 0 && sim->rng.chance(1, 50))
				//if()
			//	{
			//		parts[i].tmpcity[1]++;
			//	}
			
				// if (rt == PT_POPS && parts[i].tmpcity[5] > 0 && parts[i].tmpcity[5] > parts[ID(r)].tmpcity[5]  && parts[ID(r)].tmpcity[5] < 3 && ((parts[ID(r)].tmpcity[0] == 1 && sim->rng.chance(1, 2)) || (parts[ID(r)].tmpcity[0] == 2 && sim->rng.chance(1, 3)) || (parts[ID(r)].tmpcity[0] == 0 && sim->rng.chance(1, 10))))
				// {
				// 	parts[ID(r)].tmpcity[5]++;
				// 	parts[i].tmpcity[5]--;
				// }
			/*	if (rt == PT_POPS && parts[i].tmpcity[6] > 0 && parts[i].tmpcity[6] > parts[ID(r)].tmpcity[6] && ((parts[ID(r)].tmpcity[0] == 1 && sim->rng.chance(1, 2)) || (parts[ID(r)].tmpcity[0] == 2 && sim->rng.chance(1, 3)) || (parts[ID(r)].tmpcity[0] == 0 && sim->rng.chance(1, 10))))
				{
					parts[ID(r)].tmpcity[6]++;
					parts[i].tmpcity[6]--;
				}*/

				//if (rt == PT_BLOD || rt == PT_BVSL)
				//{
				//	partnum += 20;
				//	//int lcapacity = parts[i].oxygens + parts[i].carbons + parts[i].co2;
				//	int lcapacity = parts[ID(r)].oxygens + parts[ID(r)].carbons + parts[ID(r)].co2 + parts[ID(r)].water + parts[ID(r)].nitrogens;
				//	if (parts[i].tmpcity[6] > 0 && sim->rng.chance(1, 8) && lcapacity + partnum < parts[i].capacity)
				//	{
				//		


				//		//give stuff blood
				//	if (parts[ID(r)].oxygens + partnum < parts[ID(r)].capacity / 2 && parts[i].oxygens >= partnum + 10 && parts[i].oxygens > parts[ID(r)].oxygens)
				//	{
				//		parts[ID(r)].oxygens += partnum;
				//		parts[i].oxygens -= partnum;

				//	}
				//	if (parts[ID(r)].carbons + partnum < parts[ID(r)].capacity / 2 && parts[i].carbons >= partnum + 10 && parts[i].carbons > parts[ID(r)].carbons)
				//	{
				//		parts[ID(r)].carbons += partnum;
				//		parts[i].carbons -= partnum;

				//	}

				//	if (parts[ID(r)].co2 + partnum < parts[ID(r)].capacity / 3 && parts[i].co2 >= partnum + 10 && parts[i].co2 > parts[ID(r)].co2)
				//	{
				//		parts[ID(r)].co2 += partnum;
				//		parts[i].co2 -= partnum;

				//	}
				//	if (parts[ID(r)].water + partnum < parts[ID(r)].capacity / 3 && parts[i].water >= partnum + 10 && parts[i].water > parts[ID(r)].water)
				//	{
				//		parts[ID(r)].water += partnum;
				//		parts[i].water -= partnum;

				//	}
				//}
				//	lcapacity = parts[i].oxygens + parts[i].carbons + parts[i].co2 + parts[i].water + parts[i].nitrogens;

				//	//take stuff blood
				//	if (parts[i].tmpcity[5] > 0 && sim->rng.chance(1, 8))
				//	{
				//		if (lcapacity + partnum < 300 && parts[i].oxygens + partnum < 300 && parts[ID(r)].oxygens >= partnum + 10 && parts[ID(r)].oxygens > parts[i].oxygens)
				//		{
				//			parts[i].oxygens += partnum;
				//			parts[ID(r)].oxygens -= partnum;

				//		}
				//		if (lcapacity + partnum < 300 && parts[i].carbons + partnum < 300 && parts[ID(r)].carbons >= partnum + 10 && parts[ID(r)].carbons > parts[i].carbons)
				//		{
				//			parts[i].carbons += partnum;
				//			parts[ID(r)].carbons -= partnum;

				//		}
				//		if (lcapacity + partnum < 300 && parts[i].co2 + partnum < 300 && parts[ID(r)].co2 >= partnum + 10 && parts[i].co2 < parts[ID(r)].co2)
				//		{
				//			parts[i].co2 += partnum;
				//			parts[ID(r)].co2 -= partnum;

				//		}
				//	}
				//}
				/*if (rt == PT_HCL)
				{
					if (parts[ID(r)].tmp4 > 0 && parts[i].carbons < 290 && (elements[parts[ID(r)].ctype].Properties & | elements[parts[ID(r)].tmpcity[]].Properties))
					{

					
						if (parts[ID(r)].ctype == PT_SUGR || parts[ID(r)].ctype == PT_SWTR)
						{
							parts[i].carbons += std::min(15, parts[ID(r)].tmp4);
							parts[i].co2 += std::min(5, parts[ID(r)].tmp4);;
	
							parts[ID(r)].tmp4 -= std::min(20, parts[ID(r)].tmp4);;
						}
						else
						{
						parts[i].carbons += std::min(10, parts[ID(r)].tmp4);
						parts[ID(r)].tmp4 -= std::min(10, parts[ID(r)].tmp4);
						if (parts[ID(r)].carbons > 0)
						{
							parts[i].carbons += std::min(10, parts[ID(r)].carbons);
							parts[ID(r)].carbons -= std::min(10, parts[ID(r)].carbons);
							}
						if (parts[ID(r)].oxygens > 0)
						{
							parts[i].oxygens += std::min(10, parts[ID(r)].oxygens);
							parts[ID(r)].oxygens -= std::min(10, parts[ID(r)].oxygens);
						}
						if (parts[ID(r)].co2 > 0)
						{
							parts[i].co2 += std::min(10, parts[ID(r)].co2);
							parts[ID(r)].co2 -= std::min(10, parts[ID(r)].co2);
						}
						}
						if(parts[ID(r)].tmp4 <= 0 && parts[ID(r)].co2 <= 0 && parts[ID(r)].oxygens <= 0 && parts[ID(r)].carbons <= 0 && sim->rng.chance(1, 10))
							sim->kill_part(ID(r));
					
					}

				}*/
			}
				}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	// Redden if oxygenated
	int red = std::min(20, cpart->oxygens / 10);
	*colr += cpart->tmpcity[14] * 50;
	*colg -= red;
	*colb -= cpart->nitrogens * 50;

	// Cooking
	// Well done (Around 70 - 80 C)
	if (cpart->carbons > 273.15f + 40.0f) {
		float percent_fade = std::min(cpart->tmp2 - 273.15f, 80.0f) / 80.0f;
		percent_fade += ((abs(nx - ny) * (nx + ny) + nx) % 5) / 10.0f; // Noise

		*colr -= (*colr - 176) * percent_fade;
		*colg -= (*colg - 131) * percent_fade;
		*colb -= (*colb - 90) * percent_fade;

		// Grill lines
		if ((nx + ny) % 30 < 3) {
			*colr *= 0.9f, *colg *= 0.9f, *colb *= 0.9f;
		}
	}
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
