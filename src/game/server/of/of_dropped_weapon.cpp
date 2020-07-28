//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tf_shareddefs.h"
#include "ammodef.h"
#include "tf_gamerules.h"
#include "explode.h"
#include "in_buttons.h"
#include "tf_bot.h"
#include "of_dropped_weapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//----------------------------------------------

extern ConVar of_allow_allclass_pickups;
extern ConVar of_forceclass;

// Network table.
IMPLEMENT_SERVERCLASS_ST( CTFDroppedWeapon, DT_DroppedWeapon )
	SendPropVector( SENDINFO( m_vecInitialVelocity ), -1, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_iReserveAmmo ) ),
	SendPropInt( SENDINFO( m_iClip ) ),
	SendPropBool( SENDINFO( m_bFlamethrower ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CTFDroppedWeapon )
	DEFINE_THINKFUNC( FlyThink ),
	DEFINE_ENTITYFUNC( PackTouch ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( tf_dropped_weapon, CTFDroppedWeapon );
PRECACHE_REGISTER( tf_dropped_weapon );

IMPLEMENT_AUTO_LIST(ICondDroppedWeaponAutoList);

CTFDroppedWeapon::CTFDroppedWeapon()
{
	m_iTeamNum = TEAM_UNASSIGNED;
}

void CTFDroppedWeapon::Precache( void )
{
}

CTFDroppedWeapon *CTFDroppedWeapon::Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, const char *pszModelName, int iWeaponID, const char *pszClassname, bool bThrown)
{
	//Don't drop if it's a crowabr and everybody has it or it is a gamemode where no weapons should be dropped
	if (TFGameRules() && TFGameRules()->IsGGGamemode() ||
		iWeaponID == TF_WEAPON_CROWBAR && of_allow_allclass_pickups.GetBool() && !of_forceclass.GetBool() ||
		iWeaponID == TF_WEAPON_NONE)
		return NULL;

	CTFDroppedWeapon *pDroppedWeapon = static_cast<CTFDroppedWeapon *>( CBaseAnimating::CreateNoSpawn( "tf_dropped_weapon", vecOrigin, vecAngles, pOwner ) );
	if ( pDroppedWeapon )
	{
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszClassname );
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo *>( GetFileWeaponInfoFromHandle( hWpnInfo ) );

		pDroppedWeapon->SetModelName( AllocPooledString( pszModelName ) );
		pDroppedWeapon->WeaponID = iWeaponID;
		pDroppedWeapon->pszWeaponName = pszClassname;
		pDroppedWeapon->pWeaponInfo = pWeaponInfo;
		pDroppedWeapon->m_bFlamethrower = iWeaponID == TF_WEAPON_FLAMETHROWER || iWeaponID == TFC_WEAPON_FLAMETHROWER;
		pDroppedWeapon->m_bThrown = bThrown;

		DispatchSpawn( pDroppedWeapon );
	}

	return pDroppedWeapon;
}

void CTFDroppedWeapon::Spawn(void)
{
	Precache();
	SetModel(STRING(GetModelName()));
	BaseClass::Spawn();

	SetTouch( &CTFDroppedWeapon::PackTouch );
	//if weapon was thrown to hurt an enemy the owner cannot ever pick it up again
	if ( !m_bThrown )
	{
		SetNextThink( gpGlobals->curtime + 0.75f );
		SetThink( &CTFDroppedWeapon::FlyThink );
	}

	m_flCreationTime = gpGlobals->curtime;
	m_flNextPickupTime = 0.f;

	// no pickup until flythink
	m_bAllowOwnerPickup = false;

	float flDespawnTime = pWeaponInfo ? pWeaponInfo->m_flDespawnTime : 30.0f;

	// Die in 30 seconds
	if ( flDespawnTime > 0 )
		SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + flDespawnTime, "DieContext" );
}

void CTFDroppedWeapon::SetInitialVelocity( Vector &vecVelocity )
{ 
	m_vecInitialVelocity = vecVelocity;
}

