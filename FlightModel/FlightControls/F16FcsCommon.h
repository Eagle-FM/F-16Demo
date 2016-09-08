#ifndef _F16FCSCOMMON_H_
#define _F16FCSCOMMON_H_

#include <cmath>

#include "../UtilityFunctions.h"

// if/when trimming support is needed,
// keep settings in one place.
class F16TrimState
{
public:
	// TODO: limits of trimmer for each axis
	//const double pitchLimit;
	//const double rollLimit;
	//const double yawLimit;

	// increments per "notch" for each axis
	const double pitchIncrement;
	const double rollIncrement;
	const double yawIncrement;

	double trimPitch = 0.0;
	double trimRoll = 0.0;
	double trimYaw = 0.0;

public:
	F16TrimState(const double pitch, const double roll, const double yaw)
		//: pitchLimit(5.0), rollLimit(5.0), yawLimit(5.0)
		: pitchIncrement(0.1), rollIncrement(0.1), yawIncrement(0.1)
		, trimPitch(pitch), trimRoll(roll), trimYaw(yaw)
	{}
	~F16TrimState() {}

	void pitchUp() 		{ trimPitch += pitchIncrement; }
	void pitchDown()	{ trimPitch -= pitchIncrement; }

	void rollCCW()		{ trimRoll -= rollIncrement; }
	void rollCW()		{ trimRoll += rollIncrement; }

	void yawLeft()		{ trimYaw += yawIncrement; }
	void yawRight()		{ trimYaw -= yawIncrement; }
};

// just keep some things together in easily accessible way
class F16BodyState
{
public:
	double		alpha_DEG = 0.0;			// Angle of attack (deg)
	double		beta_DEG = 0.0;			// Slideslip angle (deg)

	double		rollRate_RPS = 0.0;			// Body roll rate (rad/sec)
	double		pitchRate_RPS = 0.0;			// Body pitch rate (rad/sec)
	double		yawRate_RPS = 0.0;			// Body yaw rate (rad/sec)

	double		ay_world = 0.0;			// World referenced up/down acceleration (m/s^2)
	double		accz = 0.0;			// Az (per normal direction convention) out the bottom of the a/c (m/s^2)
	double		accy = 0.0;			// Ay (per normal direction convention) out the right wing (m/s^2)

public:
	F16BodyState() {}
	~F16BodyState() {}

	double getRollRateDegs() const
	{
		return rollRate_RPS * F16::radiansToDegrees;
	}
	double getPitchRateDegs() const
	{
		return pitchRate_RPS * F16::radiansToDegrees;
	}
	double getYawRateDegs() const
	{
		return yawRate_RPS * F16::radiansToDegrees;
	}

	double getAccZPerG() const
	{
		return accz / F16::standard_gravity;
	}
	double getAccYPerG() const
	{
		return accy / F16::standard_gravity;
	}
};

// keep flight surface positions in one place
class F16FlightSurface
{
public:
	double		leadingEdgeFlap_DEG;			// Leading edge flap deflection (deg)
	double		leadingEdgeFlap_Right_PCT;			// Leading edge flap as a percent of maximum (0 to 1)
	double		leadingEdgeFlap_Left_PCT;			// Leading edge flap as a percent of maximum (0 to 1)

	double		flap_Command;		// command from tef controller (position), can be adjusted by roll controller?
	double		flap_DEG;			// Trailing edge flap deflection (deg)
	double		flap_PCT;			// Trailing edge flap deflection (0 to 1)

	double		elevator_DEG;			// Elevator deflection (deg)
	double		elevator_Right_PCT;			// Elevator deflection as a percent of maximum (-1 to 1)
	double		elevator_Left_PCT;			// Elevator deflection as a percent of maximum (-1 to 1)

	double		aileron_DEG;			// Aileron deflection (deg)
	double		aileron_Right_PCT;			// Aileron deflection as a percent of maximum (-1 to 1)
	double		aileron_Left_PCT;			// Aileron deflection as a percent of maximum (-1 to 1)

	double		rudder_DEG;			// Rudder  deflection (deg)
	double		rudder_PCT;			// Rudder deflection as a percent of maximum (-1 to 1)

public:
	F16FlightSurface()
		: leadingEdgeFlap_DEG(0)
		, leadingEdgeFlap_Right_PCT(0)
		, leadingEdgeFlap_Left_PCT(0)
		, flap_Command(0)
		, flap_DEG(0)
		, flap_PCT(0)
		, elevator_DEG(0)
		, elevator_Right_PCT(0)
		, elevator_Left_PCT(0)
		, aileron_DEG(0)
		, aileron_Right_PCT(0)
		, aileron_Left_PCT(0)
		, rudder_DEG(0)
		, rudder_PCT(0)
	{}
	~F16FlightSurface() {}
};


#endif // ifndef _F16FCSCOMMON_H_