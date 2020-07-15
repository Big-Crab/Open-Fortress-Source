//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//			Secondary attack: switches between pull and rapple modes
//
//
//=============================================================================//

#ifndef WEAPON_GRAPPLE_H
#define WEAPON_GRAPPLE_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "Sprite.h"
#include "rope_shared.h"
#include "beam_shared.h"

#ifdef CLIENT_DLL
	#define CWeaponGrapple C_WeaponGrapple
	#define CTFEternalShotgun C_TFEternalShotgun
#else
	#include "props.h"
	#include "te_effect_dispatch.h"
#endif

//-----------------------------------------------------------------------------
// CWeaponGrapple
//-----------------------------------------------------------------------------

class CWeaponGrapple : public CTFWeaponBaseGun       
{                                                            
	DECLARE_CLASS( CWeaponGrapple, CTFWeaponBaseGun ); 

public:

	CWeaponGrapple( void );

	void			Precache( void );
	void			PrimaryAttack( void );
	bool            CanHolster( void );
	void            Drop( const Vector &vecVelocity );
	void			ItemPostFrame( void );
	void			RemoveHook(void);
	

#ifdef GAME_DLL
	void			NotifyHookAttached(void);
	void   			DrawBeam(const Vector &endPos, const float width = 2.f);
	void			DoImpactEffect(trace_t &tr, int nDamageType);
#endif

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	float						m_flCableFuncStartTime;

private:

	void InitiateHook(CTFPlayer * pPlayer, CBaseEntity *hook);

#ifdef GAME_DLL
	CHandle<CBeam>				pBeam;
	CNetworkHandle(CBaseEntity, m_hHook);		//server hook
#define NUM_CABLE_VERTS 250
	Vector vertices[NUM_CABLE_VERTS];			//new cable effect
	bool						m_bRopeExists;
	Vector						m_vCableRight;
	Vector						m_vCableUp;
#else
	EHANDLE						m_hHook;		//client hook relay
#endif

	CNetworkVar(int, m_iAttached);
	CNetworkVar(int, m_nBulletType);
};

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Grapple Hook
//-----------------------------------------------------------------------------
class CGrappleHook : public CBaseCombatCharacter
{
    DECLARE_CLASS( CGrappleHook, CBaseCombatCharacter );

public:

	CGrappleHook(void) {}
    ~CGrappleHook(void);
    void Spawn( void );
    void Precache( void );
	static CGrappleHook *HookCreate( const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner = NULL );
	bool HookLOS();

	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
	CWeaponGrapple *GetOwner(void) { return m_hOwner; }
	Class_T Classify( void ) { return CLASS_NONE; }
 
protected:

    DECLARE_DATADESC();
 
private:

	void HookTouch( CBaseEntity *pOther );
	void FlyThink( void );
  
	CWeaponGrapple		*m_hOwner;
	CTFPlayer			*m_hPlayer;
};
#endif

#endif // WEAPON_GRAPPLE_H