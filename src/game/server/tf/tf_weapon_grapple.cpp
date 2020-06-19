//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//			Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "tf_weapon_grapple.h"
 
#ifdef CLIENT_DLL
	#include "c_tf_player.h"      
    //#include "player.h"                
	#include "c_te_effect_dispatch.h"
    //#include "te_effect_dispatch.h"    
#else                                  
	#include "game.h"                  
    #include "tf_player.h"        
    #include "player.h"                
	#include "te_effect_dispatch.h"
	#include "IEffects.h"
	#include "Sprite.h"
	#include "SpriteTrail.h"
	#include "beam_shared.h"
	#include "explode.h"

	#include "ammodef.h"		/* This is needed for the tracing done later */
	#include "gamestats.h" //
	#include "soundent.h" //
 
	#include "vphysics/constraints.h"
	#include "physics_saverestore.h"
 
#endif

//#include "effect_dispatch_data.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
#define HOOK_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"
#define BOLT_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"

#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500

ConVar of_grappleforce("of_grappleforce", "850", FCVAR_REPLICATED, "The pulling force of the grapple hook's rope.");
ConVar of_grapplereelrate("of_grapplereelrate", "100", FCVAR_REPLICATED, "The length of rope reeled in per second while attached.");
 
#ifndef CLIENT_DLL
 
LINK_ENTITY_TO_CLASS( grapple_hook, CGrappleHook );
 
BEGIN_DATADESC( CGrappleHook )
	// Function Pointers
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_THINKFUNC( HookedThink ),
	DEFINE_FUNCTION( HookTouch ),

	DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hBolt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_bPlayerWasStanding, FIELD_BOOLEAN ),
 
END_DATADESC()
 
CGrappleHook *CGrappleHook::HookCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner )
{
	// Create a new entity with CGrappleHook private data
	CGrappleHook *pHook = (CGrappleHook *)CreateEntityByName( "grapple_hook" );
	UTIL_SetOrigin( pHook, vecOrigin );
	pHook->SetAbsAngles( angAngles );
	pHook->Spawn();
 
	CWeaponGrapple *pOwner = (CWeaponGrapple *)pentOwner;
	pHook->m_hOwner = pOwner;
	pHook->SetOwnerEntity( pOwner->GetOwner() );
	pHook->m_hPlayer = (CTFPlayer *)pOwner->GetOwner();
 
	return pHook;
}

//-----------------------------------------------------------------------------
// Purpose: Shortens the player's rope
//-----------------------------------------------------------------------------
void CGrappleHook::ReelIn() {
	m_flCurrentRopeLength -= of_grapplereelrate.GetFloat() * gpGlobals->interval_per_tick;
	m_flCurrentRopeLength = clamp(m_flCurrentRopeLength, m_flMinRopeLength, m_flMaxRopeLength);
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrappleHook::~CGrappleHook( void )
{ 
	if ( m_hBolt )
	{
		UTIL_Remove( m_hBolt );
		m_hBolt = NULL;
	}
 
	// Revert Jay's gai flag
	if ( m_hPlayer )
		m_hPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, false );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CGrappleHook::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );
 
	return true;
}
 
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CGrappleHook::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}
 
