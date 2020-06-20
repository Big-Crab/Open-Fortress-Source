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
#include "tf_weaponbase.h"

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
	static CTFProjectile_Syringe *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false);

	virtual const char *GetProjectileModelName( void );
	virtual float GetGravity( void );

	static float	GetInitialVelocity( void ) { return 1000.0; }
};

//-----------------------------------------------------------------------------
// Purpose: Nailgun projectile 
//-----------------------------------------------------------------------------
// 1000 is the default in OF, 2000 is equivalent to the Quake3 plasma projectile speed
#define NAILSPEED 2000.0f
class CTFProjectile_Nail : public CTFBaseProjectile
{
	DECLARE_CLASS(CTFProjectile_Nail, CTFBaseProjectile);

public:

	CTFProjectile_Nail();
	~CTFProjectile_Nail();

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return NAILSPEED; }

	// Track the original weapon used to fire this nail
	CNetworkHandle(CBaseEntity, m_hOriginalLauncher);
	virtual void	SetLauncher(CBaseEntity *pLauncher) { m_hOriginalLauncher = pLauncher; }
	CBaseEntity		*GetOriginalLauncher(void) { return m_hOriginalLauncher; }

	// New for explosive nails
	//virtual void	ProjectileTouch(CBaseEntity *pOther);
#ifdef GAME_DLL
	// Creation. (Moved to this preprocessor def)
	static CTFProjectile_Nail *Create(CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false);
	void Spawn(void);

	DECLARE_DATADESC();

	virtual float	GetDamage() { return m_flDamage; }
	virtual int		GetDamageType() { return g_aWeaponDamageTypes[GetWeaponID()]; }
	virtual int		GetCustomDamageType();
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }
	virtual void	SetDamageRadius(float flDamageRadius) { m_flDamageRadius = flDamageRadius; }
	virtual float	GetRadius() { return m_flDamageRadius; }

	virtual void	NailTouch(CBaseEntity *pOther);
	virtual void	Explode(trace_t *pTrace, CBaseEntity *pOther);
	int 	m_hWeaponID;

protected:
	// Not networked.
	float					m_flDamage;
	float					m_flDamageRadius;

	CHandle<CBaseEntity>	m_hEnemy;
#endif

//	virtual unsigned int PhysicsSolidMaskForEntity(void) const;

};

//-----------------------------------------------------------------------------
// Purpose: Tranq projectile
//-----------------------------------------------------------------------------
class CTFProjectile_Tranq : public CTFBaseProjectile
{
	DECLARE_CLASS(CTFProjectile_Tranq, CTFBaseProjectile);

public:
	CTFProjectile_Tranq();
	~CTFProjectile_Tranq();

	// Creation.
	static CTFProjectile_Tranq *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false);

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return 1000.0; }
};


#endif	//TF_PROJECTILE_NAIL_H