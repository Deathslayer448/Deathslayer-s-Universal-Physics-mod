#include "simulation/ElementCommon.h"
#include <iostream>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

#define DIE() {parts[i].tmp3=2;return 0;}
#define EAT(other) {\
	sim->part_change_type(ID(r), parts[ID(r)].x, parts[ID(r)].y, sim->rng.chance(1, 15) ? other : PT_WATR);\
	parts[i].tmpcity[3] += parts[ID(r)].tmp4;parts[ID(r)].tmp4 = 0;}

namespace BCTR {    
	const int TEMP_RES_MULTI = 5;
	const int ENERGY_MULTI = 20;
	const int FOOD_LIFE = 80;
	const int TEMP_LIFE_GAIN = 5; // Energy gained per 0.5 degrees heat
	const int START_LIFE = 420;
	const int AGE_MULTI = 90;
	const int AGE_BASE = 60000;
	const int MUTATION_RATE = 50; // Higher = less mutations SHOULD MAKE THIS A VARIABLE

	/**
	 * Return an int represented by the bits of the target
	 * from start to end, for example if target was
	 * 000100 and we extracted from 0 to 3, we would get 0001 = 1
	 * 
	 * Assumes 32-bit int
	 */
	int extract_bits(int target, int start, int end) {
		if (start < 0) start = 0;
		if (end > 31) end = 31;
		if (target == 0) return 0;

		unsigned int mask = 0;
		for (int t = start; t <= end; ++t)
			mask |= 1 << t;
		return (target & mask) >> start;
	}

	/**
	 * Apply a random mutation to a target gene
	 */
	int mutate(Simulation *sim, int gene, int mutchance) {
		// Random flip a bit if mutation allows it
		if (sim->rng.chance(1, restrict_flt(mutchance, 1, MAX_TEMP)))
			return gene;
		return gene ^ (1 << sim->rng.between(0, 32));
	}

	void extract_genes(int gene, int &restype, int &resval, int &foodtype, int &metabolism, int &spread,
			int &move, int &graphics, bool &glow, int &mutrate) {
		restype = extract_bits(gene, 0, 3);
		resval = extract_bits(gene, 4, 7);
		foodtype = extract_bits(gene, 8, 11);
		metabolism = extract_bits(gene, 12, 15);
		spread = extract_bits(gene, 16, 19);
		move = extract_bits(gene, 20, 22);
		graphics = extract_bits(gene, 23, 25);
		glow = extract_bits(gene, 26, 26);
		mutrate = extract_bits(mutrate, 27, 31);

	}
}

/*
Bacteria features:
- Dies if it runs out of energy/food
- Dies if temperature is too high or low
- Dies if touches SOAP
- Infects STKM

Life stores energy value

tmp stores "age", time to tmpcity[3] determined by metabolism: (15 - metabolism) * AGE_MULTI + AGE_BASE

pavg0 is if the bacteria is dead

Genes are stored in ctype
Also see GameView.cpp
4:  0000  - Resistance type (See below)
			1 = HEAT (and FIRE)
			2 = COLD
			3 = VIRS
			4 = SOAP / SALT
			5 = ACID hcl
			6 = Being EATEN
			7 = Radiation
8:  0000  - Resistance value (0 - 15, 0 = no resistance)
12: 0000  - Food type (See below)
			(Not listed) = SUGR
			1 = YEST / DYST
			2 = PLNT / WOOD / SAWD
			3 = PHOT or BRAY
			4 = NEUT / ELEC / PROT
			5 = BCTR
			6 = Thermal energy
16: 0000  - Metabolism speed (0 - 15, 0 = no growth) not including other gene modifiers
	   	    Energy storage is inversely proportional to metabolism
20: 0000  - Spread vector
			(Not listed) - Default liquid
			1 - Stick to solids and powders
			2 - Float through the air
			3 - Able to "penetrate" or go inside animal cells
			4 - Inject self DNA into other BCTR
23: 000   - Move speed (increased energy cost, 0 - 7)
26: 000   - Graphics (fuzziness?)
			(Not listed) - PMODE_BLUR
			1 - SPARK
			2 - BLOB
			3 - FLAT
27: 0     - Glow in the dark?
31: 00000 - Unused


Will pass on dcolour to children

Genes:
- Resistances:
   - Heat / Cold
   - Radiation
   - VIRS
   - SOAP / ACID
   - Other BCTR
- Metabolize different food sources
   - SUGR (default)
   - YEST (not that far of an evolutionary leap)
   - PLNT / photosynthesis
   - Other BCTR?
- Growth speed
   - How fast it consumes energy
   - Produces gas as it consumes (depends on food)
- Energy storage capacity
   - Spends energy to store however
- Spread vector
   - Stick to solids
   - Spread through air
   - Spread through liquids
   - gets inside animal cells
- Color
   - Fancy colors ooooh
   - Glow in the dark gene, costs energy but is cool

Multiplier for speed of growth
Bacteria requires less sugar to grow as time goes on or if it has enough food
 */