//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CGrappleHook::Spawn( void )
{
	Precache( );
 
	SetModel( HOOK_MODEL );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(1,1,1), Vector(1,1,1) );
	SetSolid( SOLID_BBOX );
	SetGravity( 0.05f );
 
	// The rock is invisible, the crossbow bolt is the visual representation
	AddEffects( EF_NODRAW );
 
	// Make sure we're updated if we're underwater
	UpdateWaterState();
 
	SetTouch( &CGrappleHook::HookTouch );
 
	SetThink( &CGrappleHook::FlyThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
 
	m_pSpring		= NULL;
	m_fSpringLength = 0.0f;
	m_bPlayerWasStanding = false;

	// Create bolt model and parent it
	CBaseEntity *pBolt = CBaseEntity::CreateNoSpawn( "prop_dynamic", GetAbsOrigin(), GetAbsAngles(), this );
	pBolt->SetModelName( MAKE_STRING( BOLT_MODEL ) );
	pBolt->SetModel( BOLT_MODEL );
	DispatchSpawn( pBolt );
	pBolt->SetParent( this );
}
 
 
void CGrappleHook::Precache( void )
{
	PrecacheModel( HOOK_MODEL );
	PrecacheModel( BOLT_MODEL );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CGrappleHook::HookTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;
 
	if ( (pOther != m_hOwner) && (pOther->m_takedamage != DAMAGE_NO) )
	{
		m_hOwner->NotifyHookDied();

		if ( m_hPlayer )
			m_hPlayer->m_Shared.SetGrapple( false );
 
		SetTouch( NULL );
		SetThink( NULL );
 
		UTIL_Remove( this );
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();
 
		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			EmitSound( "Weapon_AR2.Reload_Push" );
 
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
 
			//FIXME: We actually want to stick (with hierarchy) to what we've hit
			SetMoveType( MOVETYPE_NONE );
 
			Vector vForward;
 
			AngleVectors( GetAbsAngles(), &vForward );
			VectorNormalize ( vForward );
 
			CEffectData	data;
 
			data.m_vOrigin = tr.endpos;
			data.m_vNormal = vForward;
			data.m_nEntIndex = 0;
 
		//	DispatchEffect( "Impact", data );
 
		//	AddEffects( EF_NODRAW );
			SetTouch( NULL );

			VPhysicsDestroyObject();
			VPhysicsInitNormal( SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false );
			AddSolidFlags( FSOLID_NOT_SOLID );
		//	SetMoveType( MOVETYPE_NONE );
 
			if ( !m_hPlayer )
			{
				Assert( 0 );
				return;
			}
 
			// Set Jay's gai flag
			m_hPlayer->SetPhysicsFlag( PFLAG_VPHYSICS_MOTIONCONTROLLER, true );

			m_hPlayer->m_Shared.SetGrapple( true );
			m_hPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, ACT_GRAPPLE_PULL_START );

			// Remove any gravity velocity on the player if they grapple above themselves
			if (GetAbsOrigin().z > m_hPlayer->GetAbsOrigin().z) {
				Vector playerVelocity = m_hPlayer->GetAbsVelocity();
				playerVelocity.z = playerVelocity.z > 0 ? playerVelocity.z : 0;
				m_hPlayer->SetAbsVelocity(playerVelocity);
			}

			//IPhysicsObject *pPhysObject = m_hPlayer->VPhysicsGetObject();

			/*
			IPhysicsObject *pRootPhysObject = VPhysicsGetObject();
			Assert( pRootPhysObject );
 
			pRootPhysObject->EnableMotion( false );
 
			// Root has huge mass, tip has little
			pRootPhysObject->SetMass( VPHYSICS_MAX_MASS );
		//	pPhysObject->SetMass( 100 );
		//	float damping = 3;
		//	pPhysObject->SetDamping( &damping, &damping );
			*/
 
			Vector origin = m_hPlayer->GetAbsOrigin();
			Vector rootOrigin = GetAbsOrigin();
			m_fSpringLength = (origin - rootOrigin).Length();
 
			m_bPlayerWasStanding = ( ( m_hPlayer->GetFlags() & FL_DUCKING ) == 0 );
			
			m_flMaxRopeLength = m_fSpringLength;
			m_flCurrentRopeLength = m_flMaxRopeLength;
 
			SetThink( &CGrappleHook::HookedThink );
			SetNextThink( gpGlobals->curtime + 0.1f );
		}
		else
		{
			// Put a mark unless we've hit the sky
			/*if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}*/
 
			SetTouch( NULL );
			SetThink( NULL );
 
			m_hOwner->NotifyHookDied();
			UTIL_Remove( this );
		}
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrappleHook::HookedThink( void )
{
	const float tickTime = 0.05f;
	//set next globalthink
	SetNextThink( gpGlobals->curtime + tickTime ); //0.1f

	//All of this push the player far from the hook
	//Vector tempVec1 = m_hPlayer->GetAbsOrigin() - GetAbsOrigin();
	//VectorNormalize(tempVec1);

	//int temp_multiplier = -1;

	//m_hPlayer->SetGravity(0.0f);
	m_hPlayer->SetGroundEntity(NULL);

	/*if (m_hOwner->m_bHook){
		//temp_multiplier = 1;
		m_hPlayer->SetAbsVelocity(tempVec1*temp_multiplier*45); //50
	}
	else
	{
		m_hPlayer->SetAbsVelocity(tempVec1*temp_multiplier*800); //400
	}*/

	Vector toHook = -(m_hPlayer->GetAbsOrigin() - GetAbsOrigin());
	float distanceToHook = toHook.Length();

	float forceMultipler = 1.0f;

	// If beyond the max rope length:
	if (distanceToHook > m_flCurrentRopeLength) {

		// Square it for exponential pull
		//float stretchDistance = (distanceToHook / m_flCurrentRopeLength) * (distanceToHook / m_flCurrentRopeLength);

		float extension = distanceToHook - m_flCurrentRopeLength;
		forceMultipler = extension;

		// Pull toward the grapple point like a rope swing
		//Vector accelerationTowardCentre = toHook.Normalized() * of_grappleforce.GetFloat() * stretchDistance;
		//m_hPlayer->SetAbsVelocity(m_hPlayer->GetAbsVelocity() + (accelerationTowardCentre * tickTime));
	}
	
	m_hPlayer->SetGroundEntity(NULL);

	Vector accelerationTowardCentre = toHook.Normalized() * of_grappleforce.GetFloat() * forceMultipler;
	m_hPlayer->SetAbsVelocity(m_hPlayer->GetAbsVelocity() + (accelerationTowardCentre * tickTime));
	ReelIn();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrappleHook::FlyThink( void )
{
	QAngle angNewAngles;
 
	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );
 
	SetNextThink( gpGlobals->curtime + 0.1f );
}
 
