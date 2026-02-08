#include "OptionsModel.h"
#include "OptionsView.h"
#include "simulation/Simulation.h"
#include "simulation/SimulationData.h"
#include "simulation/Air.h"
#include "simulation/gravity/Gravity.h"
#include "prefs/GlobalPrefs.h"
#include "common/clipboard/Clipboard.h"
#include "gui/interface/Engine.h"
#include "gui/game/GameModel.h"
#include "client/Client.h"

OptionsModel::OptionsModel(GameModel * gModel_) {
	gModel = gModel_;
	sim = gModel->GetSimulation();
}

void OptionsModel::AddObserver(OptionsView* view)
{
	observers.push_back(view);
	view->NotifySettingsChanged(this);
}

bool OptionsModel::GetHeatSimulation()
{
	return sim->legacy_enable?false:true;
}

void OptionsModel::SetHeatSimulation(bool state)
{
	sim->legacy_enable = state?0:1;
	notifySettingsChanged();
}

bool OptionsModel::GetAmbientHeatSimulation()
{
	return sim->aheat_enable?true:false;
}

void OptionsModel::SetAmbientHeatSimulation(bool state)
{
	sim->aheat_enable = state?1:0;
	notifySettingsChanged();
}

bool OptionsModel::GetNewtonianGravity()
{
	return bool(sim->grav);
}

void OptionsModel::SetNewtonianGravity(bool state)
{
	sim->EnableNewtonianGravity(state);
	notifySettingsChanged();
}

bool OptionsModel::GetWaterEqualisation()
{
	return sim->water_equal_test?true:false;
}


void OptionsModel::SetWaterEqualisation(bool state)
{
	sim->water_equal_test = state?1:0;
	notifySettingsChanged();
}

bool OptionsModel::GetNoWeightSwitching()
{
	return sim->NoWeightSwitching ? true : false;
}

void OptionsModel::SetNoWeightSwitching(bool state)
{
	sim->NoWeightSwitching = state?1:0;
	notifySettingsChanged();
	auto &sd = SimulationData::Ref();
	sd.init_can_move();
}

bool OptionsModel::GetBetterBurningEnable()
{
	return sim->betterburning_enable != 0;
}

void OptionsModel::SetBetterBurningEnable(bool state)
{
	sim->betterburning_enable = state ? 1 : 0;
	notifySettingsChanged();
}
bool OptionsModel::GetAtmosphericPressure()
{
	return sim->air->useAtmosphericPressure;
}
void OptionsModel::SetAtmosphericPressure(bool state)
{
	sim->air->useAtmosphericPressure = state;
	const float R_gas = 287.0f;
	const float P_atm = 101325.0f;
	const float P_kpa = 1000.0f;
	// Only convert cells that are at the *old* baseline, so we don't reset the whole map.
	// Switching to kPa: only cells at 1 atm → 1 kPa. Switching to atm: only cells at 1 kPa → 1 atm.
	const float P_old = state ? P_kpa : P_atm;   // old baseline (we're leaving it)
	const float P_new = state ? P_atm : P_kpa;   // new baseline (we're switching to it)
	const float tol = state ? 150.0f : 2000.0f; // tolerance: ±150 Pa for 1 kPa, ±5 kPa for 1 atm
	float newDensity = P_new / (R_gas * sim->air->ambientAirTemp);
	for (int y = 0; y < YCELLS; y++)
	{
		for (int x = 0; x < XCELLS; x++)
		{
			if (sim->air->bmap_blockair[y][x])
				continue;
			float p = sim->pv[y][x];
			if (p >= P_old - tol && p <= P_old + tol)
			{
				sim->pv[y][x] = P_new;
				sim->air->rho[y][x] = newDensity;
			}
		}
	}
	notifySettingsChanged();
}

bool OptionsModel::GetPressureBreakEnabled()
{
	return sim->air->enablePressureBreak;
}

void OptionsModel::SetPressureBreakEnabled(bool state)
{
	sim->air->enablePressureBreak = state;
	notifySettingsChanged();
}

OptionsModel::PressureUnit OptionsModel::GetPressureUnit()
{
	int unit = GlobalPrefs::Ref().Get("Simulation.PressureUnit", int(PRESSURE_KPA));
	return PressureUnit(unit);
}

void OptionsModel::SetPressureUnit(PressureUnit unit)
{
	GlobalPrefs::Ref().Set("Simulation.PressureUnit", int(unit));
	notifySettingsChanged();
}


int OptionsModel::GetAirMode()
{
	return sim->air->airMode;
}
void OptionsModel::SetAirMode(int airMode)
{
	sim->air->airMode = airMode;
	notifySettingsChanged();
}

