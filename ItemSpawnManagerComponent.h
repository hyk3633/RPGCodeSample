
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enums/ItemType.h"
#include "Structs/ItemOptionTableRow.h"
#include "Structs/ItemInfo.h"
#include "Engine/DataTable.h"
#include "ItemSpawnManagerComponent.generated.h"

class ARPGItem;
class UDataTable;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_API UItemSpawnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	

	UItemSpawnManagerComponent();

protected:

	virtual void BeginPlay() override;

public:

	void InitItemSpawnManager();

	void DropItem(const FItemInfo& Info, const FVector& Location);

	void SpawnItems(const FVector& Location);

	void CreatePool(const int32 Size);

private:

	FVector& GetRandomVector(FVector Vector);

	void ItemInitializeBeforeSpawn(const EItemType ItemType, const FVector& Location);

	ARPGItem* GetPooledItem();

	UFUNCTION()
	void AddDeactivatedNum();

	void ArmourStatRandomInitialize(FItemInfo& Info);

	void RandomBitOn(int32& Bit, const int8 Range);

	void AccessoriesStatRandomInitialize(FItemInfo& Info);

private:

	UPROPERTY(EditAnywhere, Category = "Item Spawner")
	TSubclassOf<ARPGItem> ItemClass;

	UPROPERTY(EditAnywhere, Category = "Item Spawner")
	UDataTable* ItemDataTable;

	UPROPERTY()
	TArray<ARPGItem*> ItemPool;

	int32 PoolSize;

	int32 ActivatedNum;

	int32 DeactivatedNum;
};
