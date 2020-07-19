//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the grapple hook weapon.
//			
//			Primary attack: fires a beam that hooks on a surface.
//
//
//=============================================================================//

#include "cbase.h"
#include "in_buttons.h"
#include "tf_weapon_grapple.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
//#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
//#include "../client/enginesprite.h"
#include "../client/view.h"
//#include "../client/iviewrender.h"
//#include "engine/ivmodelinfo.h"
//#include "materialsystem/imaterialvar.h"
#else
#include "tf_player.h"
#include "ammodef.h"
#include "gamestats.h"
#include "soundent.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define HOOK_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"
#define BOLT_MODEL			"models/weapons/c_models/c_grapple_proj/c_grapple_proj.mdl"

#define BOLT_AIR_VELOCITY	3500
#define BOLT_WATER_VELOCITY	1500
//#define MAX_ROPE_LENGTH		900.f
#define HOOK_PULL			720.f
//#define BOLT_AIR_VELOCITY	1000
//#define BOLT_WATER_VELOCITY 1000
#define MAX_ROPE_LENGTH		3000.0f
//#define HOOK_PULL			300.0f

//#define REEL_RATE 250.0f
//#define MIN_ROPE_LENGTH		15.0f

extern ConVar of_hook_pendulum;

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGrapple, DT_WeaponGrapple)

BEGIN_NETWORK_TABLE(CWeaponGrapple, DT_WeaponGrapple)
#ifdef CLIENT_DLL
RecvPropInt(RECVINFO(m_iAttached)),
RecvPropInt(RECVINFO(m_nBulletType)),
RecvPropEHandle(RECVINFO(m_hHook)),
#else
SendPropInt(SENDINFO(m_iAttached)),
SendPropInt(SENDINFO(m_nBulletType)),
SendPropEHandle(SENDINFO(m_hHook)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGrapple)
DEFINE_PRED_FIELD(m_iAttached, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( tf_weapon_grapple, CWeaponGrapple );


//**************************************************************************
//GRAPPLING WEAPON

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponGrapple::CWeaponGrapple(void)
{
	m_flNextPrimaryAttack = 0.f;
	m_bReloadsSingly = true;
	m_bFiresUnderwater = true;
	m_iAttached = 0;
	m_nBulletType = -1;

#ifdef GAME_DLL
	m_hHook = NULL;
	pBeam = NULL;
	m_bRopeExists = false;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CWeaponGrapple::Precache(void)
{
#ifdef GAME_DLL
	UTIL_PrecacheOther("grapple_hook");
#endif

	PrecacheModel("cable/cable_red.vmt");
	PrecacheModel("cable/cable_blue.vmt");
	PrecacheModel("cable/cable_purple.vmt");

	BaseClass::Precache();
}

void CWeaponGrapple::PrimaryAttack(void)
{
	// Can't have an active hook out
	if (m_hHook)
		return;

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());
	if (!pPlayer)
		return;

#ifdef GAME_DLL
	gamestats->Event_WeaponFired(pPlayer, true, GetClassname());

	bool bCenter = m_pWeaponInfo->GetWeaponData(m_iWeaponMode).m_bCenterfireProjectile;
	int iQuakeCvar = 0;

	if (!pPlayer->IsFakeClient())
		iQuakeCvar = V_atoi(engine->GetClientConVarValue(pPlayer->entindex(), "viewmodel_centered"));

	//Obligatory for MP so the sound can be played
	CDisablePredictionFiltering disabler;
	WeaponSound(SINGLE);

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GRAPPLE_FIRE_START);

	pPlayer->SetMuzzleFlashTime(gpGlobals->curtime + 0.5);

	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), 600, 0.2, GetOwner());

	Vector vecSrc;
	Vector vecOffset(30.f, 4.f, -6.0f);
	if (bCenter || iQuakeCvar)
	{
		vecOffset.x = 12.0f; //forward backwards
		vecOffset.y = 0.0f; // left right
		vecOffset.z = -8.0f; //up down
	}
	QAngle angle;
	GetProjectileFireSetup(pPlayer, vecOffset, &vecSrc, &angle, false);

	//fire direction
	Vector vecDir;
	AngleVectors(angle, &vecDir, &m_vCableRight, &m_vCableUp);
	VectorNormalize(vecDir);

	//Gets the position where the hook will hit
	Vector vecEnd = vecSrc + (vecDir * MAX_TRACE_LENGTH);

	//Traces a line between the two vectors
	trace_t tr;
	UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pPlayer, COLLISION_GROUP_NONE, &tr);

	//A hook that is not fired out of your face, what a mindblowing concept!
	CGrappleHook *pHook = CGrappleHook::HookCreate(vecSrc, angle, this);

	//Set hook velocity and angle
	float vel = pPlayer->GetWaterLevel() == 3 ? BOLT_WATER_VELOCITY : BOLT_AIR_VELOCITY;
	Vector HookVelocity = vecDir * vel;
	pHook->SetAbsVelocity(HookVelocity);
	VectorAngles(HookVelocity, angle); //reuse already allocated QAngle
	SetAbsAngles(angle);

	m_hHook = pHook;

	m_bRopeExists = true;
	m_flCableFuncStartTime = gpGlobals->curtime;

	//Initialize the beam
	//	DrawBeam(m_hHook->GetAbsOrigin());
