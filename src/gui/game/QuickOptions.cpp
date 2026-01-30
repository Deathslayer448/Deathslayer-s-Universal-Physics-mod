#include "QuickOptions.h"

#include "GameModel.h"
#include "GameController.h"
#include "gui/options/OptionsModel.h"

#include "simulation/Simulation.h"

SandEffectOption::SandEffectOption(GameModel * m):
QuickOption("P", "Sand effect", m, Toggle)
{

}
bool SandEffectOption::GetToggle()
{
	return m->GetSimulation()->pretty_powder;
}
void SandEffectOption::perform()
{
	m->GetSimulation()->pretty_powder = !m->GetSimulation()->pretty_powder;
}



DrawGravOption::DrawGravOption(GameModel * m):
QuickOption("G", "Draw gravity field \bg(ctrl+g)", m, Toggle)
{

}
bool DrawGravOption::GetToggle()
{
	return m->GetGravityGrid();
}
void DrawGravOption::perform()
{
	m->ShowGravityGrid(!m->GetGravityGrid());
}



DecorationsOption::DecorationsOption(GameModel * m):
QuickOption("D", "Draw decorations \bg(ctrl+b)", m, Toggle)
{

}
bool DecorationsOption::GetToggle()
{
	return m->GetDecoration();
}
void DecorationsOption::perform()
{
	m->SetDecoration(!m->GetDecoration());
}



NGravityOption::NGravityOption(GameModel * m):
QuickOption("N", "Newtonian Gravity \bg(n)", m, Toggle)
{

}
bool NGravityOption::GetToggle()
{
	return m->GetNewtonianGrvity();
}
void NGravityOption::perform()
{
	m->SetNewtonianGravity(!m->GetNewtonianGrvity());
}

BurningOption::BurningOption(GameModel* m):
QuickOption("B", "Better burning", m, Toggle)
{

}
bool BurningOption::GetToggle()
{
	return m->GetBetterBurningEnable();
}
void BurningOption::perform()
{
	m->SetBetterBurningEnable(!m->GetBetterBurningEnable());
}

NoWeightOption::NoWeightOption(GameModel* m):
QuickOption("W", "Disable weight switching", m, Toggle)
{

}
bool NoWeightOption::GetToggle()
{
	return m->GetNoWeightSwitching();
}
void NoWeightOption::perform()
{
	m->SetNoWeightSwitching(!m->GetNoWeightSwitching());
}

PressureUnitOption::PressureUnitOption(GameModel* m):
QuickOption("P", "Pressure unit: atm/kPa", m, Toggle)
{

}
bool PressureUnitOption::GetToggle()
{
	// Toggle state: true = kPa, false = atm
	return m->GetPressureUnit() == OptionsModel::PRESSURE_KPA;
}
void PressureUnitOption::perform()
{
	// Toggle between atm and kPa
	if (m->GetPressureUnit() == OptionsModel::PRESSURE_ATM)
		m->SetPressureUnit(OptionsModel::PRESSURE_KPA);
	else
		m->SetPressureUnit(OptionsModel::PRESSURE_ATM);
}

AHeatOption::AHeatOption(GameModel * m):
QuickOption("A", "Ambient heat \bg(u)", m, Toggle)
{

}
bool AHeatOption::GetToggle()
{
	return m->GetAHeatEnable();
}
void AHeatOption::perform()
{
	m->SetAHeatEnable(!m->GetAHeatEnable());
}



ConsoleShowOption::ConsoleShowOption(GameModel * m, GameController * c_):
QuickOption("C", "Show Console \bg(~)", m, Toggle)
{
	c = c_;
}
bool ConsoleShowOption::GetToggle()
{
	return 0;
}
void ConsoleShowOption::perform()
{
	c->ShowConsole();
}
