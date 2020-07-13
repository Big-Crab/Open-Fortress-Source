//====== Copyright � 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_SHOTGUN_H
#define TF_WEAPON_SHOTGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grapple.h"

#ifdef CLIENT_DLL
#define CTFShotgun C_TFShotgun
#define CTFShotgun_Soldier C_TFShotgun_Soldier
#define CTFShotgun_HWG C_TFShotgun_HWG
#define CTFShotgun_Pyro C_TFShotgun_Pyro
#define CTFScatterGun C_TFScatterGun
#define CTFSuperShotgun C_TFSuperShotgun
#define CTFEternalShotgun C_TFEternalShotgun
#define CTFShotgun_Merc C_TFShotgun_Merc
#define CTFCShotgunSB C_TFCShotgunSB
#define CTFCShotgunDB C_TFCShotgunDB
#endif

// Reload Modes
enum
{
	TF_WEAPON_SHOTGUN_RELOAD_START = 0,
	TF_WEAPON_SHOTGUN_RELOAD_SHELL,
	TF_WEAPON_SHOTGUN_RELOAD_CONTINUE,
	TF_WEAPON_SHOTGUN_RELOAD_FINISH
};

//=============================================================================
//
// Shotgun class.
//
class CTFShotgun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFShotgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CTFShotgun();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SHOTGUN; }
	virtual void	PrimaryAttack();

	virtual acttable_t *ActivityList( int &iActivityCount );
	static acttable_t m_acttableShotgun[];

protected:

	void		Fire( CTFPlayer *pPlayer );
	void		UpdatePunchAngles( CTFPlayer *pPlayer );

private:

	CTFShotgun( const CTFShotgun & ) {}
};

class CTFSuperShotgun : public CTFShotgun
{
public:
	DECLARE_CLASS(CTFSuperShotgun, CTFShotgun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFSuperShotgun();

	virtual int		GetWeaponID(void) const { return TF_WEAPON_SUPERSHOTGUN; }

	virtual acttable_t *ActivityList(int &iActivityCount);
	static acttable_t m_acttableSuperShotgun[];
};

// Scout version. Different models, possibly different behaviour later on
class CTFScatterGun : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFScatterGun, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_SCATTERGUN; }
};

class CTFCShotgunSB : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFCShotgunSB, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_SHOTGUN_SB; }
};

class CTFCShotgunDB : public CTFShotgun
{
public:
	DECLARE_CLASS( CTFCShotgunDB, CTFShotgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_SHOTGUN_DB; }
};

//***************************************************************************
//
// ETERNAL GAMEMODE STUFF
//
//***************************************************************************

class CTFEternalShotgun : public CTFSuperShotgun
{
public:
	DECLARE_CLASS(CTFEternalShotgun, CTFSuperShotgun);
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CTFEternalShotgun();

	int				GetWeaponID(void) const { return TF_WEAPON_ETERNALSHOTGUN; }

	void			Precache(void);

	void			PrimaryAttack();
	void			SecondaryAttack();
	void			ItemPostFrame();
	void			RemoveHook(void);

	bool			CanHolster(void) const;
	void            Drop(const Vector &vecVelocity);

	bool			CanSoftZoom(void) { return false; }

#ifdef GAME_DLL
	void			NotifyHookAttached(CTFPlayer *hooked = NULL);
	void   			DrawBeam(const Vector &endPos, const float width = 2.f);
#endif

private:

	void InitiateHook(CTFPlayer * pPlayer, CBaseEntity *hook);

#ifdef GAME_DLL
	CHandle<CBeam>				pBeam;
	CNetworkHandle(CTFPlayer,	m_hHooked);		//server hook
	CNetworkHandle(CBaseEntity, m_hHook);		//server hooked player
#else
	EHANDLE			m_hHook;					//client hook relay
	EHANDLE			m_hHooked;					//client hooked player relay
#endif

	CNetworkVar(bool, m_bCanRefire);
	CNetworkVar(int, m_iAttached);
};

#ifdef GAME_DLL
class MeatHook : public CGrappleHook
{
	DECLARE_CLASS(MeatHook, CGrappleHook);

public:

	MeatHook(void) {}
	~MeatHook(void);
	void Spawn(void);
	void Precache(void);
	static MeatHook *HookCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner = NULL);
	CTFSuperShotgun *GetOwner(void) { return m_hOwner; }
	bool HookLOS();

protected:

	DECLARE_DATADESC();

private:

	void HookTouch(CBaseEntity *pOther);
	void FlyThink(void);
	
	CTFEternalShotgun		*m_hOwner;
};
#endif

#endif // TF_WEAPON_SHOTGUN_H