#endif
 
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGrapple, DT_WeaponGrapple )
 
#ifdef CLIENT_DLL
void RecvProxy_HookDied( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	CWeaponGrapple *pGrapple = ((CWeaponGrapple*)pStruct);
 
	RecvProxy_IntToEHandle( pData, pStruct, pOut );
 
	CBaseEntity *pNewHook = pGrapple->GetHook();
 
	if ( pNewHook == NULL )
	{
		if ( pGrapple->GetOwner() && pGrapple->GetOwner()->GetActiveWeapon() == pGrapple )
		{
			pGrapple->NotifyHookDied();
		}
	}
}
#endif
 
BEGIN_NETWORK_TABLE( CWeaponGrapple, DT_WeaponGrapple )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInZoom ) ),
	RecvPropBool( RECVINFO( m_bMustReload ) ),
	RecvPropEHandle( RECVINFO( m_hHook ), RecvProxy_HookDied ),

	RecvPropInt ( RECVINFO (m_nBulletType)),
#else
	SendPropBool( SENDINFO( m_bInZoom ) ),
	SendPropBool( SENDINFO( m_bMustReload ) ),
	SendPropEHandle( SENDINFO( m_hHook ) ),

	SendPropInt ( SENDINFO (m_nBulletType)),
#endif
END_NETWORK_TABLE()
 
#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponGrapple )
	DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bMustReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif
 
LINK_ENTITY_TO_CLASS( tf_weapon_grapple, CWeaponGrapple );
 
//PRECACHE_WEAPON_REGISTER( tf_weapon_grapple );
 