void CTFDroppedWeapon::FlyThink( void )
{
	m_bAllowOwnerPickup = true;
}

void CTFDroppedWeapon::PackTouch( CBaseEntity *pOther )
{
	//if weapon was thrown and it hurt a player
	//don't let anybody pick it up for a second
	if ( gpGlobals->curtime < m_flNextPickupTime )
		return;

	if (!pOther || !pOther->IsPlayer() || !pOther->IsAlive())
		return;

	CTFPlayer *pTFPlayer = ToTFPlayer(pOther);

	if (!pTFPlayer || pTFPlayer->m_Shared.IsZombie())
		return;

	CBaseEntity *pOwner = GetOwnerEntity();
	bool bIsOtherOwner = pOwner == pTFPlayer;

	//code to hurt enemy players when weapon is thrown
	if (m_bThrown && !bIsOtherOwner)
	{
		Vector vel;
		AngularImpulse angImp;
		VPhysicsGetObject()->GetVelocity(&vel, &angImp);

		//If the weapon still has enough speed
		if (vel.Length() > 900.f)
		{
			//Hurt the player
			CTakeDamageInfo info;
			info.SetAttacker( pOwner );
			info.SetInflictor( this );
			info.SetDamage( 40.f );
			Vector vForce = vel;
			VectorNormalize( vForce );
			info.SetDamageForce( vForce );
			info.SetDamagePosition( GetAbsOrigin() );
			info.SetDamageType( DMG_CLUB | DMG_USEDISTANCEMOD );
			info.SetDamageCustom( TF_DMG_CUSTOM_NONE );
			//SetKillIcon(vForce);

			pTFPlayer->TakeDamage(info);

			//stop the weapon
			vel *= Vector(0.1f, 0.1f, 1.f);
			VPhysicsGetObject()->SetVelocityInstantaneous(&vel, &angImp);

			//prevent the weapon to be picked up right away after impact
			m_flNextPickupTime = gpGlobals->curtime + 1.5f;
		}

		//Stop the hurt
		m_bThrown = false;
		return;
	}

	//Don't let the person who threw this ammo pick it up until it hits the ground.
	//This way we can throw ammo to people, but not touch it as soon as we throw it ourselves
	if ( bIsOtherOwner && !m_bAllowOwnerPickup )
		return;

	if( !pTFPlayer->GetPlayerClass()->IsClass( TF_CLASS_MERCENARY ) && !of_allow_allclass_pickups.GetBool() ) // Dont let non Mercenary classes pick up weapons unless thats turned on
		return;

	bool bSuccess = true;

	// akimbo pickups have NOT pewished
	if ( WeaponID == TF_WEAPON_PISTOL_MERCENARY && pTFPlayer->OwnsWeaponID( TF_WEAPON_PISTOL_MERCENARY ) ) // If the weapon is a pistol and we already own a pistol, give us the akimbos
	{
		WeaponID = TF_WEAPON_PISTOL_AKIMBO;
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( "tf_weapon_pistol_akimbo" );
		pszWeaponName = g_aWeaponNames[TF_WEAPON_PISTOL_AKIMBO];
		pWeaponInfo = dynamic_cast<CTFWeaponInfo *>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	}

	// don't allow multiple pistols to be picked up at the same time
	if ( WeaponID == TF_WEAPON_PISTOL || WeaponID == TF_WEAPON_PISTOL_SCOUT )
	{
		if ( pTFPlayer->OwnsWeaponID( TF_WEAPON_PISTOL ) || pTFPlayer->OwnsWeaponID( TF_WEAPON_PISTOL_SCOUT ) )
			return;
	}
		
	if ( !pWeaponInfo )
		return;

	bool bUsesDMBuckets = TFGameRules() && TFGameRules()->UsesDMBuckets();
	int iSlot, iPos = pWeaponInfo->iPosition;
	if (bUsesDMBuckets)
	{
		iSlot = pWeaponInfo->iSlotDM;
		iPos = pWeaponInfo->iPositionDM;
	}
	else if (pWeaponInfo->m_iClassSlot[pTFPlayer->GetPlayerClass()->GetClassIndex()] != -1)
	{
		iSlot = pWeaponInfo->m_iClassSlot[pTFPlayer->GetPlayerClass()->GetClassIndex()];
	}
	else
	{
		iSlot = pWeaponInfo->iSlot;
	}
	
	if ( !pTFPlayer->m_hWeaponInSlot )
		return;
	
	if ( pTFPlayer->m_hWeaponInSlot[iSlot][iPos] && pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->GetWeaponID() == WeaponID )
		return;
	
	if ( pTFPlayer->m_hWeaponInSlot[iSlot][iPos] && !pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->CanHolster() )
		return;
	
	if ( bUsesDMBuckets )
	{
		if ( pTFPlayer->m_hWeaponInSlot[iSlot][iPos] && !(pTFPlayer->m_nButtons & IN_USE) )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "player_swap_weapons" );
			if ( event )
			{
				event->SetInt( "playerid", pTFPlayer->entindex() );
				event->SetInt( "current_wep", pTFPlayer->m_hWeaponInSlot[iSlot][iPos]->GetWeaponID() );
				event->SetInt( "swap_wep", WeaponID );
				gameeventmanager->FireEvent( event );
			}
			return;
		}
	}
	
	if ( bSuccess )	// If we don't own the weapon and we're not in the 3 weapon system
	{
		CSingleUserRecipientFilter filter( pTFPlayer );		// Filter the sound to the player who picked this up
		EmitSound( filter, entindex(), "AmmoPack.Touch" );	// Play the sound
		CTFWeaponBase *pGivenWeapon =(CTFWeaponBase *)pTFPlayer->GiveNamedItem( pszWeaponName ); 	// Create the weapon
		if( pGivenWeapon )
		{
			pGivenWeapon->GiveTo( pTFPlayer );	// and give it to the player

			if ( pTFPlayer->GetActiveWeapon() &&
				 pGivenWeapon->GetSlot() == pTFPlayer->GetActiveWeapon()->GetSlot() &&
				 pGivenWeapon->GetPosition() == pTFPlayer->GetActiveWeapon()->GetPosition() )
				pTFPlayer->Weapon_Switch( pGivenWeapon );

			if (m_iReserveAmmo > -1)
				pGivenWeapon->m_iReserveAmmo = m_iReserveAmmo;

			if ( m_iClip > -1 )
			{
				pGivenWeapon->m_iClip1 = m_iClip;
				if( WeaponID == TF_WEAPON_PISTOL_AKIMBO )
					pGivenWeapon->m_iClip1 *= 2;
			}

			if ( pTFPlayer->IsFakeClient() )
			{
				CTFBot *actor = ToTFBot( pTFPlayer );
				if ( actor )
				{
					actor->Weapon_Switch( pGivenWeapon );
					actor->m_bPickedUpWeapon = true;
				}
			}

			if ( pGivenWeapon->GetTFWpnData().m_bAlwaysDrop ) // superweapon
			{
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_ULTRARARE );
			}
			else if ( WeaponID_IsRocketWeapon ( pGivenWeapon->GetWeaponID() )  // "rare" weapons, this is kinda terrible
					|| WeaponID_IsGrenadeWeapon ( pGivenWeapon->GetWeaponID() ) 
					|| pGivenWeapon->GetWeaponID() == TF_WEAPON_RAILGUN
					|| pGivenWeapon->GetWeaponID() == TF_WEAPON_LIGHTNING_GUN
					|| pGivenWeapon->GetWeaponID() == TF_WEAPON_GATLINGGUN )
			{
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_RARE );
			}
			else
			{
				pTFPlayer->SpeakConceptIfAllowed( MP_CONCEPT_MVM_LOOT_COMMON ); // common weapons
			}
		}

		UTIL_Remove( this ); // Remove the dropped weapon entity
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
unsigned int CTFDroppedWeapon::PhysicsSolidMaskForEntity( void ) const
{
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_DEBRIS;
}