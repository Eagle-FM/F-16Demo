//--------------------------------------------------------------------------
// F-16 Demo External Flight Model for DCS World
// 
// Author: CptSmiley (forums.eagle.ru username)
//
// Use Only for Non-Commercial Purposes
//
//--------------------------------------------------------------------------
// Source Data:
// 1) F-16 University of Minnesota Non-Linear Flight Model
//	  http://www.aem.umn.edu/people/faculty/balas/darpa_sec/SEC.Software.html
// 2) NASA TP 1538 Simulator Study of Stall/Post-Stall Characteristics of a 
//	  Fighter Airplane With Relaxed Longitudinal Static Stability
// 3) NASA TN D-8176 Simulator Study of the Effectiveness of an Automatic Control
//    System Designed to Improve the High Angle-of-Attack Characteristics of a
//    Fighter Airplane
// 4) AFIT/GE/ENG/88D-8 A Real-time Simulator for Man-In-The-Loop Testing of
//    Aircraft Control Systems
// 5) JBSim 1.0 F-16 Configuration 
//
//--------------------------------------------------------------------------
// F-16Demo.cpp : Defines the exported functions for the DLL application.
// Control the main portion of the discrete simulation event
//
// This project will compile a DLL.  This DLL needs to be compiled with the
// same machine type of your machine (x64 only).  This DLL then needs to
// be placed within the bin directory in your mod/aircraft/XXXairplane/ 
// directory within DCS World.  
//
// Note that 32-bit build is not supported any longer.
// It might compile by adding build configuration but might not work correctly.
//
// See associated entry.lua for how to tell the mod to use the DLL flight
// model
//--------------------------------------------------------------------------
// IMPORTANT!  COORDINATE CONVENTION:
//
// DCS WORLD Convention:
// Xbody: Out the front of the nose
// Ybody: Out the top of the aircraft
// Zbody: Out the right wing
//
// Normal Aerodynamics/Control Convention:
// Xbody: Out the front of the nose
// Ybody: Out the right wing
// Zbody: Out the bottom of the aircraft
//
// This means that if you are referincing from any aerodynamic, stabilty, or control document
// they are probably using the second set of directions.  Which means you always need to switch
// the Y and the Z and reverse the Y prior to output to DCS World
//---------------------------------------------------------------------------
// TODO List:
// -Make code more "object-oriented"...
// -Differential command into the pitch controller
// -Weight on wheels determination
// -Ground reaction modeling
// -Fix actuator dynamics
// -Improve look-up tables
// -Speed brake effects and control
//---------------------------------------------------------------------------
// KNOWN Issues:
// -On ground, the FCS controls flutter due to no filtering of alpha and Nz.
//  Need logic to determine when on ground (hackish right now) to zero those
//  signals out.
// -Aircraft naturally trims to 1.3g for some reason, need to apply -0.3 pitch
//  trim to get aircraft to trim at 1.0g for flight controller
// -> relaxed static stability (RSS) so as intended?
// -Actuators cause flutter at high speed due to filtering of sensor signals
//  Removed servo-dynamics until I can figure this out
// -> reduced unnecessary unit conversions and thus resulting rounding errors etc.
// -> fixed
// -Gear reaction happening but ground handling not modeled due to lack of available
//  API calls
// -> partially now
// -Gear automatically drops at 200ft to allow simple touch downs
//---------------------------------------------------------------------------
#include "stdafx.h"
#include "F_16Demo.h"

// for debug use
#include <wchar.h>
#include <cstdio>

#include "ED_FM_Utility.h"		// Provided utility functions that were in the initial EFM example
#include "F16Constants.h"		// Common constants used throughout this DLL

#include "Inputs/F16Inputs.h"			// just list of inputs: can get potentially long list

// Model headers
#include "Atmosphere/F16Atmosphere.h"			//Atmosphere model functions
#include "Atmosphere/F16GroundSurface.h"
#include "Aerodynamics/F16Aero.h"				//Aerodynamic model functions

#include "Hydraulics/F16HydraulicSystem.h"
#include "FlightControls/F16FlightControlSystem.h"	//Flight Controls model functions

#include "Engine/F16EngineManagementSystem.h"
#include "Engine/F16Engine.h"					//Engine model functions
#include "Engine/F16FuelSystem.h"				//Fuel usage and tank usage functions

#include "LandingGear/F16LandingGear.h"			//Landing gear actuators, aerodynamic drag, wheelbrake function
#include "Airframe/F16Airframe.h"				//Canopy, dragging chute, refuel slot, section damages..
#include "Electrics/F16ElectricSystem.h"		//Generators, battery etc.
#include "EnvironmentalSystem/F16EnvControlSystem.h"	//Oxygen system, heating/cooling, G-suit, canopy sealing..

// physics integration
#include "EquationsOfMotion/F16EquationsOfMotion.h"


wchar_t dbgmsg[255] = {0};
//dbgmsg[0] = 0;


//-------------------------------------------------------
// Start of F-16 Simulation Variables
// Probably doesn't need it's own namespace or anything
// I just quickly did this to organize my F-16 specific
// variables, needs to be done better eventually
//-------------------------------------------------------
namespace F16
{
	double		longStickInputRaw = 0; // pitch orig (just for cockpit anim)
	double		latStickInputRaw = 0; // bank orig (just for cockpit anim)
	double		pedInputRaw = 0;	// yaw orig (just for cockpit anim)

	F16Atmosphere Atmos;
	F16GroundSurface Ground(&Atmos);
	F16Aero Aero(&Atmos, &Ground);
	F16FuelSystem Fuel;
	F16Airframe Airframe;
	F16EngineManagementSystem EMS(&Atmos, &Fuel);
	F16LandingGear LandingGear(&Atmos, &Ground);
	F16FlightControls FlightControls(&Atmos, &LandingGear, &Airframe);
	F16Motion Motion(&Atmos);
	F16HydraulicSystem Hydraulics;
	F16ElectricSystem Electrics;
	F16EnvControlSystem EnvCS(&Atmos);
}

