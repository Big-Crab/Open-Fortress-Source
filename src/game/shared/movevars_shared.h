//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MOVEVARS_SHARED_H
#define MOVEVARS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"

float GetCurrentGravity( void );

#if defined( OF_CLIENT_DLL ) || defined( OF_DLL )
extern ConVar of_cslide;
extern ConVar of_cslideaccelerate;
extern ConVar of_cslidefriction;

extern int of_iMovementMode;
extern float of_flQ3AirAccelerate;
#endif

extern ConVar sv_gravity;
extern ConVar sv_stopspeed;
extern ConVar sv_noclipaccelerate;
extern ConVar sv_noclipspeed;
extern ConVar sv_maxspeed;
extern ConVar sv_accelerate;
extern ConVar sv_friction;
extern ConVar sv_bounce;
extern ConVar sv_maxvelocity;
extern ConVar sv_stepsize;
extern ConVar sv_skyname;
extern ConVar sv_backspeed;
extern ConVar sv_waterdist;
extern ConVar sv_specaccelerate;
extern ConVar sv_specspeed;
extern ConVar sv_specnoclip;
extern float sv_flAirAccelerate;
extern float sv_flWaterAccelerate;
extern float sv_flWaterFriction;
extern bool sv_bFootsteps;
extern float sv_flRollSpeed;
extern float sv_flRollAngle;
// Vehicle convars
extern ConVar r_VehicleViewDampen;

// Jeep convars
extern ConVar r_JeepViewDampenFreq;
extern ConVar r_JeepViewDampenDamp;
extern ConVar r_JeepViewZHeight;

// Airboat convars
extern ConVar r_AirboatViewDampenFreq;
extern ConVar r_AirboatViewDampenDamp;
extern ConVar r_AirboatViewZHeight;

#endif // MOVEVARS_SHARED_H
