
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Enums/CharacterType.h"
#include "Enums/ItemType.h"
#include "Structs/StatInfo.h"
#include "Structs/ItemInfo.h"
#include "RPGHUD.generated.h"

/**
 * 
 */

class URPGCharacterSelectionInterface;
class URPGGameplayInterface;
class ARPGBasePlayerCharacter;
class URPGInventoryWidget;
class URPGInventorySlotWidget;
class ARPGItem;
class UDataTable;
class URPGItemSlotMenuWidget;
class URPGStatTextBoxWidget;
class URPGDamageWidget;

UCLASS()
class RPG_API ARPGHUD : public AHUD
{
	GENERATED_BODY()

public:

	ARPGHUD();

	virtual void Tick(float DeltaTime) override;

	virtual void PostInitializeComponents() override;
	
protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnWarriorSelected();

	UFUNCTION()
	void OnSorcererSelected();

	void OnCharacterSelected(ECharacterType Type);

public:

	/** ù ���� �� HUD �ʱ�ȭ */
	void InitHUD();

protected:

	void CastPawnAndBindFunctions();

public:

	/** ĳ���� ������ �� HUD �ٽ� �ε� */
	void ReloadHUD();

protected:

	void OffHUD();

	void DrawOverlay();

	/** ü��, ���� ���α׷��� �� */

	UFUNCTION()
	void SetHealthBarPercentage(float Percentage);

	UFUNCTION()
	void SetManaBarPercentage(float Percentage);

	/** ��ų ��ٿ� */

	void UpdateCooldownProgress();

	void SetProgressPercentage(const int8 Index, const float Percentage);

	UFUNCTION()
	void CooldownProgressSetFull(uint8 Bit);

	/** �κ��丮 */

	void InitInventorySlot();

public: 

	void InventoryWidgetToggle(const bool bInventoryOn);

	void AddCoins(const int32& CoinAmount);

	void AddPotion(const int32& UniqueNum, const EItemType ItemType, const int32& PotionCount);

	void AddEquipment(const int32& UniqueNum, const EItemType ItemType);

	void UpdateItemCount(const int32& UniqueNum, const int32& ItemCount);

protected:

	void ClearItemSlot(const int32& UniqueNum);

	void SetSlotIcon(const int32& UniqueNum, const EItemType ItemType);

	void ExpandInventoryIfNoSpace();

	UFUNCTION()
	void OnItemSlotButtonClickEvent(int32 UniqueNum);

	void GetPositionUnderCursor(FVector2D& Position);

	UFUNCTION()
	void OnUseOrEquipButtonClicked();

	void EquipItem(const int32& UniqueNum);

	void UnequipItem(const int32& UniqueNum);

	UFUNCTION()
	void OnDiscardButtonClicked();

	UFUNCTION()
	void OnItemSlotButtonHoveredEvent(int32 UniqueNum);

public:

	void ShowItemStatTextBox(const FItemInfo& Info);

protected:

	void HideItemStatTextBox();

public:

	void ToggleStatInfoWidget();

	void UpdateStatCharacterStatText(const FStatInfo& Stats);

	void UpdateStatEquippedItemStatText(const FStatInfo& Stats);

	void PopUpDamageWidget(const FVector_NetQuantize& PopupPosition, const int32 Damage);

private:

	UPROPERTY()
	TSubclassOf<URPGCharacterSelectionInterface> CharacterSelectWBPClass;

	URPGCharacterSelectionInterface* CharacterSelectionInterface;

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<UUserWidget> GameplayInterfaceClass;

	UPROPERTY()
	URPGGameplayInterface* GameplayInterface;

	UPROPERTY()
	ARPGBasePlayerCharacter* PlayerPawn;

	UPROPERTY()
	UMaterialInstance* ClockProgressMatInst;

	UPROPERTY()
	UMaterialInstanceDynamic* ClockProgressMatInstDynamic;

	FTimerHandle OffTimer;

	/** �κ��丮 */

	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<URPGInventorySlotWidget> ItemSlotClass;

	UPROPERTY()
	TArray<URPGInventorySlotWidget*> ActivadtedSlots;

	UPROPERTY()
	TArray<URPGInventorySlotWidget*> EmptySlots;

	UPROPERTY()
	TMap<int32, URPGInventorySlotWidget*> ItemSlotMap;

	/** �������� ����� ���� ���� */
	int32 SavedItemSlotCount = 0;

	/** Ȱ��ȭ�� ���� ���� */
	int32 ActivatedItemSlotNum = 0;

	UPROPERTY(EditAnywhere, Category = "HUD")
	UDataTable* ItemDataTable;

	/** ������ ���� �޴�  */
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<URPGItemSlotMenuWidget> ItemSlotMenuClass;

	UPROPERTY()
	URPGItemSlotMenuWidget* ItemSlotMenuWidget;

	bool bIsItemSlotMenuWidgetOn = false;

	UPROPERTY(VisibleInstanceOnly, Category = "HUD | Inventory Status")
	int32 SelectedItemUniqueNum = -1;

	UPROPERTY(VisibleInstanceOnly, Category = "HUD | Inventory Status")
	int32 EquippedArmourUniqueNum = -1;

	UPROPERTY(VisibleInstanceOnly, Category = "HUD | Inventory Status")
	int32 EquippedAccessoriesUniqueNum = -1;

	/** ������ ���� ����â */
	UPROPERTY(EditAnywhere, Category = "HUD")
	TSubclassOf<URPGStatTextBoxWidget> ItemStatBoxClass;

	UPROPERTY()
	URPGStatTextBoxWidget* ItemStatBoxWidget;

	bool bIsStatInfoWidgetOn = false;

	/** ������ ���� */

	UPROPERTY()
	TSubclassOf<URPGDamageWidget> DamageWidgetClass;

	UPROPERTY()
	TArray<URPGDamageWidget*> DamageWidgetArr;

	const int32 DamageWidgetNumber = 30;
};