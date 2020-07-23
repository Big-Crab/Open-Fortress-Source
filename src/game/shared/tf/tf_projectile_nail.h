//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail Projectile
//
//=============================================================================
#ifndef TF_PROJECTILE_NAIL_H
#define TF_PROJECTILE_NAIL_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "tf_projectile_base.h"

//-----------------------------------------------------------------------------
// Purpose: Identical to a nail except for model used
//-----------------------------------------------------------------------------
class CTFProjectile_Syringe : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Syringe, CTFBaseProjectile );

public:

	CTFProjectile_Syringe();
	~CTFProjectile_Syringe();

	// Creation.
	static CTFProjectile_Syringe *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false );

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void );

	static float	GetInitialVelocity( void ) { return 1000.0; }
};

//-----------------------------------------------------------------------------
// Purpose: Nailgun projectile 
//-----------------------------------------------------------------------------
class CTFProjectile_Nail : public CTFBaseProjectile
{
#define NAIL_SPEED 2000.0f

	DECLARE_CLASS( CTFProjectile_Nail, CTFBaseProjectile );

public:

	CTFProjectile_Nail();
	~CTFProjectile_Nail();

	// Creation.
	static CTFProjectile_Nail *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false );

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void );

	int		m_iExplosionDamage;
	void SetExplosionDamage( int iDamage ) { m_iExplosionDamage = iDamage; }

	float m_flExplosionRadius;
	void SetExplosionRadius( float flRadius ) { m_flExplosionRadius = flRadius; }

	// TODO deleteme, fixed the issue of the stats not being read from the file by giving the projectile a pointer to its firing weapon
	float m_flExplosionForce;
	void SetExplosionForce( float flForce ) { m_flExplosionForce = flForce; }

	CBaseEntity *m_pWeapon;
	void SetWeapon( CBaseEntity *pWeapon ) { m_pWeapon = pWeapon; }

#ifdef GAME_DLL
	virtual void ProjectileTouch( CBaseEntity *pOther );
#endif

	static float	GetInitialVelocity( void ) { return NAIL_SPEED; }
};

//-----------------------------------------------------------------------------
// Purpose: Tranq projectile
//-----------------------------------------------------------------------------
class CTFProjectile_Tranq : public CTFBaseProjectile
{
	DECLARE_CLASS( CTFProjectile_Tranq, CTFBaseProjectile );

public:
	CTFProjectile_Tranq();
	~CTFProjectile_Tranq();

	// Creation. 
	static CTFProjectile_Tranq *Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false );

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void );

#ifdef GAME_DLL
	virtual void ProjectileTouch( CBaseEntity *pOther );
#endif

	static float	GetInitialVelocity( void ) { return 1000.0; }
};


#endif	//TF_PROJECTILE_NAIL_H