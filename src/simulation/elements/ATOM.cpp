#include "simulation/ElementCommon.h"
#include "simulation/NuclearProperties.cpp"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_ATOM()
{
	Identifier = "DEFAULT_PT_ATOM";
	Name = "ATOM";
	Colour = 0xBB99FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.20f;
	Collision = 0.0f;
	Gravity = -0.1f;
	Diffusion = 0.30f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = 8273.15;
	HeatConduct = 5;
	Description = "Plasma, extremely hot.";

	Properties = TYPE_GAS;

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

static int update(UPDATE_FUNC_ARGS) {
	//ctype = energy, life = Z, tmp = A

	//parts[i].tmp3 = BindingEnergy(parts[i].life, parts[i].tmp); //binding energy 

	//Z can't be higher than A
	if (parts[i].life > parts[i].tmp)
		parts[i].life = parts[i].tmp;


	//Heat Sponge
	if (parts[i].temp > 10273.15)
	{
		parts[i].ctype += 10;
		parts[i].temp -= 1000;
	}
	else if (parts[i].temp > 8373.15)
	{
		parts[i].ctype += 1;
		parts[i].temp -= 100;
	}
	else if (parts[i].temp < 8273.15 && parts[i].ctype > 0)
	{
		parts[i].ctype -= 1;
		parts[i].temp += 100;
	}

	int Energy = parts[i].ctype, Z = parts[i].life, A = parts[i].tmp, N = A - Z;
	double BE = BindingEnergy(Z, A);
	parts[i].tmp3 = BindingEnergy(Z, A);
	//parts[i].tmp3 = CoulombBarrier(2, 4, 2, 4);


	//Nuclear interactions
	int r, rx, ry, rt, np;
	for (rx = -2; rx < 3; rx++)
		for (ry = -2; ry < 3; ry++)
			if ((rx || ry) && x+rx>=0 && y+ry>=0 && x+rx<XRES && y+ry<YRES)
			{
				r = pmap[y + ry][x + rx];
				if (!r)
				{
					//Decay
					//beta -   electron
					if (BE < BindingEnergy(Z + 1, A) && sim->rng.between(0, 100) == 1) //missing rate control
					{
						sim->create_part(-1, x + rx, y + ry, PT_ELEC);
						parts[i].life = Z += 1;
					}

					//beta +   positron

					else if (BE < BindingEnergy(Z - 1, A) && parts[i].life >= 1 && sim->rng.between(0, 100)==1)
					{
						sim->create_part(-1, x + rx, y + ry, PT_PHOT);
						parts[i].life = Z -= 1;
					}

					//alpha  He-4
					else if (BE < BindingEnergy(Z - 2, A - 4) + BindingEnergy(2, 4) && parts[i].tmp >=4 && sim->rng.between(0, 100) == 1)
					{
						np = sim->create_part(-1, x + rx, y + ry, PT_ATOM);
						parts[np].life = 2;
						parts[np].tmp = 4;
						//parts[np].ctype = something, have to set decay energies
						//parts[i].ctype += (BindingEnergy(Z - 2, A - 4)+BindingEnergy(2, 4)-BE) * 1000000 / JtoeV / KtoJ/100;

						parts[i].ctype += ((BindingEnergy(Z - 2, A - 4) + BindingEnergy(2, 4) - BE) * 1000000 / JtoeV + 0*CoulombBarrier(Z-2, A-4, 2, 4)) / KtoJ / 100;


						parts[i].life = Z -= 2;
						parts[i].tmp = A -= 4;
					}



					//BindingEnergy(Z - 2, A - 4) + BindingEnergy(2, 4) - BE > 0
					//products - reagent > 0



					/*
					
--decays
--a
if BindingEnergy(Z-2,A-4)/(A-4)-BE/A > 0 and tpt.get_property("tmp",i) > 4 then
if tpt.get_property("type",x+collx,y+colly) == tpt.element('none') then
tpt.create(x+collx,y+colly,"atom")
tpt.set_property("life",2,x+collx,y+colly)
tpt.set_property("tmp",4,x+collx,y+colly)
tpt.set_property("life",tpt.get_property("life",i)-2,i)
tpt.set_property("tmp",tpt.get_property("tmp",i)-4,i)
end
end
end
					*/

				}
				if (TYP(r) == PT_ATOM)
				{
					if (parts[ID(r)].life >= 0 && parts[ID(r)].tmp > 0) //another valid atom
					{
						int tEnergy = parts[ID(r)].ctype, tZ = parts[ID(r)].life, tA = parts[ID(r)].tmp;
						double tBE = BindingEnergy(tZ, tA);			//BE of the target
						double pBE = BindingEnergy(Z + tZ, A + tA); //BE of the product
						double coulombBarrier = CoulombBarrier(Z, A, tZ, tA);




						//energy transfer, to simulate thermal equilibrium
						//also makes fusion much easier to achieve
						if (Energy > 0 || tEnergy > 0)
						{
							unsigned int transfE = Energy + tEnergy;
							if (transfE <= pow(2, 31) - 1)
							{
								transfE = sim->rng.between(0, Energy + tEnergy);
								parts[ID(r)].ctype = tEnergy = Energy + tEnergy - transfE;
								parts[i].ctype = Energy = transfE;
							}
						}






						//there's enough energy to overcome the coulomb barrier and fuse the nuclei
						if (((Energy + tEnergy) * 100) * KtoJ >= coulombBarrier)
						{

							double BEdifference = BindingEnergy(Z + tZ, A + tA) - (BindingEnergy(Z, A) + BindingEnergy(tZ, tA));
							if (BEdifference > 0) //Exothermic Fusion
							{
								parts[i].ctype = floor(((Energy + tEnergy) * 100 + (BEdifference * 1000000 / JtoeV - coulombBarrier) / KtoJ) / 100);
								parts[i].life += tZ;
								parts[i].tmp += tA;
								sim->kill_part(ID(r));
							}

							else if (BEdifference < 0) //Endothermic Fusion
								//enough energy to pass coulomb barrier and increase mass
								if (((Energy + tEnergy) * 100) * KtoJ >= coulombBarrier - BEdifference * 1000000 / JtoeV)
								{
									parts[i].ctype = Energy = floor(((Energy + tEnergy) * 100 + (BEdifference * 1000000 / JtoeV - coulombBarrier) / KtoJ) / 100);
									parts[i].life += tZ;
									parts[i].tmp += tA;
									sim->kill_part(ID(r));
								}


						}


					}
				}




			}



	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	if (cpart->life == 0 && cpart->tmp == 1)
	{
		*colr = 32;
		*colg = 224;
		*colb = 255;
	}
	else if (cpart->life == 1 && cpart->tmp == 1)
	{
		*colr = 153;
		*colg = 0;
		*colb = 0;
	}
	else if (cpart->life == 1)
	{
		*colr = 80;
		*colg = 112;
		*colb = 255;
	}
	else if (cpart->life == 2 && cpart->tmp == 4)
	{
		*colr = 190;
		*colg = 190;
		*colb = 15;
	}
	else
	{
		int caddress = restrict_flt((int)(cpart->tmp) % 51 + 50, 0.0f, (200.0f * 3) - 3);
		*colr = (unsigned char)(caddress * 3);
		*colg = (unsigned char)(caddress * 3 + 1);
		*colb = (unsigned char)(caddress * 3 + 2);
	}

	*firea = 255;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	*pixel_mode = PMODE_GLOW | PMODE_ADD; //Clear default, don't draw pixel
	*pixel_mode |= FIRE_ADD;
	//Returning 0 means dynamic, do not cache
	return 0;
}

static void create(ELEMENT_CREATE_FUNC_ARGS) {
	sim->parts[i].life = 1;
	sim->parts[i].tmp = 1;
}
