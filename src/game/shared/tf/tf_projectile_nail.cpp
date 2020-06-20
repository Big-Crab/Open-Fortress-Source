//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// TF Nail
//
//=============================================================================
#include "cbase.h"
#include "tf_projectile_nail.h"
#include "tf_weaponbase.h"

#ifdef CLIENT_DLL
#include "c_basetempentity.h"
#include "c_te_legacytempents.h"
#include "c_te_effect_dispatch.h"
#include "input.h"
#include "c_tf_player.h"
#include "cliententitylist.h"
#else
#include "tf_shareddefs.h"
#include "baseanimating.h"
#endif

//=============================================================================
//
// TF Syringe Projectile functions (Server specific).
//
#define SYRINGE_MODEL				"models/weapons/w_models/w_syringe_proj.mdl"
#define SYRINGE_DISPATCH_EFFECT		"ClientProjectile_Syringe"
#define SYRINGE_GRAVITY	0.3f

LINK_ENTITY_TO_CLASS( tf_projectile_syringe, CTFProjectile_Syringe );
PRECACHE_REGISTER( tf_projectile_syringe );

short g_sModelIndexSyringe;
void PrecacheSyringe(void *pUser)
{
	g_sModelIndexSyringe = modelinfo->GetModelIndex( SYRINGE_MODEL );
}

PRECACHE_REGISTER_FN(PrecacheSyringe);

CTFProjectile_Syringe::CTFProjectile_Syringe()
{
}