void Element::Element_BCTR() {
	Identifier = "DEFAULT_PT_BCTR";
	Name = "BCTR";
	Colour = 0xDCF781_rgb;
	MenuVisible = 1;
	MenuSection = SC_ORGANIC;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 30;
	HeatConduct = 150;
	Description = "Bacteria. Feed it sugar to grow, can evolve genes.";

	Properties = TYPE_PART | PROP_NEUTPENETRATE ;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.tmpcity[3] = BCTR::START_LIFE;

	// Element properties here
	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS) {
	// Dead, dissolvable by SOAP
	if (parts[i].tmp3 != 2) {
		if (parts[i].temp < 273.15f) {
			parts[i].ctype = PT_BCTR;
			sim->part_change_type(i, x, y, PT_ICEI);
			return 0;
		}
	}
	//	int r, rx, ry;
	//	for (rx = -1; rx < 2; rx++)
	//	for (ry = -1; ry < 2; ry++)
	//		if (x>=0 && y>=0 && x<XRES && y<YRES) {
	//			r = pmap[y + ry][x + rx];
	//			// Burn
	//			if (TYP(r) == PT_FIRE || TYP(r) == PT_PLSM) {
	//				parts[i].tmpcity[3] = sim->rng.between(0, 200);
	//				parts[i].temp += 50.0f;
	//				sim->part_change_type(i, parts[i].x, parts[i].y, PT_FIRE);
	//				return 0;
	//			}

	//			if ((TYP(r) == PT_SALT || TYP(r) == PT_SOAP) && sim->rng.chance(1, 100) == 1) {
	//				sim->kill_part(i);
	//				return 0;
	//			}
	//		}
	//	return 0;
	//}

	

	int restype, resval, foodtype, metabolism, spread, move, graphics, mutrate; bool glow;
	BCTR::extract_genes(parts[i].ctype, restype, resval, foodtype, metabolism, spread, move, graphics, glow, mutrate);

	// std::cout << restype << " res val" << resval << " food: " << foodtype << " " << metabolism << " spread: " <<
	//	spread << " move: " << move << " graphics:" << graphics << " glow:" << glow << "\n";

	// Temp line
	metabolism = 7;

	int energy_capcity = 2 * BCTR::START_LIFE + (15 - metabolism) * BCTR::ENERGY_MULTI;

	// Inc age, randomize slightly
	if (sim->rng.chance(1, 20) != 1)
		parts[i].tmp++;

	// Utilize energy
	if (sim->currentTick % 50 == 0 && parts[i].tmp3 != 2) {
		int energy_consumption = metabolism + move + glow + 1; // Can't have 0 energy consumption
		parts[i].tmpcity[3] -= energy_consumption;
	}

	// Die if no food
	if (parts[i].tmpcity[3] <= 0) {
		DIE()
		parts[i].tmpcity[3] = 0;
	}
	// Too old
	if (parts[i].tmp >= (15 - metabolism) * BCTR::AGE_MULTI + BCTR::AGE_BASE) {
		// Replace self with new cell if enough energy
		if (parts[i].tmpcity[3] >= 2 * BCTR::START_LIFE) {
			parts[i].ctype = BCTR::mutate(sim,parts[i].ctype, mutrate);
			parts[i].tmp = 0;
			parts[i].tmpcity[3] -= BCTR::START_LIFE;
			return 0;
		} else DIE()
	}

	// Temperature
	int max_temp = 273.15f + 70.0f + BCTR::TEMP_RES_MULTI * (restype == 1) * resval;
	if (parts[i].temp > max_temp && sim->rng.chance(1, 50))
		DIE()
	int min_temp = 273.15f - BCTR::TEMP_RES_MULTI * (restype == 2) * resval;
	if (parts[i].temp < min_temp) {
		parts[i].tmp = parts[i].ctype;
		parts[i].ctype = PT_BCTR;
		
		sim->part_change_type(i, parts[i].x, parts[i].y, PT_ICEI);
		//DIE()
	}

	int r, rx, ry, rt;
	for (ry = -1; ry < 2; ry++){
	for (rx = -1; rx < 2; rx++) 
		 {
			if (x>=0 && y>=0 && x<XRES && y<YRES) {
				r = pmap[y + ry][x + rx];
				rt = TYP(r);

				// Burn
				/*if ((rt == PT_FIRE || rt == PT_PLSM) && sim->rng.chance(1, 80) && (restype != 1 || resval < 8)) {
					parts[i].tmpcity[3] = sim->rng.between(0, 100);
					parts[i].temp += 10.0f;
					sim->part_change_type(i, parts[i].x, parts[i].y, PT_FIRE);
					return 0;
				}*/

				// Eat other bacteria
				if (foodtype == 5 && TYP(r) == PT_BCTR && sim->rng.chance(1, 200)) {
					int restype2, resval2, foodtype2, metabolism2, spread2, move2, graphics2, mutrate2; bool glow2;
					BCTR::extract_genes(parts[ID(r)].ctype, restype2, resval2, foodtype2, metabolism2, spread2, move2, graphics2, glow2, mutrate2);
					
					if (restype2 != 6 || (restype2 == 6 && sim->rng.between(0, 15) > resval2)) {
						// Glow gene is always passed
						if (glow2) parts[ID(r)].ctype |= 1 << 26;
						EAT(PT_NONE)
						sim->kill_part(ID(r));
					}
				}

				// Movement
				// 1 = stick to solids and powders
				if (r && move == 1 && (sim->elements()[rt].Properties & TYPE_SOLID || sim->elements()[rt].Properties & TYPE_LIQUID || sim->elements()[rt].Properties & TYPE_PART)) {
					parts[i].vx = 0;
					parts[i].vy = 0;
				}
				// 2 = Float through the air
				else if (!r && move == 2 && sim->rng.chance(move, 30)) {
					parts[i].vx += rx;
					parts[i].vy += ry;
				}
				// 3 = Get inside animal cells
				else if (r && move == 3 && sim->rng.chance(move, 2000)) {

					//sim->better_do_swap(i, x, y, ID(r), parts[ID(r)].x, parts[ID(r)].y);
					parts[i].x = parts[ID(r)].x;
					parts[i].y = parts[ID(r)].y;
					pmap[y + ry][x + rx] = PMAP(i, parts[i].type);
					return 1;
				}
				// 4 = Reproduce by injecting DNA into other BCTR
				else if (r && move == 4 && TYP(r) == PT_BCTR && parts[i].tmpcity[3] > BCTR::START_LIFE && sim->rng.chance(1, 100)) {
					parts[i].ctype = BCTR::mutate(sim,parts[i].ctype, parts[i].tmpville[8]);
					parts[ID(r)].ctype = parts[i].ctype;
					parts[i].tmpcity[3] -= BCTR::START_LIFE;
					return 0;
				}

				if (!r || TYP(r) == PT_BCTR) {
					// Reproduce if enough energy stored and spot is empty
					if (parts[i].tmpcity[3] >= 2 * BCTR::START_LIFE) {
						parts[i].tmpcity[3] -= BCTR::START_LIFE;
						int ni = r ? ID(r) : sim->create_part(-1, x + rx, y + ry, PT_BCTR);
						parts[ni].dcolour = parts[i].dcolour;
						parts[ni].ctype = parts[i].ctype;
						parts[ni].tmpville[8] = parts[i].tmpville[8];

						// Mutate new bacteria
						parts[ni].ctype = BCTR::mutate(sim,parts[ni].ctype, parts[ni].tmpville[8]);
					}
					r = sim->photons[y + ry][x + rx];
				}
				if (!r) continue;
				rt = TYP(r);
				if ((rt == PT_BRKN || rt == PT_LQUD)&& parts[ID(r)].ctype)
					rt = parts[ID(r)].ctype;
				

				// Consume food, boost energy
				// Metabolism is % chance of eating / 5
				if (parts[i].tmpcity[3] < energy_capcity && sim->rng.between(0, 500) < metabolism) {

					if (((foodtype == 5 && (rt == PT_YEST || rt == PT_DYST || rt
						== PT_WOOD || rt == PT_PLNT || rt == PT_SAWD)) || (foodtype == 4 && (rt == PT_NEUT || rt == PT_PROT)) || (foodtype == 2 && sim->rng.chance(1, 25))) && sim->rng.chance(1, 50))
					
						EAT(PT_GAS)
					else if (foodtype == 3 && rt == PT_WSTE) {
						EAT(PT_WATR)
							parts[i].temp += 2.0f;
					}
					else if (foodtype == 6 && (rt == PT_OIL || rt == PT_GAS || rt == PT_DESL || rt == PT_WAX || rt == PT_MWAX) && sim->rng.chance(1, 50)) { // eat hydrocarbons
					/*	parts[i].temp += 0.5f;
						parts[ID(r)].temp -= 0.5f;
						parts[i].tmpcity[3] += BCTR::TEMP_LIFE_GAIN;*/
						EAT(PT_WSTE)
							parts[i].temp += 2.0f;

					}
					else if ((rt == PT_WATR || rt == PT_SUGR || rt == PT_MILK) && sim->rng.chance(1, 50))
						EAT(PT_GAS)
						parts[i].temp += 2.0f;
					}
				

				// Determine resistance against other elements
				// Active resistances
				bool should_res = sim->rng.between(0, 15) <= resval;
				if ((rt == PT_VRSS || rt == PT_VRSG || rt == PT_VIRS) && restype == 3 && should_res && sim->rng.chance(1, 20)) // VIRS
					parts[ID(r)].tmp3 = 1;
				else if ((rt == PT_ACID || rt == PT_HCL) && restype == 5 && should_res && sim->rng.chance(1, 20)) // ACID
					sim->kill_part(ID(r));

				// Passive resistances
				else if ((rt == PT_SALT || rt == PT_SOAP) && sim->rng.chance(1, 20) && (restype != 4 || (restype == 4 && !should_res)))
					DIE()

				// Radiation causes mutations, resistance decreases change
				else if ((rt == PT_PROT || rt == PT_NEUT || rt == PT_ELEC) &&
						sim->rng.chance(1, 10) && (restype != 7 || (restype == 7 && !should_res)))
					parts[i].ctype = BCTR::mutate(sim,parts[i].ctype, parts[i].tmpville[8]);

				// Dies when touching BRASR
				else if (rt == PT_BRAS && sim->rng.chance(1, 20))
					DIE()
			}
		}
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS) {
	*pixel_mode |= PMODE_BLUR;

	// Dead
	if (cpart->tmp3 == 2) {
		*colr *= 0.8; *colg *= 0.8; *colb *= 0.8;
		*colr += (int)(cpart->tmp) % 21 - 10;
		*colg += (int)(cpart->tmp2) % 21 - 10;
		*colb += (int)(cpart->tmp3) % 21 - 10;
		return 0;
	}

	// Color vary a bit depending on mutation
	*colr = (*colr + (cpart->ctype ^ 9999) % 255) / 2;
	*colg = (*colg + 255 - *colr) / 2;
	*colb = (*colb + 255 - (cpart->ctype ^ 999999) % 255) / 2;

	// Graphical effectss
	int graphics = BCTR::extract_bits(cpart->ctype, 23, 25);
	if (graphics == 1) *pixel_mode = PMODE_SPARK;
	else if (graphics == 2) *pixel_mode = PMODE_BLOB;
	else if (graphics == 3) *pixel_mode = PMODE_FLAT;

	// Glow in the dark
	int glow = BCTR::extract_bits(cpart->ctype, 26, 26);
	if (glow) {
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*firea = 240;
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