int OptionsModel::GetAirSolverStepsPerFrame()
{
	return sim->air->airSolverStepsPerFrame;
}
void OptionsModel::SetAirSolverStepsPerFrame(int steps)
{
	if (steps < 1) steps = 1;
	if (steps > 128) steps = 128;
	sim->air->airSolverStepsPerFrame = steps;
	notifySettingsChanged();
}

int OptionsModel::GetEdgeMode()
{
	return gModel->GetSimulation()->edgeMode;
}
void OptionsModel::SetEdgeMode(int edgeMode)
{
	GlobalPrefs::Ref().Set("Simulation.EdgeMode", edgeMode);
	gModel->SetEdgeMode(edgeMode);
	notifySettingsChanged();
}

TempScale OptionsModel::GetTemperatureScale()
{
	return gModel->GetTemperatureScale();
}
void OptionsModel::SetTemperatureScale(TempScale temperatureScale)
{
	GlobalPrefs::Ref().Set("Renderer.TemperatureScale", int(temperatureScale));
	gModel->SetTemperatureScale(temperatureScale);
	notifySettingsChanged();
}

int OptionsModel::GetThreadedRendering()
{
	return gModel->GetThreadedRendering();
}

void OptionsModel::SetThreadedRendering(bool newThreadedRendering)
{
	GlobalPrefs::Ref().Set("Renderer.SeparateThread", newThreadedRendering);
	gModel->SetThreadedRendering(newThreadedRendering);
	notifySettingsChanged();
}

float OptionsModel::GetAmbientAirTemperature()
{
	return gModel->GetSimulation()->air->ambientAirTemp;
}
void OptionsModel::SetAmbientAirTemperature(float ambientAirTemp)
{
	GlobalPrefs::Ref().Set("Simulation.AmbientAirTemp", ambientAirTemp);
	gModel->SetAmbientAirTemperature(ambientAirTemp);
	notifySettingsChanged();
}

float OptionsModel::GetVorticityCoeff()
{
	return gModel->GetSimulation()->air->vorticityCoeff;
}
void OptionsModel::SetVorticityCoeff(float vorticityCoeff)
{
	GlobalPrefs::Ref().Set("Simulation.VorticityCoeff", vorticityCoeff);
	gModel->SetVorticityCoeff(vorticityCoeff);
	notifySettingsChanged();
}

int OptionsModel::GetGravityMode()
{
	return sim->gravityMode;
}
void OptionsModel::SetGravityMode(int gravityMode)
{
	sim->gravityMode = gravityMode;
	notifySettingsChanged();
}

float OptionsModel::GetCustomGravityX()
{
	return sim->customGravityX;
}

void OptionsModel::SetCustomGravityX(float x)
{
	sim->customGravityX = x;
	notifySettingsChanged();
}

float OptionsModel::GetCustomGravityY()
{
	return sim->customGravityY;
}

void OptionsModel::SetCustomGravityY(float y)
{
	sim->customGravityY = y;
	notifySettingsChanged();
}

int OptionsModel::GetScale()
{
	return ui::Engine::Ref().GetScale();
}

void OptionsModel::SetScale(int scale)
{
	ui::Engine::Ref().SetScale(scale);
	GlobalPrefs::Ref().Set("Scale", int(scale));
	notifySettingsChanged();
}

bool OptionsModel::GetGraveExitsConsole()
{
	return ui::Engine::Ref().GraveExitsConsole;
}

void OptionsModel::SetGraveExitsConsole(bool graveExitsConsole)
{
	ui::Engine::Ref().GraveExitsConsole = graveExitsConsole;
	GlobalPrefs::Ref().Set("GraveExitsConsole", graveExitsConsole);
	notifySettingsChanged();
}

bool OptionsModel::GetNativeClipoard()
{
	return Clipboard::GetEnabled();
}

void OptionsModel::SetNativeClipoard(bool nativeClipoard)
{
	Clipboard::SetEnabled(nativeClipoard);
	GlobalPrefs::Ref().Set("NativeClipboard.Enabled", nativeClipoard);
	notifySettingsChanged();
}

bool OptionsModel::GetResizable()
{
	return ui::Engine::Ref().GetResizable();
}

void OptionsModel::SetResizable(bool resizable)
{
	ui::Engine::Ref().SetResizable(resizable);
	GlobalPrefs::Ref().Set("Resizable", resizable);
	notifySettingsChanged();
}

bool OptionsModel::GetFullscreen()
{
	return ui::Engine::Ref().GetFullscreen();
}
void OptionsModel::SetFullscreen(bool fullscreen)
{
	ui::Engine::Ref().SetFullscreen(fullscreen);
	GlobalPrefs::Ref().Set("Fullscreen", fullscreen);
	notifySettingsChanged();
}

bool OptionsModel::GetChangeResolution()
{
	return ui::Engine::Ref().GetChangeResolution();
}

void OptionsModel::SetChangeResolution(bool newChangeResolution)
{
	ui::Engine::Ref().SetChangeResolution(newChangeResolution);
	GlobalPrefs::Ref().Set("AltFullscreen", newChangeResolution);
	notifySettingsChanged();
}

