//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
//
//=============================================================================
#ifndef TF_WEAPON_NAILGUN_H
#define TF_WEAPON_NAILGUN_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"

// Client specific.
#ifdef CLIENT_DLL
#define CTFNailgun C_TFNailgun
#define CTFCNailgun C_TFCNailgun
#define CTFCNailgunSuper C_TFCNailgunSuper
#endif

//=============================================================================
//
// TF Weapon Sub-machine gun.
//
class CTFNailgun : public CTFWeaponBaseGun
{
public:

	DECLARE_CLASS( CTFNailgun, CTFWeaponBaseGun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Server specific.
#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CTFNailgun() {}
	~CTFNailgun() {}

	virtual int		GetWeaponID( void ) const			{ return TF_WEAPON_NAILGUN; }

private:

	CTFNailgun( const CTFNailgun & ) {}
};

class CTFCNailgun : public CTFNailgun
{
public:

	DECLARE_CLASS( CTFCNailgun, CTFNailgun );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_NAILGUN; }

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
};

class CTFCNailgunSuper : public CTFNailgun
{
public:

	DECLARE_CLASS( CTFCNailgunSuper, CTFNailgun );
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	virtual int		GetWeaponID( void ) const			{ return TFC_WEAPON_NAILGUN_SUPER; }

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif
};

#endif // TF_WEAPON_NAILGUN_H