// This is where the simulation send the accumulated forces to the DCS Simulation
// after each run frame
void ed_fm_add_local_force(double &x,double &y,double &z,double &pos_x,double &pos_y,double &pos_z)
{
	F16::Motion.getLocalForce(x, y, z, pos_x, pos_y, pos_z);
}

// Not used
void ed_fm_add_global_force(double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{

}

/* same but in component form , return value bool : function will be called until return value is true
while (ed_fm_add_local_force_component(x,y,z,pos_xpos_y,pos_z))
{
	--collect 
}
*/
bool ed_fm_add_local_force_component (double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{
	return false;
}
bool ed_fm_add_global_force_component (double & x,double &y,double &z,double & pos_x,double & pos_y,double & pos_z)
{
	return false;
}

// This is where the simulation send the accumulated moments to the DCS Simulation
// after each run frame
void ed_fm_add_local_moment(double &x,double &y,double &z)
{
	F16::Motion.getLocalMoment(x, y, z);
}

// Not used
void ed_fm_add_global_moment(double & x,double &y,double &z)
{

}

/* same but in component form , return value bool : function will be called until return value is true
while (ed_fm_add_local_moment_component(x,y,z))
{
	--collect 
}
*/
bool ed_fm_add_local_moment_component (double & x,double &y,double &z)
{
	return false;
}
bool ed_fm_add_global_moment_component (double & x,double &y,double &z)
{
	return false;
}

//-----------------------------------------------------------------------
// The most important part of the entire EFM code.  This is where you code
// gets called for each run frame.  Each run frame last for a duration of
// "dt" (delta time).  This can be used to help time certain features such
// as filters and lags
//-----------------------------------------------------------------------
// NOTE! dt is actually slice-length DCS wants code to limit itself to,
// not actually time passed since last frame - it is constant 0.006000 seconds.
//
// Simulation step/slice/frame should take care of syncing over network (hopefully)
// and it should take care of pausing the simulation: we can't calculate those cases here
// and can only trust DCS to give suitable value.
//
void ed_fm_simulate(double dt)
{
	double frametime = dt; // initialize only

	// Very important! clear out the forces and moments before you start calculated
	// a new set for this run frame
	F16::Motion.clear();

	// Get the total absolute velocity acting on the aircraft with wind included
	// using english units so airspeed is in feet/second here
	F16::Atmos.updateFrame(frametime);
	F16::Ground.updateFrame(frametime);

	// update thrust, engine management
	F16::EMS.updateFrame(frametime);

	// update amount of fuel used and change in mass
	F16::Fuel.updateFrame(F16::EMS.getFuelPerFrame(), frametime);
	F16::Motion.updateFuelMass(F16::Fuel.getInternalFuel()); // <- update total fuel weight

	// update oxygen provided to pilot: tanks, bleed air from engine etc.
	F16::EnvCS.updateFrame(frametime);

	// TODO: engine power to hydraulics
	F16::Hydraulics.updateFrame(frametime);

	F16::Electrics.updateFrame(frametime);
	F16::Airframe.updateFrame(frametime);

	// TODO:! give ground speed to calculate wheel slip/grip!
	// we use total velocity for now..
	// use "dry" weight and internal fuel weight (TODO: take care of it in motion)
	F16::LandingGear.updateFrame(F16::Motion.getTotalWeightN(), frametime);

	//-----CONTROL DYNAMICS------------------------
	// landing gear "down&locked" affects some logic
	F16::FlightControls.updateFrame(frametime);

	F16::Aero.updateFrame(F16::FlightControls.bodyState, F16::FlightControls.flightSurface, frametime);
	F16::Aero.computeTotals(F16::FlightControls.flightSurface, F16::FlightControls.bodyState,
		F16::LandingGear.CxGearAero, 
		F16::LandingGear.CzGearAero);

	F16::Motion.updateAeroForces(F16::Aero.getCyTotal(), F16::Aero.getCxTotal(), F16::Aero.getCzTotal(), 
								F16::Aero.getClTotal(), F16::Aero.getCmTotal(), F16::Aero.getCnTotal());

	F16::Motion.updateEngineForces(F16::EMS.Engine.getThrustN(), F16::EMS.Engine.enginePosition, F16::EMS.getTurbineMomentum());

	// just internal fuel for now, payload and external tanks later
	F16::Motion.updateWetMassCg(F16::Fuel.FwdFus.fuel, F16::Fuel.FwdFus.position, F16::Fuel.FwdFus.size);
	F16::Motion.updateWetMassCg(F16::Fuel.AftFus.fuel, F16::Fuel.AftFus.position, F16::Fuel.AftFus.size);
	F16::Motion.updateWetMassCg(F16::Fuel.LeftWing.fuel, F16::Fuel.LeftWing.position, F16::Fuel.LeftWing.size);
	F16::Motion.updateWetMassCg(F16::Fuel.RightWing.fuel, F16::Fuel.RightWing.position, F16::Fuel.RightWing.size);
	F16::Motion.commitWetMassCg();

	F16::Motion.updateFuelMassDelta(F16::Fuel.getUsageSinceLastFrame());
	F16::Fuel.clearUsageSinceLastFrame();

	if (F16::LandingGear.isWoW() == true)
	{
		F16::Motion.updateWheelForces(F16::LandingGear.wheelLeft.CxWheelFriction,
									F16::LandingGear.wheelLeft.CzWheelFriction,
									F16::LandingGear.wheelRight.CxWheelFriction,
									F16::LandingGear.wheelRight.CzWheelFriction,
									F16::LandingGear.wheelNose.CxWheelFriction,
									F16::LandingGear.wheelNose.CzWheelFriction);

		// just braking force, needs refining
		F16::Motion.updateBrakingFriction(F16::LandingGear.wheelLeft.brakeForce, F16::LandingGear.wheelRight.brakeForce);

		// use free-rolling friction as single unit for now
		// TODO: nose-wheel steering, braking forces etc.
		F16::Motion.updateNoseWheelTurn(F16::LandingGear.getNoseTurnDirection(), F16::LandingGear.getNosegearAngle());
	}

	// testing, check
	//F16::Motion.updateInertia();
}

/*
called before simulation to set up your environment for the next step
give parameters of surface under your aircraft usefull for ground effect
*/
void ed_fm_set_surface (double		h, //surface height under the center of aircraft
						double		h_obj, //surface height with objects
						unsigned		surface_type,
						double		normal_x, //components of normal vector to surface
						double		normal_y, //components of normal vector to surface
						double		normal_z //components of normal vector to surface
						)
{
	F16::Ground.setSurface(h, h_obj, surface_type, Vec3(normal_x, normal_y, normal_z));
}

void ed_fm_set_atmosphere(	double h,//altitude above sea level			(meters)
							double t,//current atmosphere temperature   (Kelvin)
							double a,//speed of sound					(meters/sec)
							double ro,// atmosphere density				(kg/m^3)
							double p,// atmosphere pressure				(N/m^2)
							double wind_vx,//components of velocity vector, including turbulence in world coordinate system (meters/sec)
							double wind_vy,//components of velocity vector, including turbulence in world coordinate system (meters/sec)
							double wind_vz //components of velocity vector, including turbulence in world coordinate system (meters/sec)
						)
{
	F16::Atmos.setAtmosphere(t, ro, a, h, p);
}

void ed_fm_set_current_mass_state ( double mass, // "dry" mass in kg (not including fuel)
									double center_of_mass_x,
									double center_of_mass_y,
									double center_of_mass_z,
									double moment_of_inertia_x,
									double moment_of_inertia_y,
									double moment_of_inertia_z
									)
{
	F16::Motion.setMassState(mass, 
							center_of_mass_x, center_of_mass_y, center_of_mass_z,
							moment_of_inertia_x, moment_of_inertia_y, moment_of_inertia_z);
}

/*
called before simulation to set up your environment for the next step
*/
void ed_fm_set_current_state (double ax,//linear acceleration component in world coordinate system
							double ay,//linear acceleration component in world coordinate system
							double az,//linear acceleration component in world coordinate system
							double vx,//linear velocity component in world coordinate system
							double vy,//linear velocity component in world coordinate system
							double vz,//linear velocity component in world coordinate system
							double px,//center of the body position in world coordinate system
							double py,//center of the body position in world coordinate system
							double pz,//center of the body position in world coordinate system
							double omegadotx,//angular accelearation components in world coordinate system
							double omegadoty,//angular accelearation components in world coordinate system
							double omegadotz,//angular accelearation components in world coordinate system
							double omegax,//angular velocity components in world coordinate system
							double omegay,//angular velocity components in world coordinate system
							double omegaz,//angular velocity components in world coordinate system
							double quaternion_x,//orientation quaternion components in world coordinate system
							double quaternion_y,//orientation quaternion components in world coordinate system
							double quaternion_z,//orientation quaternion components in world coordinate system
							double quaternion_w //orientation quaternion components in world coordinate system
							)
{
	F16::FlightControls.setCurrentState(ax, ay, az);
}

void ed_fm_set_current_state_body_axis(	double ax,//linear acceleration component in body coordinate system (meters/sec^2)
										double ay,//linear acceleration component in body coordinate system (meters/sec^2)
										double az,//linear acceleration component in body coordinate system (meters/sec^2)
										double vx,//linear velocity component in body coordinate system (meters/sec)
										double vy,//linear velocity component in body coordinate system (meters/sec)
										double vz,//linear velocity component in body coordinate system (meters/sec)
										double wind_vx,//wind linear velocity component in body coordinate system (meters/sec)
										double wind_vy,//wind linear velocity component in body coordinate system (meters/sec)
										double wind_vz,//wind linear velocity component in body coordinate system (meters/sec)
										double omegadotx,//angular accelearation components in body coordinate system (rad/sec^2)
										double omegadoty,//angular accelearation components in body coordinate system (rad/sec^2)
										double omegadotz,//angular accelearation components in body coordinate system (rad/sec^2)
										double omegax,//angular velocity components in body coordinate system (rad/sec)
										double omegay,//angular velocity components in body coordinate system (rad/sec)
										double omegaz,//angular velocity components in body coordinate system (rad/sec)
										double yaw,  //radians (rad)
										double pitch,//radians (rad/sec)
										double roll, //radians (rad/sec)
										double common_angle_of_attack, //AoA  (rad)
										double common_angle_of_slide   //AoS  (rad)
										)
{
	F16::Atmos.setAirspeed(vx, vy, vz, wind_vx, wind_vy, wind_vz);

	// set values for later
	F16::FlightControls.setBodyAxisState(common_angle_of_attack, common_angle_of_slide, omegax, omegay, omegaz, ax, ay, az);
}

// list of input enums kept in separate header for easier documenting..
//
// Command = Command Index (See Export.lua), Value = Signal Value (-1 to 1 for Joystick Axis)
void ed_fm_set_command(int command, float value)
{
	//----------------------------------
	// Set F-16 Raw Inputs
	//----------------------------------

	switch (command)
	{
	case F16::JoystickRoll:
		F16::latStickInputRaw = value; // just for cockpit anim

		F16::FlightControls.setLatStickInput(value);
		break;

	case F16::JoystickPitch:
		F16::longStickInputRaw = value; // just for cockpit anim

		F16::FlightControls.setLongStickInput(value);
		break;

	case F16::JoystickYaw:
		F16::pedInputRaw = value; // just for cockpit anim

		F16::FlightControls.setPedInput(value);
		F16::LandingGear.nosewheelTurn(value); // <- does nothing if not enabled or no weight on wheels
		break;

	case F16::JoystickThrottle:
		F16::EMS.setThrottleInput(value);
		break;

	case F16::ApuStart:
		F16::EMS.JfsStart();
		break;
	case F16::ApuStop:
		F16::EMS.JfsStop();
		break;

	case F16::EnginesStart: // "quickstart" shortcut
		F16::EMS.startEngine();
		break;
	case F16::EnginesStop: // "quickstop" shortcut
		F16::EMS.stopEngine();
		break;

	case F16::PowerOnOff: // <- not working now
		// electric system (FC3 style "all in one")
		F16::Electrics.toggleElectrics(); 
		/*
		swprintf(dbgmsg, 255, L" F16::power on/off (FC3): %d value: %f \r\n", command, value);
		::OutputDebugString(dbgmsg);
		*/
		break;

	case F16::BatteryPower:
		F16::Electrics.toggleBatteryOnOff();
		break;

	case F16::AirBrake:
		F16::FlightControls.switchAirbrake();
		break;
	case F16::AirBrakeOn:
		F16::FlightControls.setAirbrakeON();
		break;
	case F16::AirBrakeOff:
		F16::FlightControls.setAirbrakeOFF();
		break;

		// analog input (axis)
	case F16::WheelBrake:
		F16::LandingGear.setWheelBrakeLeft(value);
		F16::LandingGear.setWheelBrakeRight(value);
		break;
	case F16::WheelBrakeLeft:
		F16::LandingGear.setWheelBrakeLeft(value);
		break;
	case F16::WheelBrakeRight:
		F16::LandingGear.setWheelBrakeRight(value);
		break;

		// switch/button input (keyboard)
	case F16::WheelBrakesOn:
		F16::LandingGear.setWheelBrakesON();
		// when button pressed (down)
		break;
	case F16::WheelBrakesOff:
		F16::LandingGear.setWheelBrakesOFF();
		// when button released (up)
		break;

	case F16::ManualPitchOverride:
		F16::FlightControls.setManualPitchOverride(value);
		break;

	case F16::Gear:
		F16::LandingGear.switchGearUpDown();
		break;
	case F16::LandingGearUp:
		F16::LandingGear.setGearUp();
		break;
	case F16::LandingGearDown:
		F16::LandingGear.setGearDown();
		break;

		// flaps normally controlled by landing gear lever,
		// in case alternate switch is used
	case F16::FlapsOnOff:
		F16::FlightControls.toggleAltFlaps();
		break;
	case F16::FlapsOn:
		F16::FlightControls.setAltFlaps(true);
		break;
	case F16::FlapsOff:
		F16::FlightControls.setAltFlaps(false);
		break;

	case F16::NoseWheelSteering:
		// value includes status of it?
		F16::LandingGear.toggleNosewheelSteering();
		// no animation for nosewheel yet?
		/*
		swprintf(dbgmsg, 255, L" F16::nosewheelsteering: %d value: %f \r\n", command, value);
		::OutputDebugString(dbgmsg);
		*/
		break;

	case F16::TrimPitchDown:
		F16::FlightControls.trimState.pitchDown();
		break;
	case F16::TrimPitchUp:
		F16::FlightControls.trimState.pitchUp();
		break;

	case F16::TrimRollCCW:
		F16::FlightControls.trimState.rollCCW();
		break;
	case F16::TrimRollCW:
		F16::FlightControls.trimState.rollCW();
		break;

	case F16::TrimYawLeft:
		F16::FlightControls.trimState.yawLeft();
		break;
	case F16::TrimYawRight:
		F16::FlightControls.trimState.yawRight();
		break;

	case 215: // trimstop command (key up)
		break;

	case F16::NavigationLights:
		F16::Airframe.toggleNavigationLights();
		break;

	case F16::OxygenNormal:
		F16::EnvCS.setOxygenSystem(value);
		break;

	case F16::Canopy:
		// on/off toggle (needs some actuator support as well)
		F16::Airframe.canopyToggle();
		break;

	case F16::EjectPlane:
		// pilot ejected
		F16::Airframe.onEject();
		break;


		/**/
	case 2142:
	case 2143:
		// ignore these, they are noisy
		// (mouse x & y?)
		break;
		/**/

	default:
		{
			swprintf(dbgmsg, 255, L" F16::unknown command: %d value: %f \r\n", command, value);
			::OutputDebugString(dbgmsg);
			// do nothing
		}
		break;
	}
}

/*
	Mass handling 

	will be called  after ed_fm_simulate :
	you should collect mass changes in ed_fm_simulate 

	double delta_mass = 0;
	double x = 0;
	double y = 0; 
	double z = 0;
	double piece_of_mass_MOI_x = 0;
	double piece_of_mass_MOI_y = 0; 
	double piece_of_mass_MOI_z = 0;
 
	//
	while (ed_fm_change_mass(delta_mass,x,y,z,piece_of_mass_MOI_x,piece_of_mass_MOI_y,piece_of_mass_MOI_z))
	{
	//internal DCS calculations for changing mass, center of gravity,  and moments of inertia
	}
*/
/*
if (fuel_consumption_since_last_time > 0)
{
	delta_mass		 = fuel_consumption_since_last_time;
	delta_mass_pos_x = -1.0;
	delta_mass_pos_y =  1.0;
	delta_mass_pos_z =  0;

	delta_mass_moment_of_inertia_x	= 0;
	delta_mass_moment_of_inertia_y	= 0;
	delta_mass_moment_of_inertia_z	= 0;

	fuel_consumption_since_last_time = 0; // set it 0 to avoid infinite loop, because it called in cycle 
	// better to use stack like structure for mass changing 
	return true;
}
else 
{
	return false;
}
*/
bool ed_fm_change_mass(double & delta_mass,
						double & delta_mass_pos_x,
						double & delta_mass_pos_y,
						double & delta_mass_pos_z,
						double & delta_mass_moment_of_inertia_x,
						double & delta_mass_moment_of_inertia_y,
						double & delta_mass_moment_of_inertia_z
						)
{
	// TODO: better integration of fuel mass position and actual fuel usage calculation
	if (F16::Motion.isMassChanged() == true)
	{
		F16::Motion.getMassMomentInertiaChange(delta_mass, 
											delta_mass_pos_x, 
											delta_mass_pos_y, 
											delta_mass_pos_z,
											delta_mass_moment_of_inertia_x, 
											delta_mass_moment_of_inertia_y, 
											delta_mass_moment_of_inertia_z);
		// Can't set to true...crashing right now :(
		return false;
	}

	return false;
}

/*
	set internal fuel volume , init function, called on object creation and for refueling , 
	you should distribute it inside at different fuel tanks
*/
void ed_fm_set_internal_fuel(double fuel)
{
	// internal fuel in kg
	F16::Fuel.setInternalFuel(fuel);
}

/*
	get internal fuel volume 
*/
double ed_fm_get_internal_fuel()
{
	return F16::Fuel.getInternalFuel();
}

/*
	set external fuel volume for each payload station , called for weapon init and on reload
*/
void ed_fm_set_external_fuel(int station,
								double fuel,
								double x,
								double y,
								double z)
{
	F16::Fuel.setExternalFuel(station, fuel, x, y, z);
}

/*
	get external fuel volume 
*/
double ed_fm_get_external_fuel ()
{
	return F16::Fuel.getExternalFuel();
}

/*
	incremental adding of fuel in case of refueling process 
*/
void ed_fm_refueling_add_fuel(double fuel)
{
	/*
	swprintf(dbgmsg, 255, L" F16::add fuel: %f \r\n", fuel);
	::OutputDebugString(dbgmsg);
	*/

	return F16::Fuel.refuelAdd(fuel);
}

// see arguments in: Draw arguments for aircrafts 1.1.pdf
// also (more accurate) use ModelViewer for details in F-16DCS.edm
//
void ed_fm_set_draw_args(EdDrawArgument * drawargs, size_t size)
{
	// nose gear
	drawargs[0].f = (float)F16::LandingGear.wheelNose.getStrutAngle(); // gear angle {0=retracted;1=extended}
	drawargs[1].f = (float)F16::LandingGear.wheelNose.getStrutCompression(); // strut compression {0=extended;0.5=parking;1=retracted}

	//Nose Gear Steering
	// note: not active animation on the 3D model right now
	drawargs[2].f = F16::LandingGear.getNosegearTurn(); // nose gear turn angle {-1=CW max;1=CCW max}

	// right gear
	drawargs[3].f = (float)F16::LandingGear.wheelRight.getStrutAngle(); // gear angle {0;1}
	drawargs[4].f = (float)F16::LandingGear.wheelRight.getStrutCompression(); // strut compression {0;0.5;1}

	// left gear
	drawargs[5].f = (float)F16::LandingGear.wheelLeft.getStrutAngle(); // gear angle {0;1}
	drawargs[6].f = (float)F16::LandingGear.wheelLeft.getStrutCompression(); // strut compression {0;0.5;1}

	drawargs[9].f = F16::FlightControls.getFlapRSDraw(); // right flap (trailing edge surface)
	drawargs[10].f = F16::FlightControls.getFlapLSDraw(); // left flap (trailing edge surface)

	drawargs[11].f = F16::FlightControls.getAileronRSDraw(); // right aileron (trailing edge surface)
	drawargs[12].f = F16::FlightControls.getAileronLSDraw(); // left aileron (trailing edge surface)

	drawargs[13].f = F16::FlightControls.getLefRSDraw(); // right slat (leading edge) <- limited
	drawargs[14].f = F16::FlightControls.getLefLSDraw(); // left slat (leading edge) <- negative angle works

	drawargs[15].f = F16::FlightControls.getElevatorRSDraw(); // right elevator
	drawargs[16].f = F16::FlightControls.getElevatorLSDraw(); // left elevator

	drawargs[17].f = F16::FlightControls.getRudderDraw(); // right rudder

	drawargs[22].f = F16::Airframe.getRefuelingDoorAngle(); // refueling door (not implemented)

	drawargs[28].f = (float)F16::EMS.getAfterburnerDraw(); // afterburner right engine
	//drawargs[290..291].f // nozzle rotation?
	//drawargs[290].f = (float)F16::EMS.getNozzlePos(); // nozzle rotation?
	//drawargs[291].f = (float)F16::EMS.getNozzlePos(); // nozzle rotation?


	drawargs[38].f = F16::Airframe.getCanopyAngle(); // draw angle of canopy {0=closed;0.9=elevated;1=no draw}

	drawargs[50].f = F16::Airframe.getEjectingSeatDraw(); // ejecting seat in plane

	//drawargs[182].f // right-side brake flaps 0..1
	//drawargs[186].f // left-side brake flaps 0..1
	drawargs[182].f = F16::FlightControls.getAirbrakeRSDraw();
	drawargs[186].f = F16::FlightControls.getAirbrakeLSDraw();

	// navigation lights
	drawargs[49].f = F16::Airframe.isNavigationLight();

	// formation lights
	drawargs[88].f = F16::Airframe.isFormationLight();

	// landing lights
	drawargs[51].f = F16::Airframe.isLandingLight();

	// strobe lights
	drawargs[83].f = F16::Airframe.isStrobeLight();

	// note: current 3D model has three "lamps" implemented:
	// 190 left
	// 196 right
	// 203 tail (back)
	drawargs[190].f = F16::Airframe.getLeftLight();
	drawargs[196].f = F16::Airframe.getRightLight();
	drawargs[203].f = F16::Airframe.getBackLight();

	// following are not all implemented
	//drawargs[190..199].f // nav light #1..10
	//drawargs[200..207].f // formation light #1..8
	//drawargs[208..212].f // landing lamp #1..5

	//drawargs[336].f // cap of brake parachute (not implemented)

	/*
	// !! template has this addition, what are those values really for?
	// (documentation?)
	if (size > 616)
	{
		drawargs[611].f = drawargs[0].f;
		drawargs[614].f = drawargs[3].f;
		drawargs[616].f = drawargs[5].f;
	}
	*/
}

/*
	update draw arguments for your aircraft 
	also same prototype is used for ed_fm_set_fc3_cockpit_draw_args  : direct control over cockpit arguments , 
	it can be use full for FC3 cockpits reintegration with new flight model 
	size: count of elements in array
*/
// use ModelViewer for details in Cockpit-Viper.edm
void ed_fm_set_fc3_cockpit_draw_args(EdDrawArgument * drawargs,size_t size)
{
	/*
		-- stick arg 2 and 3 match args in cockpit edm, anim should work with proper input binding
		StickPitch.arg_number			= 2
		StickPitch.input				= {-100, 100}
		StickPitch.output				= {-1, 1}
		StickPitch.controller			= controllers.base_gauge_StickPitchPosition
		StickBank.arg_number			= 3
		StickBank.input					= {-100, 100}
		StickBank.output				= {-1, 1}
		StickBank.controller			= controllers.base_gauge_StickRollPosition
	*/
	// yay! this seems to work ok!
	// TODO: movement is really small in real-life -> limit movements (1/4 inches both axes)
	// currently you can use this to check joystick vs. in-cockpit movement
	drawargs[2].f = (float)F16::longStickInputRaw;
	drawargs[3].f = (float)F16::latStickInputRaw;

	//F16::EMS.getThrottleInput()

}

/*
shake level amplitude for head simulation , 
*/
double ed_fm_get_shake_amplitude()
{
	return 0;
}

/*
will be called for your internal configuration
*/
void ed_fm_configure(const char * cfg_path)
{
}

/*
will be called for your internal configuration
void ed_fm_release   called when fm not needed anymore : aircraft death etc.
*/
void ed_fm_release()
{
	// we are calling destructors automatically but could cleanup earlier here..
}

// see enum ed_fm_param_enum in wHumanCustomPhysicsAPI.h
double ed_fm_get_param(unsigned param_enum)
{
	switch (param_enum)
	{
		// APU parameters at engine index 0
	case ED_FM_ENGINE_0_RPM:
		return F16::EMS.JFS.getRpm();
	case ED_FM_ENGINE_0_RELATED_RPM:
		return F16::EMS.JFS.getRelatedRpm();
	case ED_FM_ENGINE_0_CORE_RPM:
	case ED_FM_ENGINE_0_CORE_RELATED_RPM:
	case ED_FM_ENGINE_0_THRUST:
	case ED_FM_ENGINE_0_RELATED_THRUST:
	case ED_FM_ENGINE_0_CORE_THRUST:
	case ED_FM_ENGINE_0_CORE_RELATED_THRUST:
		// not implemented now
		return 0;
	case ED_FM_ENGINE_0_TEMPERATURE:
		return F16::EMS.JFS.getTemperature();
	case ED_FM_ENGINE_0_OIL_PRESSURE:
		return F16::EMS.JFS.getOilPressure();
	case ED_FM_ENGINE_0_FUEL_FLOW:
		return F16::EMS.JFS.getFuelFlow();

	case ED_FM_ENGINE_1_RPM:
		return F16::EMS.getEngineRpm();
	case ED_FM_ENGINE_1_RELATED_RPM:
		return F16::EMS.getEngineRelatedRpm();
	case ED_FM_ENGINE_1_THRUST:
		return F16::EMS.getEngineThrust();
	case ED_FM_ENGINE_1_RELATED_THRUST:
		return F16::EMS.getEngineRelatedThrust();
	case ED_FM_ENGINE_1_CORE_RPM:
		return F16::EMS.getEngineRpm();
	case ED_FM_ENGINE_1_CORE_RELATED_RPM:
		return F16::EMS.getEngineRelatedRpm();
	case ED_FM_ENGINE_1_CORE_THRUST:
		return F16::EMS.getEngineThrust();
	case ED_FM_ENGINE_1_CORE_RELATED_THRUST:
		return F16::EMS.getEngineRelatedThrust();
	case ED_FM_ENGINE_1_TEMPERATURE:
		return F16::EMS.Engine.getEngineTemperature();
	case ED_FM_ENGINE_1_OIL_PRESSURE:
		return F16::EMS.Engine.getOilPressure();
	case ED_FM_ENGINE_1_FUEL_FLOW:
		return F16::EMS.Engine.getFuelFlow();
	case ED_FM_ENGINE_1_COMBUSTION:
		// not implemented now
		return 0;

	case ED_FM_SUSPENSION_0_GEAR_POST_STATE: // from 0 to 1 : from fully retracted to full released
		return F16::LandingGear.wheelNose.getStrutAngle(); // <- already has limit
	case ED_FM_SUSPENSION_0_RELATIVE_BRAKE_MOMENT:
	case ED_FM_SUSPENSION_0_UP_LOCK:
	case ED_FM_SUSPENSION_0_DOWN_LOCK:
	case ED_FM_SUSPENSION_0_WHEEL_YAW: // steering angle? or strut sideways movement?
	case ED_FM_SUSPENSION_0_WHEEL_SELF_ATTITUDE:
		return 0;

	case ED_FM_SUSPENSION_1_GEAR_POST_STATE: // from 0 to 1 : from fully retracted to full released
		return F16::LandingGear.wheelRight.getStrutAngle(); // <- already has limit
	case ED_FM_SUSPENSION_1_RELATIVE_BRAKE_MOMENT:
	case ED_FM_SUSPENSION_1_UP_LOCK:
	case ED_FM_SUSPENSION_1_DOWN_LOCK:
	case ED_FM_SUSPENSION_1_WHEEL_YAW:
	case ED_FM_SUSPENSION_1_WHEEL_SELF_ATTITUDE:
		return 0;

	case ED_FM_SUSPENSION_2_GEAR_POST_STATE: // from 0 to 1 : from fully retracted to full released
		return F16::LandingGear.wheelLeft.getStrutAngle(); // <- already has limit
	case ED_FM_SUSPENSION_2_RELATIVE_BRAKE_MOMENT:
	case ED_FM_SUSPENSION_2_UP_LOCK:
	case ED_FM_SUSPENSION_2_DOWN_LOCK:
	case ED_FM_SUSPENSION_2_WHEEL_YAW:
	case ED_FM_SUSPENSION_2_WHEEL_SELF_ATTITUDE:
		return 0;

	case ED_FM_OXYGEN_SUPPLY: // oxygen provided from on board oxygen system, pressure - pascal
		return F16::EnvCS.getCockpitPressure();
	case ED_FM_FLOW_VELOCITY:
		return 0;

	case ED_FM_CAN_ACCEPT_FUEL_FROM_TANKER: // return positive value if all conditions are matched to connect to tanker and get fuel
		// TODO: refueling door handling, collision box reduction
		return 0;

	// Groups for fuel indicator
	case ED_FM_FUEL_FUEL_TANK_GROUP_0_LEFT:			// Values from different group of tanks
	case ED_FM_FUEL_FUEL_TANK_GROUP_0_RIGHT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_1_LEFT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_1_RIGHT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_2_LEFT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_2_RIGHT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_3_LEFT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_3_RIGHT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_4_LEFT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_4_RIGHT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_5_LEFT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_5_RIGHT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_6_LEFT:
	case ED_FM_FUEL_FUEL_TANK_GROUP_6_RIGHT:
		return 0;

	case ED_FM_FUEL_INTERNAL_FUEL:
		return F16::Fuel.getInternalFuel();
	case ED_FM_FUEL_TOTAL_FUEL:
		return F16::Fuel.getInternalFuel() + F16::Fuel.getExternalFuel();

	case ED_FM_FUEL_LOW_SIGNAL:
		// Low fuel signal
		return F16::Fuel.isLowFuel();

	case ED_FM_ANTI_SKID_ENABLE:
		/* return positive value if anti skid system is on , it also corresspond with suspension table "anti_skid_installed"  parameter for each gear post .i.e 
		anti skid system will be applied only for those wheels who marked with   anti_skid_installed = true
		*/
		return 0;

	case ED_FM_COCKPIT_PRESSURIZATION_OVER_EXTERNAL: 
		// additional pressure from pressurization system , pascal , actual cabin pressure will be AtmoPressure + COCKPIT_PRESSURIZATION_OVER_EXTERNAL
		return F16::EnvCS.getCockpitPressure();

	default:
		// silence compiler warning(s)
		break;
	}
	return 0;	
}

void ed_fm_cold_start()
{
	swprintf(dbgmsg, 255, L" F16ADemo:: cold start %hs \r\n", __DATE__);
	::OutputDebugString(dbgmsg);

	// landing gear down
	// canopy open
	// electrics off
	// engine off

	// input does not work correctly yet
	F16::LandingGear.initGearsDown();
	F16::Airframe.initCanopyOpen();
	F16::FlightControls.initAirBrakeOff();
	F16::Electrics.setElectricsOnOff(false); // <- off
	F16::EMS.initEngineOff(); // <- stop
}

void ed_fm_hot_start()
{
	swprintf(dbgmsg, 255, L" F16ADemo:: hot start %hs \r\n", __DATE__);
	::OutputDebugString(dbgmsg);

	// landing gear down
	// canopy closed
	// electrics on
	// engine on
	F16::LandingGear.initGearsDown();
	F16::Airframe.initCanopyClosed();
	F16::FlightControls.initAirBrakeOff();
	F16::Electrics.setElectricsOnOff(true);
	F16::EMS.initEngineIdle();
}

void ed_fm_hot_start_in_air()
{
	swprintf(dbgmsg, 255, L" F16ADemo:: hot start in air %hs \r\n", __DATE__);
	::OutputDebugString(dbgmsg);

	// landing gear up
	// canopy closed
	// electrics on
	// engine on
	F16::LandingGear.initGearsUp();
	F16::Airframe.initCanopyClosed();
	F16::FlightControls.initAirBrakeOff();
	F16::Electrics.setElectricsOnOff(true);
	F16::EMS.initEngineCruise();
}

/* 
for experts only : called  after ed_fm_hot_start_in_air for balance FM at actual speed and height , it is directly force aircraft dynamic data in case of success 
*/
bool ed_fm_make_balance (double & ax,//linear acceleration component in world coordinate system);
									double & ay,//linear acceleration component in world coordinate system
									double & az,//linear acceleration component in world coordinate system
									double & vx,//linear velocity component in world coordinate system
									double & vy,//linear velocity component in world coordinate system
									double & vz,//linear velocity component in world coordinate system
									double & omegadotx,//angular accelearation components in world coordinate system
									double & omegadoty,//angular accelearation components in world coordinate system
									double & omegadotz,//angular accelearation components in world coordinate system
									double & omegax,//angular velocity components in world coordinate system
									double & omegay,//angular velocity components in world coordinate system
									double & omegaz,//angular velocity components in world coordinate system
									double & yaw,  //radians
									double & pitch,//radians
									double & roll)//radians
{
	return false;
}

/*
enable additional information like force vector visualization , etc.
*/
bool ed_fm_enable_debug_info()
{
	return false;
}

/*debuf watch output for topl left corner DCS window info  (Ctrl + Pause to show)
ed_fm_debug_watch(int level, char *buffer,char *buf,size_t maxlen)
level - Watch verbosity level.
ED_WATCH_BRIEF   = 0,
ED_WATCH_NORMAL  = 1,
ED_WATCH_FULL	 = 2,

return value  - amount of written bytes

using

size_t ed_fm_debug_watch(int level, char *buffer,size_t maxlen)
{
	float  value_1 = .. some value;
	float  value_2 = .. some value;
	//optional , you can change depth of displayed information with level 
	switch (level)
	{
		case 0:     //ED_WATCH_BRIEF,
			..do something
			break;
		case 1:     //ED_WATCH_NORMAL,
			..do something
		break;
		case 2:     //ED_WATCH_FULL,
			..do something
		break;
	}
	... do something 
	if (do not want to display)
	{	
		return 0;
	}
	else // ok i want to display some vars 
	{    
		return sprintf_s(buffer,maxlen,"var value1 %f ,var value2 %f",value_1,value_2);
	}
}
*/
size_t ed_fm_debug_watch(int level, char *buffer,size_t maxlen)
{
	/*
	if (level < ED_WATCH_FULL)
	{
		return 0;
	}
	*/
	/*
	if (level == 1 || level == 2)
	{
		return sprintf_s(buffer, maxlen, "fuel: %f", F16::Fuel.getInternalFuel());
	}
	*/

	/* not functional at moment
	if (level > 0)
	{
		return _snprintf(buffer, maxlen, "F16:: dynP: %f Vt: %f",
			F16::Atmos.dynamicPressure, F16::Atmos.totalVelocity);
	}
	*/
	return 0;
}

/* 
path to your plugin installed
*/
void ed_fm_set_plugin_data_install_path(const char * path)
{
}

// damages and failures
// callback when preplaneed failure triggered from mission 
void ed_fm_on_planned_failure(const char * data)
{
}

// callback when damage occurs for airframe element 
void ed_fm_on_damage(int Element, double element_integrity_factor)
{
	/*
	//TODO: check what is needed to get these
	swprintf(dbgmsg, 255, L" F16::Damage: element: %d factor: %f \r\n", Element, element_integrity_factor);
	::OutputDebugString(dbgmsg);
	*/

	// keep integrity information in airframe
	F16::Airframe.onAirframeDamage(Element, element_integrity_factor);
}

// called in case of repair routine 
void ed_fm_repair()
{
	F16::Airframe.onRepair();
}

// in case of some internal damages or system failures this function return true , to switch on repair process
bool ed_fm_need_to_be_repaired()
{
	return F16::Airframe.isRepairNeeded();
}

// inform about  invulnerability settings
void ed_fm_set_immortal(bool value)
{
	F16::Airframe.setImmortal(value);
}

// inform about  unlimited fuel
void ed_fm_unlimited_fuel(bool value)
{
	F16::Fuel.setUnlimitedFuel(value);
}

// inform about simplified flight model request 
void ed_fm_set_easy_flight(bool value)
{
}

// custom properties sheet 
void ed_fm_set_property_numeric(const char * property_name,float value)
{
	/*
	// TODO: do we set these somewhere in lua?
	swprintf(dbgmsg, 255, L" F16::property numeric: %s value: %f \r\n", property_name, value);
	::OutputDebugString(dbgmsg);
	*/
}

// custom properties sheet 
void ed_fm_set_property_string(const char * property_name,const char * value)
{
	/*
	// TODO: do we set these somewhere in lua?
	swprintf(dbgmsg, 255, L" F16::property string: %s value: %s \r\n", property_name, value);
	::OutputDebugString(dbgmsg);
	*/
}

/*
	called on each sim step 

	ed_fm_simulation_event event;
	while (ed_fm_pop_simulation_event(event))
	{
		do some with event data;
	}
*/
bool ed_fm_pop_simulation_event(ed_fm_simulation_event & out)
{
	// something like this when triggered? (reset return value after output)
	//out.event_type = ED_FM_EVENT_FAILURE;
	//memcpy(out.event_message, "autopilot", strlen("autopilot"));

	return false;
}

/*
	feedback to your fm about suspension state
*/
void ed_fm_suspension_feedback(int idx,const ed_fm_suspension_info * info)
{
	// TODO: update landing gears
	//info->acting_force;
	//info->acting_force_point;
	//info->integrity_factor;
	//info->struct_compression;

	/*
	swprintf(dbgmsg, 255, L" F16::Suspension: idx: %d comp: %f \r\n", idx, info->struct_compression);
	::OutputDebugString(dbgmsg);
	*/

	switch (idx)
	{
	case 0:
		F16::LandingGear.wheelNose.setActingForce(info->acting_force[0], info->acting_force[1], info->acting_force[2]);
		F16::LandingGear.wheelNose.setActingForcePoint(info->acting_force_point[0], info->acting_force_point[1], info->acting_force_point[2]);
		F16::LandingGear.wheelNose.setIntegrityFactor(info->integrity_factor);
		// 0.231
		F16::LandingGear.wheelNose.setStrutCompression(info->struct_compression);
		break;
	case 1:
		F16::LandingGear.wheelRight.setActingForce(info->acting_force[0], info->acting_force[1], info->acting_force[2]);
		F16::LandingGear.wheelRight.setActingForcePoint(info->acting_force_point[0], info->acting_force_point[1], info->acting_force_point[2]);
		F16::LandingGear.wheelRight.setIntegrityFactor(info->integrity_factor);
		// 0.750
		F16::LandingGear.wheelRight.setStrutCompression(info->struct_compression);
		break;
	case 2:
		F16::LandingGear.wheelLeft.setActingForce(info->acting_force[0], info->acting_force[1], info->acting_force[2]);
		F16::LandingGear.wheelLeft.setActingForcePoint(info->acting_force_point[0], info->acting_force_point[1], info->acting_force_point[2]);
		F16::LandingGear.wheelLeft.setIntegrityFactor(info->integrity_factor);
		// 0.750
		F16::LandingGear.wheelLeft.setStrutCompression(info->struct_compression);
		break;

	default:
		break;
	}
}

double test()
{
	return 10.0;
}

