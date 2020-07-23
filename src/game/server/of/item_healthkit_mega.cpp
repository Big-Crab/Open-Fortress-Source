#include "cbase.h"
#include "tf_gamerules.h"
#include "item_healthkit_mega.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sk_item_healthkit_mega("sk_item_healthkit_mega", "150");

#define TF_HEALTHKIT_PICKUP_SOUND	"HealthKitMega.Touch"

LINK_ENTITY_TO_CLASS(item_healthkit_mega, CHealthKitMega);
PRECACHE_REGISTER(item_healthkit_mega);

BEGIN_DATADESC(CHealthKitMega)
END_DATADESC()

bool CHealthKitMega::ITEM_GiveTFMegaHealth(CBasePlayer *pPlayer)
{
	CTFPlayer *pTFPlayer = ToTFPlayer(pPlayer);
	if (!pTFPlayer)
		return false;

	//in duels mega health gives 150 health, it does not give the maximum possible amount
	if (TFGameRules()->IsDuelGamemode())
	{
		int iHealthBefore = pTFPlayer->GetHealth();
		int iHealthToAdd = clamp(sk_item_healthkit_mega.GetFloat(), 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - iHealthBefore);
		pPlayer->TakeHealth(iHealthToAdd, DMG_IGNORE_MAXHEALTH);

		if (iHealthBefore < pTFPlayer->m_Shared.GetDefaultHealth())
			iHealthToAdd = max(0, pTFPlayer->GetHealth() - pTFPlayer->m_Shared.GetDefaultHealth());

		pTFPlayer->m_Shared.m_flMegaOverheal += iHealthToAdd;
	}
	else
	{
		int iHealthToAdd = pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth();
		iHealthToAdd = clamp(iHealthToAdd, 0, pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->GetHealth());
		pPlayer->TakeHealth(iHealthToAdd, DMG_IGNORE_MAXHEALTH);
		pTFPlayer->m_Shared.m_flMegaOverheal = pTFPlayer->m_Shared.GetMaxBuffedHealthDM() - pTFPlayer->m_Shared.GetDefaultHealth();
	}

	return true;
}

void CHealthKitMega::Precache(void)
{
	BaseClass::Precache();

	PrecacheScriptSound(TF_HEALTHKIT_PICKUP_SOUND);
}

bool CHealthKitMega::DoPowerupEffect( CTFPlayer *pTFPlayer )
{
	if (ITEM_GiveTFMegaHealth(pTFPlayer))
	{
		CSingleUserRecipientFilter filter(pTFPlayer);
		EmitSound(filter, entindex(), STRING(m_iszPickupSound));
		return true;
	}
	else
	{
		return false;
	}
}