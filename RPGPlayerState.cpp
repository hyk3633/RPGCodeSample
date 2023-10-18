

#include "Player/RPGPlayerState.h"
#include "../RPG.h"

void ARPGPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 0번은 체력 포션, 1번은 마나 포션
	CharacterStats = FStatInfo();
	CharacterStats.DefenseivePower = 1.f;
	CharacterStats.StrikingPower = 1.f;
	CharacterStats.SkillPower = 1.f;
	CharacterStats.MaxHP = 5000;
	CharacterStats.MaxMP = 300;

	EquippedItemStats = FStatInfo();

	EquippedArmourUniqueNum = -1;
	EquippedAccessoriesUniqueNum = -1;
}

void ARPGPlayerState::BeginPlay()
{
	Super::BeginPlay();

}

void ARPGPlayerState::UseItem(const int32& UniqueNum)
{
	if (UniqueNum)
	{
		ManaPotionCount -= 1;
	}
	else
	{
		HealthPotionCount -= 1;
	}
}

const bool ARPGPlayerState::EquipItem(const int32& UniqueNum)
{
	if (ItemMap.Contains(UniqueNum) == false || 
		(EquippedArmourUniqueNum == UniqueNum || EquippedAccessoriesUniqueNum == UniqueNum)) return false;

	const FItemInfo* EquippedItemInfo = ItemMap.Find(UniqueNum);
	if (EquippedItemInfo->ItemType == EItemType::EIT_Armour)
	{
		EquippedArmourUniqueNum = UniqueNum;
	}
	else
	{
		EquippedAccessoriesUniqueNum = UniqueNum;
	}

	UpdateEquippedItemStats(EquippedItemInfo, true);

	return true;
}

void ARPGPlayerState::UnequipItem(const int32& UniqueNum)
{
	if (ItemMap.Contains(UniqueNum) == false || 
		(EquippedArmourUniqueNum != UniqueNum && EquippedAccessoriesUniqueNum != UniqueNum)) return;
	
	const FItemInfo* EquippedItemInfo = ItemMap.Find(UniqueNum);

	if (EquippedItemInfo->ItemType == EItemType::EIT_Armour)
	{
		EquippedArmourUniqueNum = -1;
	}
	else
	{
		EquippedAccessoriesUniqueNum = -1;
	}

	UpdateEquippedItemStats(EquippedItemInfo, false);
}

void ARPGPlayerState::UpdateEquippedItemStats(const FItemInfo* Info, const bool bIsEquip)
{
	if (Info->ItemType == EItemType::EIT_Armour)
	{
		EquippedItemStats.DefenseivePower	= bIsEquip ? Info->ItemStatArr[0] : 0;
		EquippedItemStats.Dexterity			= bIsEquip ? Info->ItemStatArr[1] : 0;
		EquippedItemStats.MaxHP				= bIsEquip ? Info->ItemStatArr[2] : 0;
		EquippedItemStats.MaxMP				= bIsEquip ? Info->ItemStatArr[3] : 0;
	}
	else if (Info->ItemType == EItemType::EIT_Accessories)
	{
		EquippedItemStats.StrikingPower		= bIsEquip ? Info->ItemStatArr[0] : 0;
		EquippedItemStats.SkillPower		= bIsEquip ? Info->ItemStatArr[1] : 0;
		EquippedItemStats.AttackSpeed		= bIsEquip ? Info->ItemStatArr[2] : 0;
	}
}

void ARPGPlayerState::DiscardItem(const int32& UniqueNum)
{
	if (UniqueNum == 0)
	{
		HealthPotionCount -= 1;
		if (HealthPotionCount == 0)
		{
			ItemMap.Remove(UniqueNum);
		}
	}
	else if (UniqueNum == 1)
	{
		ManaPotionCount -= 1;
		if (ManaPotionCount == 0)
		{
			ItemMap.Remove(UniqueNum);
		}
	}
	else
	{
		ItemMap.Remove(UniqueNum);
	}
}

bool ARPGPlayerState::GetItemInfo(const int32& UniqueNum, FItemInfo& ItemInfo)
{
	if (ItemMap.Contains(UniqueNum))
	{
		ItemInfo = (*ItemMap.Find(UniqueNum));
		return true;
	}
	return false;
}

int32 ARPGPlayerState::GetItemCount(const int32& UniqueNum) const
{
	if (UniqueNum == 0)
	{
		return HealthPotionCount;
	}
	else if(UniqueNum == 1)
	{
		return ManaPotionCount;
	}
	else
	{
		return ItemMap.Contains(UniqueNum) ? 1 : 0;
	}
}

const bool ARPGPlayerState::GetItemStatInfo(const int32 UniqueNum, FItemInfo& Info)
{
	if (ItemMap.Contains(UniqueNum))
	{
		Info = (*ItemMap.Find(UniqueNum));
		return true;
	}
	else
	{
		return false;
	}
}

const int32 ARPGPlayerState::AddItem(ARPGItem* PickedItem)
{
	EItemType PickedItemType = PickedItem->GetItemInfo().ItemType;
	const int32 Idx = StaticCast<int32>(PickedItemType);

	switch (PickedItemType)
	{
	case EItemType::EIT_Coin:
		Coins += PickedItem->GetItemInfo().ItemStatArr[0];
		return -1;
	case EItemType::EIT_HealthPotion:
		HealthPotionCount += 1;
		if (ItemMap.Contains(0) == false)
		{
			ItemMap.Add(0, PickedItem->GetItemInfo());
		}
		return 0;
	case EItemType::EIT_ManaPotion:
		ManaPotionCount += 1;
		if (ItemMap.Contains(1) == false)
		{
			ItemMap.Add(1, PickedItem->GetItemInfo());
		}
		return 1;
	case EItemType::EIT_Armour:
	case EItemType::EIT_Accessories:
		ItemMap.Add(CurrentItemUniqueNum, PickedItem->GetItemInfo());
		break;
	}
	return CurrentItemUniqueNum++;
}
