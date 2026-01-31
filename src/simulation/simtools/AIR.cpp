#include "simulation/ToolCommon.h"
#include "simulation/Air.h"

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength);

void SimTool::Tool_AIR()
{
	Identifier = "DEFAULT_TOOL_AIR";
	Name = "AIR";
	Colour = 0xFFFFFF_rgb;
	Description = "Air, creates airflow and pressure.";
	Perform = &perform;
}

static int perform(SimTool *tool, Simulation * sim, Particle * cpart, int x, int y, int brushX, int brushY, float strength)
{
	// Strength 1.0 adds ~0.05 atm per application (pv in Pascals). Gentle; many clicks to build pressure.
	sim->pv[y/CELL][x/CELL] += strength * 5000.0f;

	if (sim->pv[y/CELL][x/CELL] > MAX_PRESSURE)
		sim->pv[y/CELL][x/CELL] = MAX_PRESSURE;
	else if (sim->pv[y/CELL][x/CELL] < MIN_PRESSURE)
		sim->pv[y/CELL][x/CELL] = MIN_PRESSURE;
	return 1;
}
