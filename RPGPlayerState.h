
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Enums/ItemType.h"
#include "Structs/StatInfo.h"
#include "Item/RPGItem.h"
#include "RPGPlayerState.generated.h"

/**
 * 
 */

UCLASS()
class RPG_API ARPGPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	void UseItem(const int32& UniqueNum);

	const bool EquipItem(const int32& UniqueNum);

	void UnequipItem(const int32& UniqueNum);

protected:

	void UpdateEquippedItemStats(const FItemInfo* Info, const bool bIsEquip);

public:

	void DiscardItem(const int32& UniqueNum);

	bool GetItemInfo(const int32& UniqueNum, FItemInfo& ItemInfo);

	FORCEINLINE int32 GetLastItemArrayNumber() { return CurrentItemUniqueNum; }
	FORCEINLINE int32 GetCoins() const { return Coins; }
	int32 GetItemCount(const int32& UniqueNum) const;
	FORCEINLINE const FStatInfo& GetCurrentCharacterStats() { return CharacterStats; }
	FORCEINLINE const FStatInfo& GetEquippedItemStats() { return EquippedItemStats; }
	const bool GetItemStatInfo(const int32 UniqueNum, FItemInfo& Info);
	const bool IsEquippedItem(const int32& UniqueNum) { return (UniqueNum == EquippedArmourUniqueNum || UniqueNum == EquippedAccessoriesUniqueNum); }

public:

	const int32 AddItem(ARPGItem* PickedItem);

private:

	int32 Coins = 0;

	int32 HealthPotionCount = 0;

	int32 ManaPotionCount = 0;

	TMap<int32, FItemInfo> ItemMap;

	int32 CurrentItemUniqueNum = 2;

	int32 EquippedArmourUniqueNum = -1;

	int32 EquippedAccessoriesUniqueNum = -1;

	FStatInfo CharacterStats;

	FStatInfo EquippedItemStats;
};
