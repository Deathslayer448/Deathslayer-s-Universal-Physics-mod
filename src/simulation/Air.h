#pragma once
#include "SimulationConfig.h"
#include "AirSolverWrapper.h"

class Simulation;
struct RenderableSimulation;

class Air
{
public:
	Simulation & sim;
	AirSolverWrapper rusanovSolver;
	int airMode;
	int airSolverStepsPerFrame;  // max steps per display frame (1 = slow/stable, higher = faster sim)
	float ambientAirTemp;
	float vorticityCoeff;
	bool useAtmosphericPressure; // UI display mode: if true, show pressure relative to atmospheric
	float ovx[YCELLS][XCELLS];
	float ovy[YCELLS][XCELLS];
	float opv[YCELLS][XCELLS];
	float ohv[YCELLS][XCELLS]; // Ambient Heat
	float rho[YCELLS][XCELLS]; // Density (kg/m³) - for ideal gas law P = ρRT
	unsigned char bmap_blockair[YCELLS][XCELLS];
	unsigned char bmap_blockairh[YCELLS][XCELLS];
	float kernel[9];
	void make_kernel(void);
	static float vorticity(const RenderableSimulation & sm, int y, int x);
	void update_airh(void);
	void update_air(void);
	void Clear();
	void ClearAirH();
	void Invert();
	void ApproximateBlockAirMaps();
	Air(Simulation & sim);
};