CTFProjectile_Syringe::~CTFProjectile_Syringe()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Syringe *CTFProjectile_Syringe::Create( const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, int bCritical )
{
	return static_cast<CTFProjectile_Syringe*>( CTFBaseProjectile::Create( "tf_projectile_syringe", vecOrigin, vecAngles, pOwner, CTFProjectile_Syringe::GetInitialVelocity(), g_sModelIndexSyringe, SYRINGE_DISPATCH_EFFECT, pScorer, bCritical ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_Syringe::GetProjectileModelName( void )
{
	return SYRINGE_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_Syringe::GetGravity( void )
{
	return SYRINGE_GRAVITY;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *GetSyringeTrailParticleName( int iTeamNumber, bool bCritical )
{
	if ( iTeamNumber == TF_TEAM_BLUE )
	{
		return ( bCritical ? "nailtrails_medic_blue_crit" : "nailtrails_medic_blue" );
	}
	else if ( iTeamNumber == TF_TEAM_RED )
	{
		return ( bCritical ? "nailtrails_medic_red_crit" : "nailtrails_medic_red" );
	}
	else 
	{
		return ( bCritical ? "nailtrails_medic_dm_crit" : "nailtrails_medic_dm" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileSyringeCallback( const CEffectData &data )
{
	// Get the syringe and add it to the client entity list, so we can attach a particle system to it.
	C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer*>( ClientEntityList().GetBaseEntityFromHandle( data.m_hEntity ) );
	if ( pPlayer )
	{
		C_LocalTempEntity *pSyringe = ClientsideProjectileCallback( data, SYRINGE_GRAVITY );
		if ( pSyringe )
		{
			if ( pPlayer->GetTeamNumber() == TF_TEAM_RED ) pSyringe->m_nSkin = 0;
			else if  (pPlayer->GetTeamNumber() == TF_TEAM_BLUE ) pSyringe->m_nSkin = 1;
			else pSyringe->m_nSkin = 2;
			bool bCritical = ( ( data.m_nDamageType & DMG_CRITICAL ) != 0 );

			
			pPlayer->m_Shared.UpdateParticleColor( pSyringe->AddParticleEffect( GetSyringeTrailParticleName( pPlayer->GetTeamNumber(), bCritical ) ) );
			pSyringe->AddEffects( EF_NOSHADOW );
			pSyringe->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT( SYRINGE_DISPATCH_EFFECT, ClientsideProjectileSyringeCallback );

#endif

//=============================================================================
//
// TF Nailgun Projectile functions (Server specific).
//
#define NAILGUN_NAIL_MODEL				"models/weapons/w_models/w_nail.mdl"
#define NAILGUN_NAIL_DISPATCH_EFFECT	"ClientProjectile_Nail"
#define NAILGUN_NAIL_GRAVITY	0.3f

#ifdef GAME_DLL
BEGIN_DATADESC( CTFProjectile_Nail )
DEFINE_ENTITYFUNC(NailTouch),
//DEFINE_THINKFUNC(FlyThink),
END_DATADESC()
#endif

LINK_ENTITY_TO_CLASS(tf_projectile_nail, CTFProjectile_Nail);
PRECACHE_REGISTER(tf_projectile_nail);

short g_sModelIndexNail;
void PrecacheNail(void *pUser)
{
	g_sModelIndexNail = modelinfo->GetModelIndex(NAILGUN_NAIL_MODEL);
}

PRECACHE_REGISTER_FN(PrecacheNail);

CTFProjectile_Nail::CTFProjectile_Nail()
{
}

CTFProjectile_Nail::~CTFProjectile_Nail()
{
}

// Server specific functions
#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Nail *CTFProjectile_Nail::Create(CTFWeaponBase *pWeapon, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, int bCritical)
{
	CTFProjectile_Nail *pNail = static_cast<CTFProjectile_Nail*>(CBaseEntity::CreateNoSpawn("tf_projectile_nail", vecOrigin, vecAngles, pOwner));
	// Initialize the owner.
	pNail->SetOwnerEntity(pOwner);
	if (pWeapon)
	{
		pNail->m_hWeaponID = pWeapon->GetWeaponID();
		pNail->SetLauncher(pWeapon);
		if (pWeapon->GetDamageRadius() >= 0)
			pNail->SetDamageRadius(pWeapon->GetDamageRadius());
	}
	
	pNail->Spawn();

	//return static_cast<CTFProjectile_Nail*>(CTFBaseProjectile::Create("tf_projectile_nail", vecOrigin, vecAngles, pOwner, CTFProjectile_Nail::GetInitialVelocity(), g_sModelIndexNail, NAILGUN_NAIL_DISPATCH_EFFECT, pScorer, bCritical));
	return pNail;
}

// Nonstatic spawn function
void CTFProjectile_Nail::Spawn()
{
	// Precache.
	Precache();

//#ifdef GAME_DLL
//#ifndef CLIENT_DLL
	// Setup the touch functions.
	SetTouch(&CTFProjectile_Nail::NailTouch);
//#endif
	/*
	
	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM );
	AddEFlags( EFL_NO_WATER_VELOCITY_CHANGE );
	AddEffects( EF_NOSHADOW );

	SetCollisionGroup( TFCOLLISION_GROUP_ROCKETS );

	*/
}

//-----------------------------------------------------------------------------
// Purpose: Handles nail collision interaction
//-----------------------------------------------------------------------------
void CTFProjectile_Nail::NailTouch(CBaseEntity *pOther)
{
	// Verify a correct "other."
	if (pOther)
	{
		Assert(pOther);
		if (pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS))
			return;
	}
	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if (pTrace->surface.flags & SURF_SKY)
	{
		UTIL_Remove(this);
		return;
	}

	trace_t trace;
	memcpy(&trace, pTrace, sizeof(trace_t));
	Explode(&trace, pOther);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFProjectile_Nail::GetCustomDamageType()
{
	return TF_DMG_CUSTOM_NONE;
}


void CTFProjectile_Nail::Explode(trace_t *pTrace, CBaseEntity *pOther)
{
	// Save this entity as enemy, they will take 100% damage.
	if (!pOther)
		pOther = pTrace->m_pEnt;
	m_hEnemy = pOther;

	// Invisible.
	SetModelName(NULL_STRING);
	AddSolidFlags(FSOLID_NOT_SOLID);
	m_takedamage = DAMAGE_NO;

	// Pull out a bit.
	if (pTrace->fraction != 1.0)
	{
		SetAbsOrigin(pTrace->endpos + (pTrace->plane.normal * 1.0f));
	}
	// SetAbsOrigin( pTrace->endpos - GetAbsVelocity() );

	// Play explosion sound and effect.
	Vector vecOrigin = GetAbsOrigin();
	CPVSFilter filter(vecOrigin);
	int EntIndex = 0;
	if (pOther)
		EntIndex = pOther->entindex();
	
	// Visual/Audio: (Removed for nails, copied from rockets) deleteme
	//TE_TFExplosion(filter, 0.0f, vecOrigin, pTrace->plane.normal, EntIndex, GetWeaponID(), GetOriginalLauncher());
	//CSoundEnt::InsertSound(SOUND_COMBAT, vecOrigin, 1024, 3.0);

	// Damage.
	CBaseEntity *pAttacker = GetOwnerEntity();
	IScorer *pScorerInterface = dynamic_cast<IScorer*>(pAttacker);
	if (pScorerInterface)
	{
		pAttacker = pScorerInterface->GetScorer();
	}

	CTakeDamageInfo info(this, pAttacker, vec3_origin, vecOrigin, GetDamage(), GetDamageType());
	CTFWeaponBase *pTFWeapon = dynamic_cast<CTFWeaponBase*>( GetOriginalLauncher() );
	info.SetWeapon(pTFWeapon);
	info.SetDamageCustom( GetCustomDamageType() );
	float flRadius = GetRadius();

	RadiusDamage(info, vecOrigin, flRadius, CLASS_NONE, NULL);

	// Debug!
	/*if (tf_rocket_show_radius.GetBool())
	{
		DrawRadius(flRadius);
	}*/

	// Don't decal players with scorch.
	if (!pOther->IsPlayer())
	{
		UTIL_DecalTrace(pTrace, "Scorch");
	}

	// Get the Weapon info
	CTFWeaponBase *pWeapon = (CTFWeaponBase *)CreateEntityByName(WeaponIdToAlias(m_hWeaponID));
	if (pWeapon)
	{
		WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot(pWeapon->GetClassname());
		Assert(hWpnInfo != GetInvalidWeaponInfoHandle());
		CTFWeaponInfo *pWeaponInfo = dynamic_cast<CTFWeaponInfo*>(GetFileWeaponInfoFromHandle(hWpnInfo));
		Assert(pWeaponInfo && "Failed to get CTFWeaponInfo in weapon spawn");

		UTIL_Remove(pWeapon);
	}

	// Remove the nail.
	UTIL_Remove(this);
}
#endif

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_Nail::GetProjectileModelName(void)
{
	return NAILGUN_NAIL_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_Nail::GetGravity(void)
{
	return NAILGUN_NAIL_GRAVITY;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
/*void CTFProjectile_Nail::ProjectileTouch(CBaseEntity *pOther)
{

	// Verify a correct "other."
	if (pOther)
	{
		Assert(pOther);
		if (pOther->IsSolidFlagSet(FSOLID_TRIGGER | FSOLID_VOLUME_CONTENTS))
			return;
	}
	// Handle hitting skybox (disappear).
	const trace_t *pTrace = &CBaseEntity::GetTouchTrace();
	if (pTrace->surface.flags & SURF_SKY)
	{
		UTIL_Remove(this);
		return;
	}

	trace_t trace;
	memcpy(&trace, pTrace, sizeof(trace_t));
	Explode(&trace, pOther);
}*/

/*unsigned int CTFProjectile_Nail::PhysicsSolidMaskForEntity(void) const
{
	// New: skips allies
	// Only collide with the other team
	//int teamContents = (GetTeamNumber() == TF_TEAM_RED) ? CONTENTS_BLUETEAM : CONTENTS_REDTEAM;
	//return BaseClass::PhysicsSolidMaskForEntity() | teamContents | CONTENTS_HITBOX;
	return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX;
}*/


#ifdef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *GetNailTrailParticleName(int iTeamNumber, bool bCritical)
{
	if (iTeamNumber == TF_TEAM_BLUE)
	{
		return (bCritical ? "nailtrails_super_blue_crit" : "nailtrails_super_blue");
	}
	else if (iTeamNumber == TF_TEAM_RED)
	{
		return (bCritical ? "nailtrails_super_red_crit" : "nailtrails_super_red");
	}
	else 
	{
		return (bCritical ? "nailtrails_super_dm_crit" : "nailtrails_super_dm");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileNailCallback(const CEffectData &data)
{
	C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer*>(ClientEntityList().GetBaseEntityFromHandle(data.m_hEntity));
	if (pPlayer)
	{
		C_LocalTempEntity *pNail = ClientsideProjectileCallback(data, NAILGUN_NAIL_GRAVITY);
		if (pNail)
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TF_TEAM_RED:
				pNail->m_nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				pNail->m_nSkin = 1;
				break;
			case TF_TEAM_MERCENARY:
				pNail->m_nSkin = 2;
				break;
			}
			bool bCritical = ((data.m_nDamageType & DMG_CRITICAL) != 0);
			pPlayer->m_Shared.UpdateParticleColor( pNail->AddParticleEffect(GetNailTrailParticleName(pPlayer->GetTeamNumber(), bCritical)) );
			pNail->AddEffects(EF_NOSHADOW);
			pNail->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT(NAILGUN_NAIL_DISPATCH_EFFECT, ClientsideProjectileNailCallback);

#endif

//=============================================================================
//
// TF Tranq Projectile functions (Server specific).
//

#define TRANQ_MODEL				"models/weapons/w_models/w_classic_tranq_proj.mdl"
#define TRANQ_DISPATCH_EFFECT	"ClientProjectile_Tranq"
#define TRANQ_GRAVITY	0.01f

LINK_ENTITY_TO_CLASS(tf_projectile_tranq, CTFProjectile_Tranq);
PRECACHE_REGISTER(tf_projectile_tranq);

short g_sModelIndexTranq;
void PrecacheTranq(void *pUser)
{
	g_sModelIndexTranq = modelinfo->GetModelIndex(TRANQ_MODEL);
}

PRECACHE_REGISTER_FN(PrecacheTranq);


CTFProjectile_Tranq::CTFProjectile_Tranq()
{
}

CTFProjectile_Tranq::~CTFProjectile_Tranq()
{
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFProjectile_Tranq *CTFProjectile_Tranq::Create(const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner, CBaseEntity *pScorer, int bCritical)
{
	return static_cast<CTFProjectile_Tranq*>(CTFBaseProjectile::Create("tf_projectile_tranq", vecOrigin, vecAngles, pOwner, CTFProjectile_Tranq::GetInitialVelocity(), g_sModelIndexTranq, TRANQ_DISPATCH_EFFECT, pScorer, bCritical));
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CTFProjectile_Tranq::GetProjectileModelName(void)
{
	return TRANQ_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
float CTFProjectile_Tranq::GetGravity(void)
{
	return TRANQ_GRAVITY;
}

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *GetTranqTrailParticleName(int iTeamNumber, bool bCritical)
{
	if (iTeamNumber == TF_TEAM_BLUE)
	{
		return (bCritical ? "nailtrails_medic_blue_crit" : "nailtrails_medic_blue");
	}
	else if (iTeamNumber == TF_TEAM_RED)
	{
		return (bCritical ? "nailtrails_medic_red_crit" : "nailtrails_medic_red");
	}
	else 
	{
		return (bCritical ? "tranq_tracer_teamcolor_dm_crit" : "tranq_tracer_teamcolor_dm");
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientsideProjectileTranqCallback(const CEffectData &data)
{
	C_TFPlayer *pPlayer = dynamic_cast<C_TFPlayer*>(ClientEntityList().GetBaseEntityFromHandle(data.m_hEntity));
	if (pPlayer)
	{
		C_LocalTempEntity *pNail = ClientsideProjectileCallback(data, TRANQ_GRAVITY);
		if (pNail)
		{
			switch (pPlayer->GetTeamNumber())
			{
			case TF_TEAM_RED:
				pNail->m_nSkin = 0;
				break;
			case TF_TEAM_BLUE:
				pNail->m_nSkin = 1;
				break;
			case TF_TEAM_MERCENARY:
				pNail->m_nSkin = 2;
				break;
			}
			bool bCritical = ((data.m_nDamageType & DMG_CRITICAL) != 0);
			pPlayer->m_Shared.UpdateParticleColor( pNail->AddParticleEffect(GetTranqTrailParticleName(pPlayer->GetTeamNumber(), bCritical)) );
			pNail->AddEffects(EF_NOSHADOW);
			pNail->flags |= FTENT_USEFASTCOLLISIONS;
		}
	}
}

DECLARE_CLIENT_EFFECT(TRANQ_DISPATCH_EFFECT, ClientsideProjectileTranqCallback);

#endif