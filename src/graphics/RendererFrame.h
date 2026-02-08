#pragma once
#include "SimulationConfig.h"
#include "Pixel.h"
#include "common/Plane.h"
#include <array>

constexpr auto RendererFrameSize = Vec2<int>{ WINDOW.X, RES.Y };
using RendererFrame = PlaneAdapter<std::array<pixel, WINDOW.X * RES.Y>, RendererFrameSize.X, RendererFrameSize.Y>;

struct RendererStats
{
	int foundParticles = 0;
	float hdispLimitMin = 0;
	float hdispLimitMax = 0;
	bool hdispLimitValid = false;
	// Normalized air overlay L/H (like heat): actual min/max on map for status line.
	float airNormPressureMin = 0;
	float airNormPressureMax = 0;
	bool airNormPressureValid = false;
	float airNormViscosityMin = 0;
	float airNormViscosityMax = 0;
	bool airNormViscosityValid = false;
	float airNormVelocityMin = 0;
	float airNormVelocityMax = 0;
	bool airNormVelocityValid = false;
};
