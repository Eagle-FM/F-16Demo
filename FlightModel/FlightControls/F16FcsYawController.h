#ifndef _F16FCSYAWCONTROLLER_H_
#define _F16FCSYAWCONTROLLER_H_

#include <cmath>

#include "UtilityFunctions.h"

#include "F16FcsCommon.h"
#include "F16Actuator.h"


class F16FcsYawController
{
protected:
	F16BodyState *bodyState;
	F16FlightSurface *flightSurface;
	F16TrimState *trimState;

	Limiter<double>		rudderLimiter;

	// F8
	//LinearFunction<double> yawAxis;

protected:
	double getRudderCommand(const double pedInput) const
	{
		double rudderForceCommand = pedInput * 450.0;

		double rudderCommand = 0.0;
		if (abs(rudderForceCommand) < 44.0)
		{
			rudderCommand = 0.0;
		}
		else if (rudderForceCommand >= 44.0)
		{
			rudderCommand = -0.0739 * rudderForceCommand + 3.2512;
		}
		else if (rudderForceCommand <= -44.0)
		{
			rudderCommand = -0.0739 * rudderForceCommand - 3.2512;
		}

		return rudderLimiter.limit(rudderCommand);
	}

public:
	F16FcsYawController() :
		bodyState(nullptr),
		flightSurface(nullptr),
		trimState(nullptr),
		rudderLimiter(-30, 30) // deflection limit
		//yawAxis(1, 2.04, 3.23, 0.5, 1)
	{}
	~F16FcsYawController() {}

	void setRef(F16BodyState *bs, F16FlightSurface *fs, F16TrimState *ts)
	{
		bodyState = bs;
		flightSurface = fs;
		trimState = ts;
	}

	// Controller for yaw
	void fcsCommand(double pedInput, double alphaFiltered)
	{
		const double roll_rate = bodyState->getRollRateDegs();
		const double yaw_rate = bodyState->getYawRateDegs();

		double rudderCommand = getRudderCommand(pedInput);
		double rudderCommandFilteredWTrim = trimState->trimYaw - rudderCommand;

		double alphaGained = alphaFiltered * (1.0 / 57.3);
		double rollRateWithAlpha = roll_rate * alphaGained;
		double yawRateWithRoll = yaw_rate - rollRateWithAlpha;

		// TODO: use flightSurface->roll_Command instead?
		// should get roll side and yaw direction into consideration?
		double aileronGained = limit(0.05 * alphaFiltered, 0.0, 1.5) * flightSurface->roll_Command;

		//double ay = bodyState->getAccYPerG();
		// TODO: side acceleration (+ (ay * 19.3)) ?
		flightSurface->yaw_Command = aileronGained + yawRateWithRoll + rudderCommandFilteredWTrim;

		// without blending, rudder command == yaw command
		// -> change in mixer (after this call) if/when necessary
		flightSurface->rudder_Command = flightSurface->yaw_Command;
	}
};

#endif // ifndef _F16FCSYAWCONTROLLER_H_
