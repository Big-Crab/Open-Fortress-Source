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
	DECLARE_CLASS(CTFProjectile_Syringe, CTFBaseProjectile);

public:

	CTFProjectile_Syringe();
	~CTFProjectile_Syringe();

	// Creation.
	static CTFProjectile_Syringe *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false);

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return 1000.0; }
};

//-----------------------------------------------------------------------------
// Purpose: Nailgun projectile 
//-----------------------------------------------------------------------------
class CTFProjectile_Nail : public CTFBaseProjectile
{
#define NAIL_SPEED 2000.0f

	DECLARE_CLASS(CTFProjectile_Nail, CTFBaseProjectile);

public:

	CTFProjectile_Nail();
	~CTFProjectile_Nail();

	// Creation.
	static CTFProjectile_Nail *Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL, CBaseEntity *pScorer = NULL, int bCritical = false);

	virtual const char *GetProjectileModelName(void);
	virtual float GetGravity(void);

	static float	GetInitialVelocity(void) { return NAIL_SPEED; }
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

#ifdef GAME_DLL
	virtual void ProjectileTouch(CBaseEntity *pOther);
#endif

	static float	GetInitialVelocity(void) { return 1000.0; }
};


#endif	//TF_PROJECTILE_NAIL_H