#endif

	//m_flNextPrimaryAttack = gpGlobals->curtime - 1.f;
	m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	SetWeaponIdleTime(gpGlobals->curtime + SequenceDuration(ACT_VM_PRIMARYATTACK));
}

#ifndef GAME_DLL
static IMaterial *g_pBeamWireframeMaterial;
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::ItemPostFrame(void)
{
	if (!CanAttack())
		return;

	CTFPlayer *pPlayer = ToTFPlayer(GetOwner());

	CBaseEntity *Hook = GetHookEntity();
#ifdef GAME_DLL
	// removed this as it prevents most swinging movement with the hook
//	if (Hook && m_iAttached && !HookLOS())
//		RemoveHook();

	//Update the beam depending on the hook position
	if (pBeam && !m_iAttached)
	{
		//Set where it ends
		pBeam->PointEntInit(m_hHook->GetAbsOrigin(), this);
		pBeam->SetEndAttachment(LookupAttachment("muzzle"));
	}

	// update new cable effect
	if (m_bRopeExists && Hook)
	{
		// hook pos
		Vector startPoint = m_hHook->GetAbsOrigin();
		// muzzle pos
		Vector endPoint = Vector(0, 0, 0);
		QAngle muzzleAngle;
		GetAttachment("muzzle", endPoint, muzzleAngle);
		Vector muzzleForward;
		AngleVectors(muzzleAngle, &muzzleForward);

		// start -> end
		Vector cableBaseDir = endPoint - startPoint;
		// normalized, store length in absCableLength
		float absCableLength = VectorNormalize(cableBaseDir);
		float cableStep = absCableLength / NUM_CABLE_VERTS;
		float cableStepCurrent = cableStep;

		// Remap cableStepCurrent/absCableLength to this
		const float xEnd = 10.0f;
		const float S = 2.0f;
		const float dispScale = 550.0f; // 150
		const float dispScaleTwang = 500.0f;
		//const float timeScale = 1.5f;
		const float timeScale = 2.5f;
		const float timeScaleTwang = 20.0f;
		const float timeScaleTwangScale = 0.4f;
		const float maxTwang = 0.4f;
		const float maxTime = 10.0f;

		float t = min((gpGlobals->curtime - m_flCableFuncStartTime) * timeScale, maxTime);
		float t2 = (gpGlobals->curtime - m_flCableFuncStartTime) * timeScaleTwang;
		float s2 = maxTwang + ((gpGlobals->curtime - m_flCableFuncStartTime) * timeScaleTwangScale);
		//float s2 = 0.4f;

		//DevMsg("t was %f\n", t);

		// 0 is always startPoint
		vertices[0] = startPoint;
		vertices[NUM_CABLE_VERTS - 1] = endPoint;
		// Iterate over the vertices and position them along the cable
		for (int i = 1; i < NUM_CABLE_VERTS - 1; i++)
		{
			// If attached, twang like a taught wire
			if (m_iAttached)
			{
				// 0...1
				float x = cableStepCurrent / absCableLength;
				float displacementTwang = sinf(M_PI_F * x) * sinf(sinf(t2)) * (pow(1.0f - min(s2, 1.0f), 3));

				vertices[i] = startPoint + (cableBaseDir * cableStepCurrent) + (m_vCableUp * displacementTwang * dispScaleTwang);
			}
			// If flying, buckle the cable by spiralling outward
			else
			{
				// 0 to xEnd based on distance down the cable we are
				//float x = (cableStepCurrent / absCableLength) * xEnd;
				float x = cableStepCurrent / (xEnd * 15.0f);

				float displacement;
				// if x < S
				displacement = (sinf(t * x) / (t * x)) * ((t * t) / 25.0f) * min(x, 1.0f);
				if (x >= S)
				{
					displacement /= (x - S + 1);
				}

				float spiralProgress = (x * M_PI_F) * t;

				displacement *= dispScale;

				vertices[i] = startPoint + (cableBaseDir * cableStepCurrent) + (sinf(spiralProgress) * m_vCableRight * displacement) + (cosf(spiralProgress) * m_vCableUp * displacement);
			}
			cableStepCurrent += cableStep;

			const Vector offset = m_vCableUp * 2.0f;
			const Vector offsetH = m_vCableRight * 2.0f;
			int col = ceil(((float)i / (float)NUM_CABLE_VERTS) * 255.0);
			DebugDrawLine(vertices[i], vertices[i - 1], col, col, col, false, 0.05f);
			DebugDrawLine(vertices[i] + offset, vertices[i - 1] + offset, col, col, col, false, 0.05f);
			DebugDrawLine(vertices[i] - offset, vertices[i - 1] + offset, col, col, col, false, 0.05f);
			DebugDrawLine(vertices[i] + offsetH, vertices[i - 1] + offsetH, col, col, col, false, 0.05f);
			DebugDrawLine(vertices[i] - offsetH, vertices[i - 1] + offsetH, col, col, col, false, 0.05f);
		}
	}
//#else
	//Hook = m_hHook.Get();
#endif

	if (!pPlayer || !pPlayer->IsAlive())
	{
		if (Hook)
			RemoveHook();
		return;
	}

	if (pPlayer->m_nButtons & IN_ATTACK)
	{
		if (m_flNextPrimaryAttack < gpGlobals->curtime)
			PrimaryAttack();

		if (Hook && m_iAttached) //hook is attached to a surface
		{
			pPlayer->SetGroundEntity(NULL);

			if ((Hook->GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length() <= 15.0f/* ||					//player is very close to the attached hook			// was 100.0f
																				  (m_iAttached == 1 && pPlayer->GetGroundEntity() && of_hook_pendulum.GetBool())*/)			//player touched the ground while swinging
			{
				RemoveHook();
			}
			else if (m_iAttached == 2) //notify player how it should behave
			{
				InitiateHook(pPlayer, Hook);
			}
		}
	}
	else if (m_iAttached)
	{
		RemoveHook();
	}
}

void CWeaponGrapple::InitiateHook(CTFPlayer *pPlayer, CBaseEntity *hook)
{
	pPlayer->m_Shared.SetHook(hook);

	//player velocity
	Vector pVel = pPlayer->GetAbsVelocity();

	//rope vector
	Vector playerCenter = pPlayer->WorldSpaceCenter() - (pPlayer->WorldSpaceCenter() - pPlayer->GetAbsOrigin()) * .25;
	playerCenter += (pPlayer->EyePosition() - playerCenter) * 0.5;
	Vector rope = hook->GetAbsOrigin() - pPlayer->GetAbsOrigin();

	if (!of_hook_pendulum.GetBool())
	{
		pPlayer->SetGroundEntity(NULL);

		VectorNormalize(rope);
		rope = rope * HOOK_PULL;

		//Resulting velocity
		Vector newVel = pVel + rope;
		float velLength = max(pVel.Length() + 200.f, HOOK_PULL);
		float newVelLength = clamp(newVel.Length(), HOOK_PULL, velLength);

		pPlayer->m_Shared.SetHookProperty(newVelLength);
	}
	else
	{
		pPlayer->m_Shared.SetHookProperty(rope.Length());
	}

	m_iAttached = 1;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponGrapple::RemoveHook(void)
{
#ifdef GAME_DLL
	if (m_hHook)
	{
		m_hHook->SetTouch(NULL);
		m_hHook->SetThink(NULL);
		UTIL_Remove(m_hHook);
	}

	if (pBeam)
	{
		UTIL_Remove(pBeam); //Kill beam
		pBeam = NULL;
	}

	m_bRopeExists = false;
#endif

	CTFPlayer *pPlayer = ToTFPlayer(GetPlayerOwner());

	if (pPlayer)
	{
		pPlayer->m_Shared.SetHook(NULL);
		pPlayer->m_Shared.SetHookProperty(0.f);
	}

	m_hHook = NULL;
	SendWeaponAnim(ACT_VM_IDLE); //ACT_VM_RELOAD
	//m_flNextPrimaryAttack = gpGlobals->curtime + 1.f;
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.1f;
	m_iAttached = 0;
}

CBaseEntity *CWeaponGrapple::GetHookEntity(void)
{
#ifdef GAME_DLL
	return m_hHook;
#else
	return m_hHook.Get();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponGrapple::CanHolster(void) const
{
	CBaseEntity *Hook = NULL;
#ifdef GAME_DLL
	Hook = m_hHook;
#else
	Hook = m_hHook.Get();
#endif
	if (Hook)
		return false;

	return BaseClass::CanHolster();
}

bool CWeaponGrapple::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	if (GetHookEntity())
		RemoveHook();

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::Drop(const Vector &vecVelocity)
{
	if (GetHookEntity())
		return;

	BaseClass::Drop( vecVelocity );
}

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponGrapple::NotifyHookAttached(void)
{
	m_iAttached = 2;
}

bool CWeaponGrapple::HookLOS()
{
	CBaseEntity *player = GetOwner();

	if (!player)
		return false;

	Vector playerCenter = player->GetAbsOrigin();
	playerCenter += (player->EyePosition() - playerCenter) * 0.5;

	trace_t tr;
	UTIL_TraceLine(m_hHook->GetAbsOrigin(), playerCenter, MASK_ALL, m_hHook, COLLISION_GROUP_NONE, &tr);

	//Msg("%f %f %f %f\n", m_hHook->GetAbsOrigin().Length(), playerCenter.Length(), tr.endpos.Length(), (tr.endpos - playerCenter).Length());

	return (tr.endpos - playerCenter).Length() < 2.f;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a beam
// Input  : &startPos - where the beam should begin
//          &endPos - where the beam should end
//          width - what the diameter of the beam should be (units?)
//-----------------------------------------------------------------------------
void CWeaponGrapple::DrawBeam(const Vector &endPos, const float width)
{
	//Draw the main beam shaft
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
		return;

	//Pick cable color
	if (pPlayer->GetTeamNumber() == TF_TEAM_RED)
		pBeam = CBeam::BeamCreate("cable/cable_red.vmt", width);
	else if (pPlayer->GetTeamNumber() == TF_TEAM_BLUE)
		pBeam = CBeam::BeamCreate("cable/cable_blue.vmt", width);
	else
		pBeam = CBeam::BeamCreate("cable/cable_purple.vmt", width);

	//Set where it ends
	pBeam->PointEntInit(endPos, this);
	pBeam->SetEndAttachment(LookupAttachment("muzzle"));

	pBeam->SetWidth(width);

	// Higher brightness means less transparent
	pBeam->SetBrightness(255);
	pBeam->RelinkBeam();

	//Sets scrollrate of the beam sprite 
	float scrollOffset = gpGlobals->curtime + 5.5;
	pBeam->SetScrollRate(scrollOffset);

	UpdateWaterState();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &tr - used to figure out where to do the effect
//          nDamageType - ???
//-----------------------------------------------------------------------------
void CWeaponGrapple::DoImpactEffect(trace_t &tr, int nDamageType)
{
	if (!(tr.surface.flags & SURF_SKY))
	{
		CPVSFilter filter(tr.endpos);
		te->GaussExplosion(filter, 0.0f, tr.endpos, tr.plane.normal, 0);
		m_nBulletType = GetAmmoDef()->Index("GaussEnergy");
		UTIL_ImpactTrace(&tr, m_nBulletType);
	}
}

#endif

//**************************************************************************
//HOOK

#ifdef GAME_DLL

LINK_ENTITY_TO_CLASS(grapple_hook, CGrappleHook);

BEGIN_DATADESC(CGrappleHook)
DEFINE_THINKFUNC(FlyThink),
DEFINE_FUNCTION(HookTouch),
DEFINE_FIELD(m_hOwner, FIELD_EHANDLE),
END_DATADESC()

CGrappleHook *CGrappleHook::HookCreate(const Vector &vecOrigin, const QAngle &angAngles, CBaseEntity *pentOwner)
{
	CGrappleHook *pHook = (CGrappleHook *)CreateEntityByName("grapple_hook");
	UTIL_SetOrigin(pHook, vecOrigin);
	pHook->SetAbsAngles(angAngles);
	pHook->Spawn();

	CWeaponGrapple *pOwner = (CWeaponGrapple *)pentOwner;
	pHook->m_hOwner = pOwner;
	pHook->SetOwnerEntity(pOwner->GetOwner());

	return pHook;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrappleHook::~CGrappleHook(void)
{
	CTFPlayer *pOwner = (CTFPlayer *)GetOwnerEntity();
	if (!pOwner)
		return;

	if (pOwner)
		pOwner->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, false);
}

unsigned int CGrappleHook::PhysicsSolidMaskForEntity() const
{
	return (BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX) & ~CONTENTS_GRATE;
}

bool CGrappleHook::CreateVPhysics(void)
{
	// Create the object in the physics system
	VPhysicsInitNormal(SOLID_BBOX, FSOLID_NOT_STANDABLE, false);

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn
//-----------------------------------------------------------------------------
void CGrappleHook::Spawn(void)
{
	Precache();

	SetModel(HOOK_MODEL);
	SetMoveType(MOVETYPE_FLY, MOVECOLLIDE_FLY_CUSTOM);
	UTIL_SetSize(this, -Vector(1, 1, 1), Vector(1, 1, 1));
	SetSolid(SOLID_BBOX);
	SetGravity(0.05f);

	// The rock is invisible, the crossbow bolt is the visual representation
	AddEffects(EF_NODRAW);

	// Make sure we're updated if we're underwater
	UpdateWaterState();

	// Create bolt model and parent it
	CBaseEntity *pBolt = CBaseEntity::CreateNoSpawn("prop_dynamic", GetAbsOrigin(), GetAbsAngles(), this);
	pBolt->SetModelName(MAKE_STRING(BOLT_MODEL));
	pBolt->SetModel(BOLT_MODEL);
	DispatchSpawn(pBolt);
	pBolt->SetParent(this);

	SetTouch(&CGrappleHook::HookTouch);
	SetThink(&CGrappleHook::FlyThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CGrappleHook::Precache(void)
{
	PrecacheModel(HOOK_MODEL);
	PrecacheModel(BOLT_MODEL);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGrappleHook::FlyThink(void)
{
	if (!m_hOwner)
	{
		SetThink(NULL);
		SetTouch(NULL);
		UTIL_Remove(this);
		return;
	}

	if ((GetAbsOrigin() - m_hOwner->GetAbsOrigin()).Length() >= MAX_ROPE_LENGTH)
	{
		m_hOwner->RemoveHook();
		return;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CGrappleHook::HookTouch(CBaseEntity *pOther)
{
	// removed LOS check - invalidates locomotion with hook
	if (!pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) || !GetOwnerEntity() /*|| !m_hOwner->HookLOS() */)
	{
		m_hOwner->RemoveHook();
		return;
	}

	//hooked an entity that can be damaged
	if (pOther != m_hOwner && pOther->m_takedamage != DAMAGE_NO)
	{
		m_hOwner->RemoveHook();
	}
	else
	{
		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if (pOther->GetMoveType() == MOVETYPE_NONE && !(tr.surface.flags & SURF_SKY))
		{
			SetMoveType(MOVETYPE_NONE);
			EmitSound("Weapon_AR2.Reload_Push");

			SetTouch(NULL);
			SetThink(NULL);

			m_hOwner->NotifyHookAttached();

			VPhysicsDestroyObject();
			VPhysicsInitNormal(SOLID_VPHYSICS, FSOLID_NOT_STANDABLE, false);
			AddSolidFlags(FSOLID_NOT_SOLID);

			CTFPlayer *pOwner = (CTFPlayer *)GetOwnerEntity();
			pOwner->SetPhysicsFlag(PFLAG_VPHYSICS_MOTIONCONTROLLER, true);
			pOwner->DoAnimationEvent(PLAYERANIMEVENT_CUSTOM, ACT_GRAPPLE_PULL_START);

			m_hOwner->m_flCableFuncStartTime = gpGlobals->curtime;
		}
		else
		{
			m_hOwner->RemoveHook();
		}
	}
}

#endif