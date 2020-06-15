#include "weapongiver.h"

void WeaponGiver::Weapon_Create(CTFPlayer *player, const char *pWeaponName) {
	//CBaseCombatCharacter::Weapon_Create
	CBaseCombatWeapon *pWeapon = CBaseCombatCharacter::Weapon_Create(STRING(pWeaponName));
	player->Weapon_Equip(pGivenWeapon);
}
