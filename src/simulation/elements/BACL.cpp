#include "simulation/ElementCommon.h"

//int Element_WSTE_update(UPDATE_FUNC_ARGS);
//int Element_WSTE_graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_BACL() {
	Identifier = "DEFAULT_PT_BACL";
	Name = "BACL";
	Colour = 0x333333_rgb;
	MenuVisible = 0;
	MenuSection = SC_POWDERS;
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
	Meltable = 0;
	Hardness = 2;
	PhotonReflectWavelengths = 0x00000000;

	Weight = 90;

	HeatConduct = 150;
	Description = "Broken Anti Coal.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	DefaultProperties.life = 110;

	//Update = &Element_WSTE_update;
	//Graphics = &Element_WSTE_graphics;
}