#ifndef CLIENT_DLL
/*
acttable_t	CWeaponGrapple::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,		ACT_MP_STAND_MELEE,				false },
	{ ACT_MP_CROUCH_IDLE,		ACT_MP_CROUCH_MELEE,			false },
	{ ACT_MP_RUN,				ACT_MP_RUN_MELEE,				false },
	{ ACT_MP_WALK,				ACT_MP_WALK_MELEE,				false },
	{ ACT_MP_AIRWALK,			ACT_MP_AIRWALK_MELEE,			false },
	{ ACT_MP_CROUCHWALK,		ACT_MP_CROUCHWALK_MELEE,		false },
	{ ACT_MP_JUMP,				ACT_MP_JUMP_MELEE,				false },
	{ ACT_MP_JUMP_START,		ACT_MP_JUMP_START_MELEE,		false },
	{ ACT_MP_JUMP_FLOAT,		ACT_MP_JUMP_FLOAT_MELEE,		false },
	{ ACT_MP_JUMP_LAND,			ACT_MP_JUMP_LAND_MELEE,			false },
	{ ACT_MP_SWIM,				ACT_MP_SWIM_MELEE,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_MP_ATTACK_STAND_MELEE,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_MP_ATTACK_CROUCH_MELEE,		false },
	{ ACT_MP_ATTACK_SWIM_PRIMARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_ATTACK_STAND_SECONDARYFIRE,	ACT_MP_ATTACK_STAND_MELEE_SECONDARY, false },
	{ ACT_MP_ATTACK_CROUCH_SECONDARYFIRE,	ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY,false },
	{ ACT_MP_ATTACK_SWIM_SECONDARYFIRE,		ACT_MP_ATTACK_SWIM_MELEE,		false },
	{ ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE,	ACT_MP_ATTACK_AIRWALK_MELEE,	false },

	{ ACT_MP_GESTURE_FLINCH,	ACT_MP_GESTURE_FLINCH_MELEE, false },

	{ ACT_MP_GRENADE1_DRAW,		ACT_MP_MELEE_GRENADE1_DRAW,	false },
	{ ACT_MP_GRENADE1_IDLE,		ACT_MP_MELEE_GRENADE1_IDLE,	false },
	{ ACT_MP_GRENADE1_ATTACK,	ACT_MP_MELEE_GRENADE1_ATTACK,	false },
	{ ACT_MP_GRENADE2_DRAW,		ACT_MP_MELEE_GRENADE2_DRAW,	false },
	{ ACT_MP_GRENADE2_IDLE,		ACT_MP_MELEE_GRENADE2_IDLE,	false },
	{ ACT_MP_GRENADE2_ATTACK,	ACT_MP_MELEE_GRENADE2_ATTACK,	false },

	{ ACT_MP_GESTURE_VC_HANDMOUTH,	ACT_MP_GESTURE_VC_HANDMOUTH_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FINGERPOINT,	ACT_MP_GESTURE_VC_FINGERPOINT_MELEE,	false },
	{ ACT_MP_GESTURE_VC_FISTPUMP,	ACT_MP_GESTURE_VC_FISTPUMP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_THUMBSUP,	ACT_MP_GESTURE_VC_THUMBSUP_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODYES,	ACT_MP_GESTURE_VC_NODYES_MELEE,	false },
	{ ACT_MP_GESTURE_VC_NODNO,	ACT_MP_GESTURE_VC_NODNO_MELEE,	false },                             
};
 
IMPLEMENT_ACTTABLE(CWeaponGrapple);
 */
#endif
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGrapple::CWeaponGrapple( void )
{
	m_bReloadsSingly	= true;
	m_bFiresUnderwater	= true;
	m_bInZoom			= false;
	m_bMustReload		= false;

	m_nBulletType = -1;

	
	#ifndef CLIENT_DLL
		m_pLightGlow= NULL;

		pBeam	= NULL;
	#endif
}
  
