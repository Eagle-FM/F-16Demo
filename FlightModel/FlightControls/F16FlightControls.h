#ifndef _F16FLIGHTCONTROLS_H_
#define _F16FLIGHTCONTROLS_H_

#include "../stdafx.h"
#include <memory.h>
#include "../UtilityFunctions.h"

//#include "../include/general_filter.h"
#include "DummyFilter.h"

#include "Inputs/F16AnalogInput.h"


namespace F16
{
	// TODO! real actuator support

	class F16FcsController
	{
	public:
		F16FcsController() {}
		~F16FcsController() {}
	};

	// simple actuator,
	// replace with another later when ready..
	class F16Actuator
	{
	public:
		// This is "idealized" mathematical model more than actual
		// representation. Hence things like movement geometry is missing.
		// No calculation for force required either.

		double m_moveRate;
		double m_commanded;
		double m_current;

		double m_minLimit;
		double m_maxLimit;
		bool m_haveLimits;

	public:
		F16Actuator() 
			: m_moveRate(1.0), m_commanded(0), m_current(0),
			m_minLimit(0), m_maxLimit(0), m_haveLimits(false)
		{}
		F16Actuator(const double moverate)
			: m_moveRate(moverate), m_commanded(0), m_current(0),
			m_minLimit(0), m_maxLimit(0), m_haveLimits(false)
		{}
		F16Actuator(const double moverate, const double minLimit, const double maxLimit)
			: m_moveRate(moverate), m_commanded(0), m_current(0),
			m_minLimit(minLimit), m_maxLimit(maxLimit), m_haveLimits(true)
		{}
		~F16Actuator() {}

