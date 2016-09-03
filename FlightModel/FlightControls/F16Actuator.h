#ifndef _F16ACTUATOR_H_
#define _F16ACTUATOR_H_

#include "../stdafx.h"
#include <memory.h>
#include "../UtilityFunctions.h"


namespace F16
{
	// simple actuator,
	// replace with another later when ready..
	class F16Actuator
	{
	public:
		// This is "idealized" mathematical model more than actual
		// representation. Hence things like movement geometry is missing.
		// No calculation for the required force, either.
		// -> add torque
		// -> add servo
		// -> add sensor and delay (lag)

		// "Command" is the position into which actuator should move.
		// "Current" is where the actuator is at the moment (consider: if move finished within time or not).
		// "Moverate" is simply "step per frame" the actuator can take.
		// Movement values can be angles, radians, inches, meters etc. 
		// Just as long as caller takes care that units match.
		//
		// Limits are there to check that calculations won't overrun.
		// 
		double m_moveRate;		// "slowness"
		double m_commanded;		// target movement
		double m_current;		// current position
		double m_minLimit;		// lower limit
		double m_maxLimit;		// upper limit
		bool m_haveLimits;		// if limits are defined

		// Force threshold needed to move either way:
		// provided force must exceed required force to be able to move in that direction.
		//
		// For example, if there is no more hydraulic power (engine out of fuel)
		// but reservoir still has enough to push landing gear down but not enough to pull back in.
		//
		// Actuator code works for any unit this moment: N, V etc.
		// Caller just has to make sure to provide values in same unit each time
		//
		double m_forceResistsInc;		// force threshold to move towards upper limit
		double m_forceResistsDec;		// force threshold to move towards lower limit
		double m_forceProvided;			// amount of force provided to actuator to operate

		// Amount of dampening actuator provides against external forces.
		// For example, airflow acting on control surface.
		double m_dampeningRate;

		// External force affecting actuator
		double m_externalForce;

		// is in working condition/damaged
		bool m_isWorking;

	public:
		/*
		F16Actuator() 
			: m_moveRate(1.0), m_commanded(0), m_current(0),
			m_minLimit(0), m_maxLimit(0), m_haveLimits(false), 
			m_isWorking(true)
		{}
		*/

		F16Actuator(const double moverate)
			: m_moveRate(moverate), m_commanded(0), m_current(0),
			m_minLimit(0), m_maxLimit(0), m_haveLimits(false), 
			m_forceResistsInc(0), m_forceResistsDec(0), m_forceProvided(0),
			m_dampeningRate(0), m_externalForce(0), m_isWorking(true)
		{}
		F16Actuator(const double moverate, const double minLimit, const double maxLimit)
			: m_moveRate(moverate), m_commanded(0), m_current(0),
			m_minLimit(minLimit), m_maxLimit(maxLimit), m_haveLimits(true), 
			m_forceResistsInc(0), m_forceResistsDec(0), m_forceProvided(0),
			m_dampeningRate(0), m_externalForce(0), m_isWorking(true)
		{}
		~F16Actuator() {}

		// TODO: improve upon this before using
		// this should be non-linear function to give actual movement when resisting force increases near limit?
		// or are all actuators expected work exactly to limit and then stop?
		bool forceThresholdMove(const double movementPerFrame, const double diff, const double frameTime)
		{
			/* this is still crude.. don't do this..

			// if force threshold defined
			if (diff > 0 && m_forceProvided < m_forceResistsInc && m_forceResistsInc > 0)
			{
				// movement towards upper limit:
				// not enough force -> can't continue
				return;
			}
			// if force threshold defined
			if (diff < 0 && m_forceProvided < m_forceResistsDec && m_forceResistsDec > 0)
			{
				// movement towards lower limit:
				// not enough force -> can't continue
				return;
			}

			TODO: something like this instead?
			if ((m_forceProvided * movementPerFrame) > m_forceResistsInc && diff > 0)
			{
				m_current += m_forceProvided * movementPerFrame;
			}
			if ((m_forceProvided * movementPerFrame) > m_forceResistsDec && diff < 0)
			{
				m_current += m_forceProvided * movementPerFrame;
			}
			*/

			return true;
		}

		void updateFrame(const double frameTime)
		{
			if (m_isWorking == false)
			{
				return;
			}

			double movementPerFrame = m_moveRate*frameTime;
			double diff = m_commanded - m_current;


			// not enough force provided to complete movement -> end
			if (forceThresholdMove(movementPerFrame, diff, frameTime) == false)
			{
				return;
			}


			// if movement per frame is small enough (either direction),
			// it can be done in one frame
			if (movementPerFrame < abs(diff))
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

}

#endif // ifndef _F16ACTUATOR_H_