//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponGrapple::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "grapple_hook" );
#endif
 
	PrecacheModel( "cable/cable_red.vmt" );
 	PrecacheModel( "cable/cable_blue.vmt" );
	PrecacheModel( "cable/cable_purple.vmt" );

	BaseClass::Precache();
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::PrimaryAttack( void )
{
	// Can't have an active hook out
	if ( m_hHook != NULL )
		return;
 
	#ifndef CLIENT_DLL
	CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

	if ( !pPlayer )
	{
		return;
	}

	//Disabled on MP it makes think the weapon that it needs to reload 
	/*if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}*/

	//m_iPrimaryAttacks++;
	gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );

	//Obligatory for MP so the sound can be played
	CDisablePredictionFiltering disabler;
	WeaponSound( SINGLE );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GRAPPLE_FIRE_START );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	//Disabled so we can shoot all the time that we want
	//m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );	

	//We will not shoot bullets anymore
	//pPlayer->FireBullets( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );

	pPlayer->SetMuzzleFlashTime( gpGlobals->curtime + 0.5 );

	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner() );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}

	trace_t tr;
	Vector vecShootOrigin, vecShootDir, vecDir, vecEnd;

	//Gets the direction where the player is aiming
	AngleVectors (pPlayer->EyeAngles(), &vecDir);

	//Gets the position of the player
	vecShootOrigin = pPlayer->Weapon_ShootPosition();

	//Gets the position where the hook will hit
	vecEnd	= vecShootOrigin + (vecDir * MAX_TRACE_LENGTH);	
	
	//Traces a line between the two vectors
	UTIL_TraceLine( vecShootOrigin, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	//Draws the beam
	DrawBeam( vecShootOrigin, tr.endpos, 2 );

	#endif

	FireHook();
 
	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );
}
 
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::SecondaryAttack( void )
{
	// nothing
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Reload( void )
{
	if ( ( m_bMustReload ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_IDLE ); //ACT_VM_RELOAD
 
		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
 
		//Mark this as done
		m_bMustReload = false;
	}
 
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Toggles between pull and rappel mode
//-----------------------------------------------------------------------------
bool CWeaponGrapple::ToggleHook( void )
{
	#ifndef CLIENT_DLL

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
 
	if ( m_bHook )
	{
		//m_bHook = false;
		ClientPrint(pPlayer,HUD_PRINTCENTER, "Pull mode");
		//return m_bHook;
	}
	else
	{
		//m_bHook = true;
		ClientPrint(pPlayer,HUD_PRINTCENTER, "Rappel mode");
		//return m_bHook;
	}
	#endif
	return !m_bHook;
}
  
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemBusyFrame( void )
{
	// Allow zoom toggling even when we're reloading
	//CheckZoomToggle();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemPostFrame( void )
{
	//Enforces being able to use PrimaryAttack and Secondary Attack
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
	if ( ( pOwner->m_nButtons & IN_ATTACK ) )
	{
		if ( m_flNextPrimaryAttack < gpGlobals->curtime )
		{
			PrimaryAttack();
		}
	}
	else if ( m_bMustReload ) //&& HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}

	if ( ( pOwner->m_afButtonPressed & IN_ATTACK2 ) )
	{
		if ( m_flNextPrimaryAttack < gpGlobals->curtime )
		{
			SecondaryAttack();
		}
	}
	else if ( m_bMustReload ) //&& HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}
 
	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}

#ifndef CLIENT_DLL
	if ( m_hHook )
	{
		if ( !(pOwner->m_nButtons & IN_ATTACK) && !(pOwner->m_nButtons & IN_ATTACK2))
		{
			m_hHook->SetTouch( NULL );
			m_hHook->SetThink( NULL );
 
			UTIL_Remove( m_hHook );
			m_hHook = NULL;

			CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

			if ( pPlayer )
				pPlayer->m_Shared.SetGrapple( false );
	
			NotifyHookDied();
 
			m_bMustReload = true;
		}
	}
#endif
 
//	BaseClass::ItemPostFrame();
}
 
//-----------------------------------------------------------------------------
// Purpose: Fires the hook
//-----------------------------------------------------------------------------
void CWeaponGrapple::FireHook( void )
{
	if ( m_bMustReload )
		return;
 
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
 
	if ( pOwner == NULL )
		return;
 
#ifndef CLIENT_DLL
	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );	
	Vector vecSrc		= pOwner->Weapon_ShootPosition();
 
	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );
 
	CGrappleHook *pHook = CGrappleHook::HookCreate( vecSrc, angAiming, this );
 
	if ( pOwner->GetWaterLevel() == 3 )
	{
		pHook->SetAbsVelocity( vecAiming * BOLT_WATER_VELOCITY );
	}
	else
	{
		pHook->SetAbsVelocity( vecAiming * BOLT_AIR_VELOCITY );
	}
 
	m_hHook = pHook;
 
#endif
 
	//WeaponSound( SINGLE );
	//WeaponSound( SPECIAL2 );
 
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
 
	m_flNextPrimaryAttack = m_flNextSecondaryAttack	= gpGlobals->curtime + 0.75;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Deploy( void )
{
	if ( m_bMustReload )
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}
 
	return BaseClass::Deploy();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponGrapple::Holster( CBaseCombatWeapon *pSwitchingTo )
{
#ifndef CLIENT_DLL
	if ( m_hHook )
	{
		m_hHook->SetTouch( NULL );
		m_hHook->SetThink( NULL );
 
		UTIL_Remove( m_hHook );
		m_hHook = NULL;

		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

		if ( pPlayer )
			pPlayer->m_Shared.SetGrapple( false );
 
		NotifyHookDied();
 
		m_bMustReload = true;
	}
#endif
 
	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::Drop( const Vector &vecVelocity )
{
#ifndef CLIENT_DLL
	if ( m_hHook )
	{
		m_hHook->SetTouch( NULL );
		m_hHook->SetThink( NULL );
 
		UTIL_Remove( m_hHook );
		m_hHook = NULL;

		CTFPlayer *pPlayer = ToTFPlayer( GetPlayerOwner() );

		if ( pPlayer )
			pPlayer->m_Shared.SetGrapple( false );
 
		NotifyHookDied();
 
		m_bMustReload = true;
	}
#endif
 
	BaseClass::Drop( vecVelocity );
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::HasAnyAmmo( void )
{
	if ( m_hHook != NULL )
		return true;
 
	return BaseClass::HasAnyAmmo();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::CanHolster( void )
{
	//Can't have an active hook out
	if ( m_hHook != NULL )
		return false;
 
	return BaseClass::CanHolster();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::NotifyHookDied( void )
{
	m_hHook = NULL;
 
#ifndef CLIENT_DLL
	if ( pBeam )
	{
		UTIL_Remove( pBeam ); //Kill beam
		pBeam = NULL;

		UTIL_Remove( m_pLightGlow ); //Kill sprite
		m_pLightGlow = NULL;
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Draws a beam
// Input  : &startPos - where the beam should begin
//          &endPos - where the beam should end
//          width - what the diameter of the beam should be (units?)
//-----------------------------------------------------------------------------
void CWeaponGrapple::DrawBeam( const Vector &startPos, const Vector &endPos, float width )
{
#ifndef CLIENT_DLL
	//Tracer down the middle (NOT NEEDED, IT WILL FIRE A TRACER)
	//UTIL_Tracer( startPos, endPos, 0, TRACER_DONT_USE_ATTACHMENT, 6500, false, "GaussTracer" );
 
	trace_t tr;
	//Draw the main beam shaft
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer )
	{
		if ( pPlayer->GetTeamNumber() == TF_TEAM_RED  )
			pBeam = CBeam::BeamCreate("cable/cable_red.vmt", 2);
		else if ( pPlayer->GetTeamNumber() == TF_TEAM_BLUE )
			pBeam = CBeam::BeamCreate("cable/cable_blue.vmt", 2);
		else
			pBeam = CBeam::BeamCreate("cable/cable_purple.vmt", 2);
	}
	else
	{
		pBeam = CBeam::BeamCreate("cable/cable_purple.vmt", 2);
	}

	// It starts at startPos
	pBeam->SetStartPos( startPos );
 
	// This sets up some things that the beam uses to figure out where
	// it should start and end
	pBeam->PointEntInit( endPos, this );
 
	// This makes it so that the beam appears to come from the muzzle of the pistol
	pBeam->SetEndAttachment( LookupAttachment("muzzle") );
	pBeam->SetWidth( width );
//	pBeam->SetEndWidth( 0.05f );
 
	// Higher brightness means less transparent
	pBeam->SetBrightness( 255 );
	//pBeam->SetColor( 255, 185+random->RandomInt( -16, 16 ), 40 );
	pBeam->RelinkBeam();

	//Sets scrollrate of the beam sprite 
	float scrollOffset = gpGlobals->curtime + 5.5;
	pBeam->SetScrollRate(scrollOffset);
 
	// The beam should only exist for a very short time
	//pBeam->LiveForTime( 0.1f );

	UpdateWaterState();
 
	SetTouch( &CGrappleHook::HookTouch );
 
	SetThink( &CGrappleHook::FlyThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
#endif
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - used to figure out where to do the effect
//          nDamageType - ???
//-----------------------------------------------------------------------------
void CWeaponGrapple::DoImpactEffect( trace_t &tr, int nDamageType )
{
#ifndef CLIENT_DLL
	if ( (tr.surface.flags & SURF_SKY) == false )
	{
		CPVSFilter filter( tr.endpos );
		te->GaussExplosion( filter, 0.0f, tr.endpos, tr.plane.normal, 0 );
		m_nBulletType = GetAmmoDef()->Index("GaussEnergy");
		UTIL_ImpactTrace( &tr, m_nBulletType );
	}
#endif
}
