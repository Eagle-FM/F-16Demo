#ifndef _F16ELECTRICSYSTEM_H_
#define _F16ELECTRICSYSTEM_H_

#include "ED_FM_Utility.h"		// Provided utility functions that were in the initial EFM example
#include "F16Constants.h"		// Common constants used throughout this DLL

#include "Electrics/F16AcDcConverter.h"
#include "Electrics/F16Battery.h"
#include "Electrics/F16ElectricBus.h"
#include "Electrics/F16MainGenerator.h"

/*
sources:
- NASA TP 2857

*/

class F16ElectricSystem
{
protected:
	F16Battery battery;

	// bit too generic: FC3 style use only
	bool electricsOnOff;

	// type: AC no 1, AC no 2, DC "battery" bus
	F16ElectricBus AcNo1;
	F16ElectricBus AcNo2;
	F16ElectricBus DcBat;

public:
	F16ElectricSystem() 
		: battery()
		, electricsOnOff(false)
	{}
	~F16ElectricSystem() {}

	// FC3 style electric power on/off ("all in one")
	void toggleElectrics()
	{
		electricsOnOff = !electricsOnOff;
	}
	void setElectricsOnOff(bool status)
	{
		electricsOnOff = status;
	}
	void toggleBatteryOnOff()
	{
	}

	// update with engine/APU rpm/torque
	// and power consumption
	void updateFrame(const double frameTime)
	{
	}

};

#endif // ifndef _F16ELECTRICSYSTEM_H_