		void updateFrame(const double frameTime)
		{
			double movementPerFrame = m_moveRate*frameTime;
			double diff = m_commanded - m_current;
			if (movementPerFrame < diff)
			{
				// moving direction from whatever position we are in
				if (diff > 0)
				{
					m_current += movementPerFrame;
				}
				else if (diff < 0)
				{
					m_current -= movementPerFrame;
				}
			}
			else
			{
				// movement step per frame smaller than what max per frame is
				// -> set to target
				m_current = m_commanded;
			}

			// check that boundaries are not exceeded
			if (m_haveLimits == true && m_minLimit != m_maxLimit)
			{
				m_current = limit(m_current, m_minLimit, m_maxLimit);
			}
		}
	};

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
			return accz / 9.81;
		}
		double getAccYPerG() const
		{
			return accy / 9.81;
		}
	};

	// keep flight surface positions in one place
	class F16FlightSurface
	{
	public:
		double		leadingEdgeFlap_DEG = 0.0;			// Leading edge flap deflection (deg)
		double		leadingEdgeFlap_PCT = 0.0;			// Leading edge flap as a percent of maximum (0 to 1)

		double		flap_DEG = 0.0;			// Trailing edge flap deflection (deg)
		double		flap_PCT = 0.0;			// Trailing edge flap deflection (0 to 1)

		double		elevator_DEG = 0.0;			// Elevator deflection (deg)
		double		elevator_PCT = 0.0;			// Elevator deflection as a percent of maximum (-1 to 1)

		double		aileron_DEG = 0.0;			// Aileron deflection (deg)
		double		aileron_PCT = 0.0;			// Aileron deflection as a percent of maximum (-1 to 1)

		double		rudder_DEG = 0.0;			// Rudder  deflection (deg)
		double		rudder_PCT = 0.0;			// Rudder deflection as a percent of maximum (-1 to 1)

	public:
		F16FlightSurface() 
			: leadingEdgeFlap_DEG(0)
			, leadingEdgeFlap_PCT(0)
			, flap_DEG(0)
			, flap_PCT(0)
			, elevator_DEG(0)
			, elevator_PCT(0)
			, aileron_DEG(0)
			, aileron_PCT(0)
			, rudder_DEG(0)
			, rudder_PCT(0)
		{}
		~F16FlightSurface() {}
	};


	class F16FlightControls
	{
	public:
		bool		simInitialized;

	//protected:
		double		leading_edge_flap_integral;
		double		leading_edge_flap_integrated;
		double		leading_edge_flap_rate;
		double		leading_edge_flap_integrated_gained;
		double		leading_edge_flap_integrated_gained_biased;

		// note: airbrake limit different when landing gear down (prevent strike to runway)
		// cx_brk = 0.08, --coefficient, drag, breaks <- for airbrake?
		double airbrakeAngle; // 0 = off (in percentage)
		double airbrakeRate; // movement rate
		bool airbrakeSwitch; // switch status

		F16Actuator airbrakeActuator;

		bool isGearDown; // is landing gear down

		F16TrimState trimState;

		// Pitch controller variables
		AnalogInput		longStickInput; // pitch normalized
		AnalogInput		latStickInput; // bank normalized
		AnalogInput		pedInput;		// Pedal input command normalized (-1 to 1)

		// when MPO pressed down, override AOA/G-limiter and direct control of horiz. tail
		bool manualPitchOverride;

		// flap position logic according to when landing gears are down:
		// flaps are controlled with landing gear lever as well,
		// gears go down -> trailing edge flaps go down
		// gear go up -> trailing edge flaps go up
		bool gearRelatedFlaps;

		double		m_stickCommandPosFiltered;
		double		m_alphaFiltered;
		double		m_longStickForce;

		// Control filters (general filters to easily code up when compared to report block diagrams)
		DummyFilter	pitchRateWashout;
		DummyFilter	pitchIntegrator;
		DummyFilter	pitchPreActuatorFilter;
		DummyFilter	pitchActuatorDynamicsFilter;
		DummyFilter	accelFilter;
		DummyFilter	latStickForceFilter;
		DummyFilter	rollCommandFilter;
		DummyFilter	rollActuatorDynamicsFilter;
		DummyFilter	rollRateFilter1;
		DummyFilter	rollRateFilter2;
		DummyFilter	rudderCommandFilter;
		DummyFilter	yawRateWashout;
		DummyFilter	yawRateFilter;
		DummyFilter	yawServoFilter;

		F16BodyState bodyState;
		F16FlightSurface flightSurface;

	public:
		F16FlightControls() 
			: simInitialized(false)
			, leading_edge_flap_integral(0)
			, leading_edge_flap_integrated(0)
			, leading_edge_flap_rate(0)
			, leading_edge_flap_integrated_gained(0)
			, leading_edge_flap_integrated_gained_biased(0)
			, airbrakeAngle(0)
			, airbrakeRate(1)
			, airbrakeSwitch(false)
			, airbrakeActuator(1.0) // <- for now, set 1 (1*frametime)
			, isGearDown(true)
			, trimState(-0.3, 0, 0) // <- why -0.3 for pitch? edit: apparently to counteract something else?
			, longStickInput(-1.0, 1.0)
			, latStickInput(-1.0, 1.0)
			, pedInput(-1.0, 1.0)
			, manualPitchOverride(false)
			, gearRelatedFlaps(false)
			, m_stickCommandPosFiltered(0)
			, m_alphaFiltered(0)
			, m_longStickForce(0)
			, pitchRateWashout()
			, pitchIntegrator()
			, pitchPreActuatorFilter()
			, pitchActuatorDynamicsFilter()
			, accelFilter()
			, latStickForceFilter()
			, rollCommandFilter()
			, rollActuatorDynamicsFilter()
			, rollRateFilter1()
			, rollRateFilter2()
			, rudderCommandFilter()
			, yawRateWashout()
			, yawRateFilter()
			, yawServoFilter()
			, bodyState()
			, flightSurface()
		{
			// just do this once when constructing
			initialize(0);
		}
		~F16FlightControls() {}

		void setLatStickInput(double value) 
		{
			latStickInput = value;
		}
		void setLongStickInput(double value) 
		{
			longStickInput = -value;
		}
		void setPedInput(double value)
		{
			pedInput = -value;
		}

		void setManualPitchOverride(bool aoa_override)
		{
			manualPitchOverride = aoa_override;
		}

		// landing gear related flaps:
		// flaps are controlled with landing gear lever as well,
		// gears go down -> trailing edge flaps go down
		// gear go up -> trailing edge flaps go up
		void setGearRelatedFlaps(bool up)
		{
			gearRelatedFlaps = up;
		}

		void initAirBrakeOff()
		{
			airbrakeSwitch = false;
			airbrakeAngle = 0.0;
		}
		void setAirbrakeON()
		{
			airbrakeSwitch = true;
		}
		void setAirbrakeOFF()
		{
			airbrakeSwitch = false;
		}
		void switchAirbrake()
		{
			airbrakeSwitch = !airbrakeSwitch;
		}
		void setIsGearDown(bool gearDown)
		{
			isGearDown = gearDown;
		}

		// right-side
		float getAirbrakeRSAngle() const
		{
			return (float)airbrakeAngle;
		}
		// left-side
		float getAirbrakeLSAngle() const
		{
			return (float)airbrakeAngle;
		}

		void updateAirBrake(const double frameTime)
		{
			// for now, just use frametime for rate of movement
			// and add actuators for it later
			double movementRate = airbrakeRate*frameTime;

			// note: airbrake limit 60 degrees normally, 
			// 43 deg when landing gear down (prevent strike to runway)
			double maxAnglePCT = 1.0; // 60 deg
			if (isGearDown == true)
			{
				maxAnglePCT = 0.71; // ~43 deg
			}
			// TODO: if weight on wheel -> max opening
			// if gear down but no weight on wheel -> restricted
			// controlled by additional switch in cockpit?

			// this uses just percentage for now
			if (airbrakeSwitch == true && airbrakeAngle < maxAnglePCT)
			{
				airbrakeAngle += movementRate;
			}
			else if (airbrakeSwitch == false && airbrakeAngle > 0)
			{
				airbrakeAngle -= movementRate;
			}
			airbrakeAngle = limit(airbrakeAngle, 0, maxAnglePCT);
		}

		/*
		bool initializeLeadingEdgeFlapController()
		{
		}
		*/

		// Controller for the leading edge flaps
		double leading_edge_flap_controller(double dynamicPressure_FTLB, double staticPressure_FTLB, double frameTime)
		{
			double qbarOverPs = dynamicPressure_FTLB/staticPressure_FTLB;

			if(!(simInitialized))
			{
				leading_edge_flap_integral = -bodyState.alpha_DEG;
				leading_edge_flap_integrated = leading_edge_flap_integral + 2 * bodyState.alpha_DEG;
				return leading_edge_flap_integral;
			}

			leading_edge_flap_rate = (bodyState.alpha_DEG - leading_edge_flap_integrated) * 7.25;
			leading_edge_flap_integral += (leading_edge_flap_rate * frameTime);

			leading_edge_flap_integrated = leading_edge_flap_integral + bodyState.alpha_DEG * 2.0;
			leading_edge_flap_integrated_gained = leading_edge_flap_integrated * 1.38;
			leading_edge_flap_integrated_gained_biased = leading_edge_flap_integrated_gained + 1.45 - (9.05 * qbarOverPs);	

			return leading_edge_flap_integrated_gained_biased; 
		}

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

			return limit(rudderCommand, -30.0, 30.0);
		}

		// Controller for yaw
		double fcs_yaw_controller(double pedInput, double aileron_commanded, double dt)
		{
			const double roll_rate = bodyState.getRollRateDegs();
			const double yaw_rate = bodyState.getYawRateDegs();
			double ay = bodyState.getAccYPerG();

			double rudderCommand = getRudderCommand(pedInput);
			double rudderCommandFiltered = rudderCommandFilter.Filter(dt,rudderCommand);
			double rudderCommandFilteredWTrim = trimState.trimYaw - rudderCommandFiltered;

			double alphaGained = m_alphaFiltered * (1.0 / 57.3);
			double rollRateWithAlpha = roll_rate * alphaGained;
			double yawRateWithRoll = yaw_rate - rollRateWithAlpha;

			double yawRateWithRollWashedOut = yawRateWashout.Filter(dt,yawRateWithRoll);
			double yawRateWithRollFiltered = yawRateFilter.Filter(dt,yawRateWithRollWashedOut);

			double yawRateFilteredWithSideAccel = yawRateWithRollFiltered;// + (ay * 19.3);

			double aileronGained = limit(0.05 * m_alphaFiltered, 0.0, 1.5) * aileron_commanded;

			double finalRudderCommand = aileronGained + yawRateFilteredWithSideAccel + rudderCommandFilteredWTrim;

			return finalRudderCommand;

			//TODO: Figure out why there is a ton of flutter at high speed due to these servo dynamics
			//double yawServoCommand = yawServoFilter.Filter(!(simInitialized),dt,finalRudderCommand);
			//return yawServoCommand;
		}

		double getPitchRateCommand(const double longStickInputForce) const
		{
			double longStickCommand_G = 0.0;
			if (abs(longStickInputForce) <= 8.0)
			{
				longStickCommand_G = 0.0;
			}
			else if ((longStickInputForce < -8) && (longStickInputForce > -33.0))
			{
				longStickCommand_G = (0.016 * longStickInputForce) + 0.128;
			}
			else if (longStickInputForce <= -33.0)
			{
				longStickCommand_G = (0.067 * longStickInputForce) + 1.8112;
			}
			else if ((longStickInputForce > 8.0) && (longStickInputForce < 33.0))
			{
				longStickCommand_G = (0.032 * longStickInputForce) - 0.256;
			}
			else if (longStickInputForce >= 33.0)
			{
				longStickCommand_G = 0.0681*longStickInputForce - 1.4468;
			}
			return longStickCommand_G;
		}

		// Stick force schedule for pitch control
		double fcs_pitch_controller_force_command(double longStickInput, double dt)
		{
			double longStickInputForce = 0.0;
			if(longStickInput > 0.0)
			{
				longStickInputForce = longStickInput * 80.0;
			}
			else
			{
				longStickInputForce = longStickInput * 180.0;
			}
			longStickInputForce = limit(longStickInputForce,-180.0,80.0);
			m_longStickForce = longStickInputForce;

			double longStickCommand_G = getPitchRateCommand(longStickInputForce);

			double longStickCommandWithTrim_G = trimState.trimPitch - longStickCommand_G;

			double longStickCommandWithTrimLimited_G = limit(longStickCommandWithTrim_G, -4.0, 8.0);

			double longStickCommandWithTrimLimited_G_Rate = 4.0 * (longStickCommandWithTrimLimited_G - m_stickCommandPosFiltered);
			m_stickCommandPosFiltered += (longStickCommandWithTrimLimited_G_Rate * dt);

			return m_stickCommandPosFiltered;
		}

		// Schedule gain component due to dynamic pressure
		double dynamic_pressure_schedule(const double dynPressure_LBFT2) const
		{
			double dynamicPressure_kNM2 = dynPressure_LBFT2 * 1.4881639/1000.0; //for kN/m^2
			double scheduleOutput = 0.0;
			if(dynamicPressure_kNM2 < 9.576)
			{
				scheduleOutput = 1.0;
			}
			else if((dynamicPressure_kNM2 >= 9.576) && (dynamicPressure_kNM2 <= 43.0))
			{
				scheduleOutput =  (-0.018 * dynamicPressure_kNM2) + 1.1719;
				//scheduleOutput =  (-0.0239 * dynamicPressure_kNM2) + 1.2292;
			}
			else if(dynamicPressure_kNM2 > 43.0)
			{
				scheduleOutput = -0.003 * dynamicPressure_kNM2 + 0.5277;
				//scheduleOutput = -0.001 * dynamicPressure_kNM2 + 0.2422;
			}
			return limit(scheduleOutput,0.05,1.0);
		}

		// Angle of attack limiter logic
		double angle_of_attack_limiter(const double alphaFiltered, const double pitchRateCommand) const
		{
			double topLimit = limit((alphaFiltered - 22.5) * 0.69, 0.0, 99999.0);
			double bottomLimit = limit((alphaFiltered - 15.0 + pitchRateCommand) * 0.322, 0.0, 99999.0);

			return (topLimit + bottomLimit);
		}


		// Controller for pitch
		// (differentialCommand is hard-coded to 0 in caller)
		double fcs_pitch_controller(double longStickInput, double differentialCommand, double dynPressure_LBFT2, double dt)
		{
			const double pitch_rate = bodyState.getPitchRateDegs();
			const double az = bodyState.getAccZPerG();

			double stickCommandPos = fcs_pitch_controller_force_command(longStickInput, dt);

			double dynamicPressureScheduled = dynamic_pressure_schedule(dynPressure_LBFT2);	

			double azFiltered = accelFilter.Filter(dt, az-1.0);

			double alphaLimited = limit(bodyState.alpha_DEG, -5.0, 30.0);
			double alphaLimitedRate = 10.0 * (alphaLimited - m_alphaFiltered);
			m_alphaFiltered += (alphaLimitedRate * dt);

			double pitchRateWashedOut = pitchRateWashout.Filter(dt,pitch_rate);
			double pitchRateCommand = pitchRateWashedOut * 0.7 * dynamicPressureScheduled;		

			double limiterCommand = angle_of_attack_limiter(-m_alphaFiltered, pitchRateCommand);

			double gLimiterCommand = -(azFiltered +  (pitchRateWashedOut * 0.2));	

			double finalCombinedCommand = dynamicPressureScheduled * (2.5 * (stickCommandPos + limiterCommand + gLimiterCommand));

			double finalCombinedCommandFilteredLimited = limit(pitchIntegrator.Filter(dt,finalCombinedCommand),-25.0,25.0);
			finalCombinedCommandFilteredLimited = finalCombinedCommandFilteredLimited + finalCombinedCommand;

			double finalPitchCommandTotal = pitchPreActuatorFilter.Filter(dt,finalCombinedCommandFilteredLimited);
			finalPitchCommandTotal += (0.5 * m_alphaFiltered);

			return finalPitchCommandTotal;

			// TODO: There are problems with flutter with the servo dynamics...needs to be nailed down!
			//double actuatorDynamicsResult = pitchActuatorDynamicsFilter.Filter(!(simInitialized),dt,finalPitchCommandTotal);
			//return actuatorDynamicsResult;	
		}

		double getRollRateCommand(const double latStickForceFinal) const
		{
			double rollRateCommand = 0.0;
			if (abs(latStickForceFinal) < 3.0)
			{
				rollRateCommand = 0.0;
			}
			else if ((latStickForceFinal >= 3.0) && (latStickForceFinal <= 25.0))
			{
				rollRateCommand = 0.9091 * latStickForceFinal - 2.7273;
			}
			else if ((latStickForceFinal > 25.0) && (latStickForceFinal <= 46.0))
			{
				rollRateCommand = 2.8571 * latStickForceFinal - 51.429;
			}
			else if ((latStickForceFinal > 46.0))
			{
				rollRateCommand = 7.5862 * latStickForceFinal - 268.97;
			}
			else if ((latStickForceFinal <= -3.0) && (latStickForceFinal >= -25.0))
			{
				rollRateCommand = 0.9091 * latStickForceFinal + 2.7273;
			}
			else if ((latStickForceFinal < -25.0) && (latStickForceFinal >= -46.0))
			{
				rollRateCommand = 2.8571 * latStickForceFinal + 51.429;
			}
			else if ((latStickForceFinal < -46.0))
			{
				rollRateCommand = 7.5862 * latStickForceFinal + 268.97;
			}
			return rollRateCommand;
		}

		double getRollFeelGain(const double longStickForce) const
		{
			double longStickForceGained = longStickForce * 0.0667;
			double rollFeelGain = 0.0;
			if (abs(longStickForce) > 25.0)
			{
				rollFeelGain = 0.7;
			}
			else if (longStickForce >= 0.0)
			{
				rollFeelGain = -0.012 * longStickForceGained + 1.0;
			}
			else if (longStickForce < 0.0)
			{
				rollFeelGain = 0.012 * longStickForceGained + 1.0;
			}
			return rollFeelGain;
		}

		// Controller for roll
		double fcs_roll_controller(double latStickInput, double dynPressure_LBFT2, double dt)
		{
			const double roll_rate = bodyState.getRollRateDegs();
			double ay = bodyState.getAccYPerG();


			double latStickForceCmd = latStickInput * 75.0;
			double latStickForce = latStickForceFilter.Filter(dt,latStickForceCmd);

			double latStickForceBiased = latStickForce - (ay * 8.9);  // CJS: remove side acceleration bias?

			double rollFeelGain = getRollFeelGain(m_longStickForce);

			double rollRateCommand = getRollRateCommand(latStickForceBiased * rollFeelGain);

			double rollRateCommandFilterd = rollCommandFilter.Filter(dt,rollRateCommand);

			double rollRateFiltered1 = rollRateFilter1.Filter(dt,roll_rate);

			double rollRateFiltered2 = (rollRateFilter2.Filter(dt,rollRateFiltered1));

			double rollRateCommandCombined = rollRateFiltered2 - rollRateCommandFilterd - trimState.trimRoll;

			double dynamicPressure_NM2 = dynPressure_LBFT2 * 47.880258889;

			double pressureGain = 0.0;
			if(dynamicPressure_NM2 < 19153.0)
			{
				pressureGain = 0.2;
			}
			else if((dynamicPressure_NM2 >= 19153.0) && (dynamicPressure_NM2 <= 23941.0))
			{
				pressureGain = -0.00002089 * dynamicPressure_NM2 + 0.6;
			}
			else
			{
				pressureGain = 0.1;
			}

			double rollCommandGained = limit(rollRateCommandCombined * pressureGain, -21.5, 21.5);

			// Mechanical servo dynamics
			double rollActuatorCommand = rollActuatorDynamicsFilter.Filter(dt,rollCommandGained);	
			return rollActuatorCommand;
		}		

		// Passive flap schedule for the F-16...nominal for now from flight manual comments
		double fcs_flap_controller(double airspeed_FPS)
		{
			double airspeed_KTS = 0.5924838012958964 * airspeed_FPS;
			double trailing_edge_flap_deflection = 0.0;

			if(airspeed_KTS < 240.0)
			{
				trailing_edge_flap_deflection = 20.0;
			}
			else if((airspeed_KTS >= 240.0) && (airspeed_KTS <= 370.0))
			{
				trailing_edge_flap_deflection = (1.0 - ((airspeed_KTS - 240.0)/(370.0-240.0))) * 20.0;
			}
			else
			{
				trailing_edge_flap_deflection = (1.0 - ((airspeed_KTS - 240.0)/(370.0-240.0))) * 20.0;
			}

			trailing_edge_flap_deflection = limit(trailing_edge_flap_deflection,0.0,20.0);

			return trailing_edge_flap_deflection;
		}

		bool initializeYawController()
		{
			if (simInitialized == true)
			{
				return true;
			}

			double numerators[2] = { 0.0, 4.0 };
			double denominators[2] = { 1.0, 4.0 };
			rudderCommandFilter.InitFilter(numerators, denominators, 1);

			double numerators1[2] = { 1.0, 0.0 };
			double denominators1[2] = { 1.0, 1.0 };
			yawRateWashout.InitFilter(numerators1, denominators1, 1);

			double numerators2[2] = { 3.0, 15.0 };
			double denominators2[2] = { 1.0, 15.0 };
			yawRateFilter.InitFilter(numerators2, denominators2, 1);

			double numerators3[3] = { 0.0, 0.0, pow(52.0, 2.0) };
			double denomiantors3[3] = { 1.0, 2.0*0.7*52.0, pow(52.0, 2.0) };
			yawServoFilter.InitFilter(numerators3, denomiantors3, 2);
			return true;
		}

		bool initializePitchController()
		{
			if (simInitialized == true)
			{
				return true;
			}

			double numerators[2] = { 1.0, 0.0 };
			double denominators[2] = { 1.0, 1.0 };
			pitchRateWashout.InitFilter(numerators, denominators, 1);

			numerators[0] = 0.0; numerators[1] = 2.5;
			denominators[0] = 1.0; denominators[1] = 0.0;
			pitchIntegrator.InitFilter(numerators, denominators, 1);

			numerators[0] = 3.0; numerators[1] = 15;
			denominators[0] = 1.0; denominators[1] = 15.0;
			pitchPreActuatorFilter.InitFilter(numerators, denominators, 1);

			double numerators2[3] = { 0.0, 0.0, pow(52.0, 2.0) };
			double denomiantors2[3] = { 1.0, 2.0*0.7*52.0, pow(52.0, 2.0) };
			pitchActuatorDynamicsFilter.InitFilter(numerators2, denomiantors2, 2);

			numerators[0] = 0.0; numerators[1] = 15.0;
			denominators[0] = 1.0; denominators[1] = 15.0;
			accelFilter.InitFilter(numerators, denominators, 1);
			return true;
		}

		bool initializeRollController()
		{
			if (simInitialized == true)
			{
				return true;
			}

			double numerators[2] = { 0.0, 60.0 };
			double denominators[2] = { 1.0, 60.0 };
			latStickForceFilter.InitFilter(numerators, denominators, 1);

			double numerators1[2] = { 0.0, 10.0 };
			double denominators1[2] = { 1.0, 10.0 };
			rollCommandFilter.InitFilter(numerators1, denominators1, 1);

			double numerators2[3] = { 0.0, 0.0, pow(52.0, 2.0) };
			double denomiantors2[3] = { 1.0, 2.0*0.7*52.0, pow(52.0, 2.0) };
			rollActuatorDynamicsFilter.InitFilter(numerators2, denomiantors2, 2);

			double numerators3[2] = { 0.0, 50.0 };
			double denominators3[2] = { 1.0, 50.0 };
			rollRateFilter1.InitFilter(numerators3, denominators3, 1);

			double numerators4[3] = { 4.0, 64.0, 6400.0 };
			double denomiantors4[3] = { 1.0, 80.0, 6400.0 };
			rollRateFilter2.InitFilter(numerators4, denomiantors4, 2);
			return true;
		}

	public:

		bool initialize(double dt)
		{
			if (simInitialized == true)
			{
				return true;
			}

			bool res = true;

			res &= initializePitchController();
			res &= initializeRollController();
			res &= initializeYawController();

			resetFilters(dt);
			return res;
		}
		void resetFilters(double dt)
		{
			// this kind of stuff is only needed on (re-)initialize
			// -> remove one pointless flag parameter from each Filter() call
			pitchRateWashout.ResetFilter(dt);
			pitchIntegrator.ResetFilter(dt);
			pitchPreActuatorFilter.ResetFilter(dt);
			pitchActuatorDynamicsFilter.ResetFilter(dt);
			accelFilter.ResetFilter(dt);
			latStickForceFilter.ResetFilter(dt);
			rollCommandFilter.ResetFilter(dt);
			rollActuatorDynamicsFilter.ResetFilter(dt);
			rollRateFilter1.ResetFilter(dt);
			rollRateFilter2.ResetFilter(dt);
			rudderCommandFilter.ResetFilter(dt);
			yawRateWashout.ResetFilter(dt);
			yawRateFilter.ResetFilter(dt);
			yawServoFilter.ResetFilter(dt);
		}

		// before simulation starts
		void setCurrentState(double ax, double ay, double az)
		{
			bodyState.ay_world = ay;
		}


		// gather some values for use later
		void setBodyAxisState(double common_angle_of_attack, double common_angle_of_slide, double omegax, double omegay, double omegaz, double ax, double ay, double az)
		{
			//-------------------------------
			// Start of setting F-16 states
			//-------------------------------
			bodyState.alpha_DEG = common_angle_of_attack * F16::radiansToDegrees;
			bodyState.beta_DEG = common_angle_of_slide * F16::radiansToDegrees;
			bodyState.rollRate_RPS = omegax;
			bodyState.pitchRate_RPS = omegaz;
			bodyState.yawRate_RPS = -omegay;

			// note the change in coordinate system here..
			bodyState.accz = ay;
			bodyState.accy = az;
		}

		// after first frame is done
		void setInitialized()
		{
			simInitialized = true;
		}

		//---------------------------------------------
		//-----CONTROL DYNAMICS------------------------
		//---------------------------------------------
		void updateFrame(double totalVelocity_FPS, double dynamicPressure_LBFT2, double ps_LBFT2, double frametime)
		{
			//if (airbrakeExtended != airbrakeSwitch)
			// -> actuator movement by frame step
			updateAirBrake(frametime);

			// Call the leading edge flap dynamics controller, this controller is based on dynamic pressure and angle of attack
			// and is completely automatic
			// Leading edge flap deflection (deg)
			flightSurface.leadingEdgeFlap_DEG = leading_edge_flap_controller(dynamicPressure_LBFT2, ps_LBFT2, frametime);
			flightSurface.leadingEdgeFlap_PCT = limit(flightSurface.leadingEdgeFlap_DEG / 25.0, 0.0, 1.0);

			// Call the longitudinal (pitch) controller.  Takes the following inputs:
			// -Normalize long stick input
			// -Trimmed G offset
			// -Angle of attack (deg)
			// -Pitch rate (rad/sec)
			// -Differential command (from roll controller, not quite implemented yet)

			double elevator_DEG_commanded = -(fcs_pitch_controller(longStickInput.getValue(), 0.0, dynamicPressure_LBFT2, frametime));
			// Call the servo dynamics model (not used as it causes high flutter in high speed situations, related to filtering and dt rate)
			flightSurface.elevator_DEG = elevator_DEG_commanded; //F16::ACTUATORS::elevator_actuator(F16::elevator_DEG_commanded,dt);
			flightSurface.elevator_DEG = limit(flightSurface.elevator_DEG, -25.0, 25.0);

			double aileron_DEG_commanded = (fcs_roll_controller(latStickInput.getValue(), dynamicPressure_LBFT2, frametime));
			flightSurface.aileron_DEG = aileron_DEG_commanded; //F16::ACTUATORS::aileron_actuator(F16::aileron_DEG_commanded,dt);
			flightSurface.aileron_DEG = limit(flightSurface.aileron_DEG, -21.5, 21.5);

			double rudder_DEG_commanded = fcs_yaw_controller(pedInput.getValue(), aileron_DEG_commanded, frametime);
			flightSurface.rudder_DEG = rudder_DEG_commanded; //F16::ACTUATORS::rudder_actuator(F16::rudder_DEG_commanded,dt);
			flightSurface.rudder_DEG = limit(flightSurface.rudder_DEG, -30.0, 30.0);

			// reuse in drawargs
			//flightSurface.leadingEdgeFlap_PCT = 
			flightSurface.aileron_PCT = flightSurface.aileron_DEG / 21.5;
			flightSurface.elevator_PCT = flightSurface.elevator_DEG / 25.0;
			flightSurface.rudder_PCT = flightSurface.rudder_DEG / 30.0;

			// Trailing edge flap deflection (deg)
			flightSurface.flap_DEG = fcs_flap_controller(totalVelocity_FPS);
			flightSurface.flap_PCT = flightSurface.flap_DEG / 20.0;
		}

	};
}

#endif // ifndef _F16FLIGHTCONTROLS_H_