bool OptionsModel::GetForceIntegerScaling()
{
	return ui::Engine::Ref().GetForceIntegerScaling();
}

void OptionsModel::SetForceIntegerScaling(bool forceIntegerScaling)
{
	ui::Engine::Ref().SetForceIntegerScaling(forceIntegerScaling);
	GlobalPrefs::Ref().Set("ForceIntegerScaling", forceIntegerScaling);
	notifySettingsChanged();
}

bool OptionsModel::GetBlurryScaling()
{
	return ui::Engine::Ref().GetBlurryScaling();
}

void OptionsModel::SetBlurryScaling(bool newBlurryScaling)
{
	ui::Engine::Ref().SetBlurryScaling(newBlurryScaling);
	GlobalPrefs::Ref().Set("BlurryScaling", newBlurryScaling);
	notifySettingsChanged();
}

bool OptionsModel::GetFastQuit()
{
	return ui::Engine::Ref().GetFastQuit();
}
void OptionsModel::SetFastQuit(bool fastquit)
{
	ui::Engine::Ref().SetFastQuit(fastquit);
	GlobalPrefs::Ref().Set("FastQuit", bool(fastquit));
	notifySettingsChanged();
}

bool OptionsModel::GetGlobalQuit()
{
	return ui::Engine::Ref().GetGlobalQuit();
}
void OptionsModel::SetGlobalQuit(bool newGlobalQuit)
{
	ui::Engine::Ref().SetGlobalQuit(newGlobalQuit);
	GlobalPrefs::Ref().Set("GlobalQuit", newGlobalQuit);
	notifySettingsChanged();
}

int OptionsModel::GetDecoSpace()
{
	return gModel->GetDecoSpace();
}
void OptionsModel::SetDecoSpace(int decoSpace)
{
	GlobalPrefs::Ref().Set("Simulation.DecoSpace", decoSpace);
	gModel->SetDecoSpace(decoSpace);
	notifySettingsChanged();
}

bool OptionsModel::GetShowAvatars()
{
	return ui::Engine::Ref().ShowAvatars;
}

void OptionsModel::SetShowAvatars(bool state)
{
	ui::Engine::Ref().ShowAvatars = state;
	GlobalPrefs::Ref().Set("ShowAvatars", state);
	notifySettingsChanged();
}

bool OptionsModel::GetMouseClickRequired()
{
	return gModel->GetMouseClickRequired();
}

void OptionsModel::SetMouseClickRequired(bool mouseClickRequired)
{
	GlobalPrefs::Ref().Set("MouseClickRequired", mouseClickRequired);
	gModel->SetMouseClickRequired(mouseClickRequired);
	notifySettingsChanged();
}

bool OptionsModel::GetIncludePressure()
{
	return gModel->GetIncludePressure();
}

void OptionsModel::SetIncludePressure(bool includePressure)
{
	GlobalPrefs::Ref().Set("Simulation.IncludePressure", includePressure);
	gModel->SetIncludePressure(includePressure);
	notifySettingsChanged();
}

bool OptionsModel::GetPerfectCircle()
{
	return gModel->GetPerfectCircle();
}

void OptionsModel::SetPerfectCircle(bool perfectCircle)
{
	GlobalPrefs::Ref().Set("PerfectCircleBrush", perfectCircle);
	gModel->SetPerfectCircle(perfectCircle);
	notifySettingsChanged();
}

bool OptionsModel::GetMomentumScroll()
{
	return ui::Engine::Ref().MomentumScroll;
}

void OptionsModel::SetMomentumScroll(bool state)
{
	GlobalPrefs::Ref().Set("MomentumScroll", state);
	ui::Engine::Ref().MomentumScroll = state;
	notifySettingsChanged();
}

bool OptionsModel::GetRedirectStd()
{
	return Client::Ref().GetRedirectStd();
}

void OptionsModel::SetRedirectStd(bool newRedirectStd)
{
	GlobalPrefs::Ref().Set("RedirectStd", newRedirectStd);
	Client::Ref().SetRedirectStd(newRedirectStd);
	notifySettingsChanged();
}
bool OptionsModel::GetAutoStartupRequest()
{
	return Client::Ref().GetAutoStartupRequest();
}

void OptionsModel::SetAutoStartupRequest(bool newAutoStartupRequest)
{
	GlobalPrefs::Ref().Set("AutoStartupRequest", newAutoStartupRequest);
	Client::Ref().SetAutoStartupRequest(newAutoStartupRequest);
	notifySettingsChanged();
}

void OptionsModel::notifySettingsChanged()
{
	for (size_t i = 0; i < observers.size(); i++)
	{
		observers[i]->NotifySettingsChanged(this);
	}
}

OptionsModel::~OptionsModel() {
}

