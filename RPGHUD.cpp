
#include "UI/RPGHUD.h"
#include "UI/RPGCharacterSelectionInterface.h"
#include "UI/RPGGameplayInterface.h"
#include "UI/RPGInventoryWidget.h"
#include "UI/RPGInventorySlotWidget.h"
#include "UI/RPGItemSlotMenuWidget.h"
#include "UI/RPGStatTextBoxWidget.h"
#include "UI/RPGStatInfoWidget.h"
#include "UI/RPGDamageWidget.h"
#include "Player/Character/RPGBasePlayerCharacter.h"
#include "Player/RPGPlayerController.h"
#include "Item/RPGItem.h"
#include "GameSystem/ItemSpawnManagerComponent.h"
#include "../RPG.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DataTable.h"
#include "UObject/ConstructorHelpers.h"

ARPGHUD::ARPGHUD()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FClassFinder<URPGCharacterSelectionInterface> WBP_CharacterSelect(TEXT("WidgetBlueprint'/Game/_Assets/Blueprints/HUD/WBP_CharacterSelect.WBP_CharacterSelect_C'"));
	if (WBP_CharacterSelect.Succeeded()) CharacterSelectWBPClass = WBP_CharacterSelect.Class;

	static ConstructorHelpers::FObjectFinder<UMaterialInstance> Obj_CooldownProgress(TEXT("/Game/_Assets/Materials/Circular/MI_ClockProgress.MI_ClockProgress"));
	if (Obj_CooldownProgress.Succeeded()) ClockProgressMatInst = Obj_CooldownProgress.Object;

	static ConstructorHelpers::FClassFinder<UUserWidget> DamageWidgetAsset(TEXT("WidgetBlueprint'/Game/_Assets/Blueprints/HUD/WBP_DamageWidget.WBP_DamageWidget_C'"));
	if (DamageWidgetAsset.Succeeded()) { DamageWidgetClass = DamageWidgetAsset.Class; }

	DamageWidgetArr.Init(nullptr, DamageWidgetNumber);
}

void ARPGHUD::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	
}

void ARPGHUD::BeginPlay()
{
	Super::BeginPlay();

	if (CharacterSelectWBPClass)
	{
		CharacterSelectionInterface = CreateWidget<URPGCharacterSelectionInterface>(GetOwningPlayerController(), CharacterSelectWBPClass);

		CharacterSelectionInterface->Button_Warrior->OnClicked.AddDynamic(this, &ARPGHUD::OnWarriorSelected);
		CharacterSelectionInterface->Button_Sorcerer->OnClicked.AddDynamic(this, &ARPGHUD::OnSorcererSelected);

		CharacterSelectionInterface->AddToViewport();
	}
}

void ARPGHUD::OnWarriorSelected()
{
	OnCharacterSelected(ECharacterType::ECT_Warrior);
	CharacterSelectionInterface->RemoveFromParent();
}

void ARPGHUD::OnSorcererSelected()
{
	OnCharacterSelected(ECharacterType::ECT_Sorcerer);
	CharacterSelectionInterface->RemoveFromParent();
}

void ARPGHUD::OnCharacterSelected(ECharacterType Type)
{
	ARPGPlayerController* RPGController = Cast<ARPGPlayerController>(GetOwningPlayerController());
	if (RPGController == nullptr) return;

	RPGController->OnCharacterSelected(Type);
}

void ARPGHUD::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateCooldownProgress();
}

// 각 위젯들을 뷰포트에 띄우기
void ARPGHUD::InitHUD()
{
	CastPawnAndBindFunctions();

	if (ItemSlotMenuClass)
	{
		ItemSlotMenuWidget = CreateWidget<URPGItemSlotMenuWidget>(GetOwningPlayerController(), ItemSlotMenuClass);
		ItemSlotMenuWidget->GetUseButton()->OnClicked.AddDynamic(this, &ARPGHUD::OnUseOrEquipButtonClicked);
		ItemSlotMenuWidget->GetDiscardButton()->OnClicked.AddDynamic(this, &ARPGHUD::OnDiscardButtonClicked);
		ItemSlotMenuWidget->SetVisibility(ESlateVisibility::Hidden);
		ItemSlotMenuWidget->AddToViewport(2);
	}

	if (ItemStatBoxClass)
	{
		ItemStatBoxWidget = CreateWidget<URPGStatTextBoxWidget>(GetOwningPlayerController(), ItemStatBoxClass);
		ItemStatBoxWidget->SetVisibility(ESlateVisibility::Hidden);
		ItemStatBoxWidget->AddToViewport(1);
	}

	if (DamageWidgetClass)
	{
		for (int32 Idx = 0; Idx < DamageWidgetNumber; Idx++)
		{
			URPGDamageWidget* DamageWidget = CreateWidget<URPGDamageWidget>(GetOwningPlayerController(), DamageWidgetClass);
			DamageWidget->InitDamageWidget();
			DamageWidgetArr[Idx] = DamageWidget;
			DamageWidget->AddToViewport();
		}
	}

	DrawOverlay();
}

void ARPGHUD::CastPawnAndBindFunctions()
{
	if (GetOwningPlayerController())
	{
		PlayerPawn = Cast<ARPGBasePlayerCharacter>(GetOwningPlayerController()->GetPawn());
	}

	if (PlayerPawn)
	{
		PlayerPawn->DOnChangeHealthPercentage.AddUFunction(this, FName("SetHealthBarPercentage"));
		PlayerPawn->DOnChangeManaPercentage.AddUFunction(this, FName("SetManaBarPercentage"));
		PlayerPawn->DOnAbilityCooldownEnd.AddUFunction(this, FName("CooldownProgressSetFull"));
	}
}

// 캐릭터 리스폰 시 HUD 초기화 후 다시 띄우기
void ARPGHUD::ReloadHUD()
{
	CastPawnAndBindFunctions();

	SelectedItemUniqueNum = -1;
	bIsStatInfoWidgetOn = false;
	bIsItemSlotMenuWidgetOn = false;

	GameplayInterface->MinimapWidget->SetBrushFromMaterial(PlayerPawn->GetMinimapMatInst());
	GameplayInterface->StatInfoWidget->SetVisibility(ESlateVisibility::Hidden);
	GameplayInterface->InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
	ItemStatBoxWidget->SetVisibility(ESlateVisibility::Hidden);
	ItemSlotMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	GameplayInterface->SetVisibility(ESlateVisibility::Visible);
}

void ARPGHUD::OffHUD()
{
	GameplayInterface->SetVisibility(ESlateVisibility::Hidden);
}

void ARPGHUD::DrawOverlay()
{
	if (GameplayInterfaceClass)
	{
		GameplayInterface = CreateWidget<URPGGameplayInterface>(GetOwningPlayerController(), GameplayInterfaceClass);
		GameplayInterface->InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		GameplayInterface->MinimapWidget->SetBrushFromMaterial(PlayerPawn->GetMinimapMatInst());
		GameplayInterface->AddToViewport(0);
		InitInventorySlot();
	}
}

void ARPGHUD::SetHealthBarPercentage(float Percentage)
{
	GameplayInterface->HealthBar->SetPercent(Percentage);
	if (Percentage == 0.f)
	{
		GetWorldTimerManager().SetTimer(OffTimer, this, &ARPGHUD::OffHUD, 3.f);
	}
}

void ARPGHUD::SetManaBarPercentage(float Percentage)
{
	GameplayInterface->ManaBar->SetPercent(Percentage);
}

void ARPGHUD::UpdateCooldownProgress()
{
	if (PlayerPawn && PlayerPawn->IsLocallyControlled() && PlayerPawn->GetAbilityCooldownBit())
	{
		for (int8 idx = 0; idx < 4; idx++)
		{
			SetProgressPercentage(idx, PlayerPawn->GetCooldownPercentage(idx));
		}
	}
}

// 스킬 프로그레스 바 퍼센티지 갱신
void ARPGHUD::SetProgressPercentage(const int8 Index, const float Percentage)
{
	ClockProgressMatInstDynamic = UMaterialInstanceDynamic::Create(ClockProgressMatInst, this);
	ClockProgressMatInstDynamic->SetScalarParameterValue(FName("Percent"), Percentage);

	switch (Index)
	{
	case 0:
		GameplayInterface->ClockProgress_Q->SetBrushResourceObject(ClockProgressMatInstDynamic);
		break;
	case 1:
		GameplayInterface->ClockProgress_W->SetBrushResourceObject(ClockProgressMatInstDynamic);
		break;
	case 2:
		GameplayInterface->ClockProgress_E->SetBrushResourceObject(ClockProgressMatInstDynamic);
		break;
	case 3:
		GameplayInterface->ClockProgress_R->SetBrushResourceObject(ClockProgressMatInstDynamic);
		break;
	}
}

void ARPGHUD::CooldownProgressSetFull(uint8 Bit)
{
	SetProgressPercentage(Bit, 1);
}

/** ---------------------- 인벤토리 ---------------------- */

// 인벤토리 슬롯 초기화
void ARPGHUD::InitInventorySlot()
{
	if (GameplayInterface->InventoryWidget == nullptr || ItemSlotClass == nullptr) return;

	// 슬롯 96개 미리 생성
	for (int32 Idx = 0; Idx < 96; Idx++)
	{
		URPGInventorySlotWidget* InvSlot = CreateWidget<URPGInventorySlotWidget>(GetOwningPlayerController(), ItemSlotClass);
		if (InvSlot)
		{
			// 16개만 활성화
			if (Idx < 16)
			{	
				GameplayInterface->InventoryWidget->AddSlotToGridPanel(InvSlot, Idx / 4, Idx % 4);
				ActivadtedSlots.Add(InvSlot);
			}
			else
			{
				InvSlot->SetVisibility(ESlateVisibility::Hidden);
			}

			InvSlot->BindButtonEvent();
			InvSlot->DOnIconButtonClicked.AddUFunction(this, FName("OnItemSlotButtonClickEvent"));
			InvSlot->DOnIconButtonHovered.AddUFunction(this, FName("OnItemSlotButtonHoveredEvent"));
			
			EmptySlots.Add(InvSlot);
		}
	}

	ActivatedItemSlotNum = 16;
}

void ARPGHUD::InventoryWidgetToggle(const bool bInventoryOn)
{
	if (bInventoryOn)
	{
		GameplayInterface->InventoryWidget->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		GameplayInterface->InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

// 코인 획득
void ARPGHUD::AddCoins(const int32& CoinAmount)
{
	if (GameplayInterface->InventoryWidget == nullptr) return;

	GameplayInterface->InventoryWidget->AddCoins(CoinAmount);
}

// 포션 획득
void ARPGHUD::AddPotion(const int32& UniqueNum, const EItemType ItemType, const int32& PotionCount)
{
	ExpandInventoryIfNoSpace();

	if (ItemSlotMap.Contains(UniqueNum) == false)
	{
		ItemSlotMap.Add(UniqueNum, ActivadtedSlots[SavedItemSlotCount]);
		ActivadtedSlots[SavedItemSlotCount]->SaveItemToSlot(ItemType);
		ActivadtedSlots[SavedItemSlotCount]->SetItemCountText(PotionCount);
		ActivadtedSlots[SavedItemSlotCount]->SetUniqueNumber(UniqueNum);
		SetSlotIcon(UniqueNum, ItemType);
		SavedItemSlotCount++;
	}
	else
	{
		(*ItemSlotMap.Find(UniqueNum))->SetItemCountText(PotionCount);
	}
}

// 장비 획득
void ARPGHUD::AddEquipment(const int32& UniqueNum, const EItemType ItemType)
{
	ExpandInventoryIfNoSpace();

	ItemSlotMap.Add(UniqueNum, ActivadtedSlots[SavedItemSlotCount]);
	ActivadtedSlots[SavedItemSlotCount]->SaveItemToSlot(ItemType);
	ActivadtedSlots[SavedItemSlotCount]->SetItemCountText(1);
	ActivadtedSlots[SavedItemSlotCount]->SetUniqueNumber(UniqueNum);
	SetSlotIcon(UniqueNum, ItemType);
	SavedItemSlotCount++;
}

void ARPGHUD::UpdateItemCount(const int32& UniqueNum, const int32& ItemCount)
{
	if (ItemCount == 0)
	{
		SavedItemSlotCount--;

		ClearItemSlot(UniqueNum);

		// 슬롯 1 페이지(16개 슬롯)에서 한 개의 아이템만 저장되어 있는 경우
		// 페이지 전체 비활성화
		if(ActivatedItemSlotNum > 16 && SavedItemSlotCount % 16 == 0)
		{
			if (GameplayInterface->InventoryWidget == nullptr)
			{
				GameplayInterface->InventoryWidget->RemoveSlotPage();
			}
			// 배열 뒤에서부터 제거
			for (int32 Idx = ActivatedItemSlotNum - 1; Idx > ActivatedItemSlotNum - 16; Idx--)
			{
				ActivadtedSlots.RemoveAt(Idx);
			}
			ActivatedItemSlotNum -= 16;
		}
	}
	else
	{
		(*ItemSlotMap.Find(UniqueNum))->SetItemCountText(ItemCount);
	}
}

// 아이템 슬롯 비우기
void ARPGHUD::ClearItemSlot(const int32& UniqueNum)
{
	// 슬롯 UI 초기화
	URPGInventorySlotWidget* SlotToRemove = (*ItemSlotMap.Find(UniqueNum));
	SlotToRemove->ClearSlot();

	// 맵에서 제거
	ItemSlotMap.Remove(UniqueNum);

	if (SavedItemSlotCount > 0)
	{
		// 활성화된 슬롯 뒤로 보내기
		int32 Index;
		ActivadtedSlots.Find(SlotToRemove, Index);
		for (int32 Idx = Index; Idx < SavedItemSlotCount; Idx++)
		{
			ActivadtedSlots[Idx] = ActivadtedSlots[Idx + 1];

		}
		ActivadtedSlots[SavedItemSlotCount] = SlotToRemove;

		// 그리드 재정렬
		if (GameplayInterface->InventoryWidget)
		{
			GameplayInterface->InventoryWidget->SortGridPanel(SlotToRemove, Index, SavedItemSlotCount);
		}
	}
}

void ARPGHUD::SetSlotIcon(const int32& UniqueNum, const EItemType ItemType)
{
	const int32 RowNumber = StaticCast<int32>(ItemType);
	FItemOptionTableRow* ItemTableRow = ItemDataTable->FindRow<FItemOptionTableRow>(FName(*(FString::FormatAsNumber(RowNumber))), FString(""));
	if (ItemTableRow && ItemTableRow->ItemIcon)
	{
		(*ItemSlotMap.Find(UniqueNum))->SetSlotIcon(ItemTableRow->ItemIcon);
	}
}

void ARPGHUD::ExpandInventoryIfNoSpace()
{
	if (GameplayInterface->InventoryWidget == nullptr) return;

	if (SavedItemSlotCount == ActivatedItemSlotNum)
	{
		if (ActivatedItemSlotNum == 96)
		{
			ELOG(TEXT("Inventory is full!"));
			return;
		}

		for (int32 Idx = ActivatedItemSlotNum; Idx < ActivatedItemSlotNum + 16; Idx++)
		{
			ActivadtedSlots.Add(EmptySlots[Idx]);
			EmptySlots[Idx]->SetVisibility(ESlateVisibility::Visible);
			GameplayInterface->InventoryWidget->AddSlotToGridPanel(EmptySlots[Idx], Idx / 4, Idx % 4);
		}

		ActivatedItemSlotNum += 16;
	}
}

void ARPGHUD::OnItemSlotButtonClickEvent(const int32 UniqueNum)
{
	bIsItemSlotMenuWidgetOn = true;
	SelectedItemUniqueNum = UniqueNum;

	if (UniqueNum < 2)
	{
		ItemSlotMenuWidget->SetUseText(FString(TEXT("사용하기")));
		
	}
	else
	{
		if (UniqueNum == EquippedArmourUniqueNum || UniqueNum == EquippedAccessoriesUniqueNum)
		{
			ItemSlotMenuWidget->SetUseText(FString(TEXT("장착해제")));
		}
		else
		{
			ItemSlotMenuWidget->SetUseText(FString(TEXT("장착하기")));
		}
	}

	FVector2D DrawPosition;
	GetPositionUnderCursor(DrawPosition);
	ItemSlotMenuWidget->SetWidgetPosition(DrawPosition);
	ItemSlotMenuWidget->SetVisibility(ESlateVisibility::Visible);
}

void ARPGHUD::GetPositionUnderCursor(FVector2D& Position)
{
	float MX, MY;
	GetOwningPlayerController()->GetMousePosition(MX, MY);
	int32 VX, VY;
	GetOwningPlayerController()->GetViewportSize(VX, VY);

	Position.X = MX - (VX / 2);
	Position.Y = MY - (VY / 2);
}

// 사용 / 장착 버튼 클릭 이벤트
void ARPGHUD::OnUseOrEquipButtonClicked()
{
	ARPGPlayerController* RPGController = Cast<ARPGPlayerController>(GetOwningPlayerController());
	if (RPGController == nullptr) return;

	ItemSlotMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	
	bIsItemSlotMenuWidgetOn = false;

	// 선택된 아이템이 포션일 경우 
	if (SelectedItemUniqueNum < 2)
	{
		RPGController->UseItem(SelectedItemUniqueNum);
	}
	else // 선택된 아이템이 장비일 경우
	{
		// 장착된 장비 번호와 선택된 장비 번호가 같으면 아이템이 장착된 상태이므로 장착 해제
		if (SelectedItemUniqueNum == EquippedArmourUniqueNum)
		{
			RPGController->DoUnequipItem(EquippedArmourUniqueNum);
			UnequipItem(EquippedArmourUniqueNum);
		}
		else if (SelectedItemUniqueNum == EquippedAccessoriesUniqueNum)
		{
			RPGController->DoUnequipItem(EquippedAccessoriesUniqueNum);
			UnequipItem(EquippedAccessoriesUniqueNum);
		}
		else // 그렇지 않으면 장비를 장착
		{
			URPGInventorySlotWidget* SelectedSlot = (*ItemSlotMap.Find(SelectedItemUniqueNum));
			const EItemType SelectedItemType = SelectedSlot->GetSavedItemType();

			if (SelectedItemType == EItemType::EIT_Armour)
			{
				if (EquippedArmourUniqueNum != -1)
				{
					RPGController->DoUnequipItem(EquippedArmourUniqueNum);
					UnequipItem(EquippedArmourUniqueNum);
				}
			}
			else if (SelectedItemType == EItemType::EIT_Accessories)
			{
				if (EquippedAccessoriesUniqueNum != -1)
				{
					RPGController->DoUnequipItem(EquippedAccessoriesUniqueNum);
					UnequipItem(EquippedAccessoriesUniqueNum);
				}
			}

			RPGController->DoEquipItem(SelectedItemUniqueNum);
			EquipItem(SelectedItemUniqueNum);
		}
	}

	SelectedItemUniqueNum = -1;

	RPGController->DoUpdateEquippedItemStat();
}

void ARPGHUD::EquipItem(const int32& UniqueNum)
{
	if (UniqueNum < 2) return;
	if (ItemSlotMap.Contains(UniqueNum) == false) return;

	URPGInventorySlotWidget* SelectedSlot = (*ItemSlotMap.Find(UniqueNum));
	const EItemType SelectedItemType = SelectedSlot->GetSavedItemType();

	// 장비 창에 장착할 아이템의 아이콘으로 설정
	GameplayInterface->InventoryWidget->SetEquipmentSlot(SelectedItemType, SelectedSlot->GetIconMaterial());
	SelectedSlot->SetBorderStateToEquipped(true);

	if (SelectedItemType == EItemType::EIT_Armour)
	{
		EquippedArmourUniqueNum = UniqueNum;
	}
	else if (SelectedItemType == EItemType::EIT_Accessories)
	{
		EquippedAccessoriesUniqueNum = UniqueNum;
	}
}

void ARPGHUD::UnequipItem(const int32& UniqueNum)
{
	if (UniqueNum < 2) return;
	if (ItemSlotMap.Contains(UniqueNum) == false) return;

	URPGInventorySlotWidget* SelectedSlot = (*ItemSlotMap.Find(UniqueNum));
	const EItemType SelectedItemType = SelectedSlot->GetSavedItemType();

	// 장착된 아이템 슬롯 초기화
	GameplayInterface->InventoryWidget->ClearEquipmentSlot(SelectedItemType);
	SelectedSlot->SetBorderStateToEquipped(false);

	if (SelectedItemType == EItemType::EIT_Armour)
	{
		EquippedArmourUniqueNum = -1;
	}
	else if (SelectedItemType == EItemType::EIT_Accessories)
	{
		EquippedAccessoriesUniqueNum = -1;
	}
}

// 버리기 버튼 이벤트
void ARPGHUD::OnDiscardButtonClicked()
{
	ARPGPlayerController* RPGController = Cast<ARPGPlayerController>(GetOwningPlayerController());
	if (RPGController == nullptr) return;

	ItemSlotMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	
	bIsItemSlotMenuWidgetOn = false;

	RPGController->DiscardItem(SelectedItemUniqueNum);

	if (SelectedItemUniqueNum == EquippedArmourUniqueNum || SelectedItemUniqueNum == EquippedAccessoriesUniqueNum)
	{
		UnequipItem(SelectedItemUniqueNum);
		RPGController->DoUpdateEquippedItemStat();
	}

	SelectedItemUniqueNum = -1;
}

// 슬롯에 커서가 올라가면 스탯 정보창 표시
void ARPGHUD::OnItemSlotButtonHoveredEvent(int32 UniqueNum)
{
	if (UniqueNum != -1)
	{
		ARPGPlayerController* RPGController = Cast<ARPGPlayerController>(GetOwningPlayerController());
		if (RPGController == nullptr) return;

		RPGController->GetItemInfoStruct(UniqueNum);
	}
	else
	{
		HideItemStatTextBox();
	}
}

void ARPGHUD::ShowItemStatTextBox(const FItemInfo& Info)
{
	const int32 RowNumber = StaticCast<int32>(Info.ItemType);
	FItemOptionTableRow* ItemTableRow = ItemDataTable->FindRow<FItemOptionTableRow>(FName(*(FString::FormatAsNumber(RowNumber))), FString(""));

	FString StatString;
	const int8 ArrEnd = ItemTableRow->PropertyNames.Num();
	for (int8 Idx = 0; Idx < ArrEnd; Idx++)
	{
		if (Info.ItemStatArr[Idx] > 0)
		{
			if (StatString.Len() > 0) StatString += FString(TEXT("\n"));
			StatString += FString::Printf(TEXT("%s : %.1f"), *ItemTableRow->PropertyNames[Idx], Info.ItemStatArr[Idx]);
		}
	}

	ItemStatBoxWidget->SetStatText(StatString);
	FVector2D DrawPosition;
	GetPositionUnderCursor(DrawPosition);
	ItemStatBoxWidget->SetWidgetPosition(DrawPosition);
	ItemStatBoxWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ARPGHUD::HideItemStatTextBox()
{
	ItemStatBoxWidget->SetVisibility(ESlateVisibility::Hidden);
}

void ARPGHUD::ToggleStatInfoWidget()
{
	if (bIsStatInfoWidgetOn)
	{
		GameplayInterface->StatInfoWidget->SetVisibility(ESlateVisibility::Hidden);
		bIsStatInfoWidgetOn = false;
	}
	else
	{
		GameplayInterface->StatInfoWidget->SetVisibility(ESlateVisibility::Visible);
		bIsStatInfoWidgetOn = true;
	}
}

void ARPGHUD::UpdateStatCharacterStatText(const FStatInfo& Stats)
{
	GameplayInterface->StatInfoWidget->UpdateStatCharacterStatText(Stats);
}

void ARPGHUD::UpdateStatEquippedItemStatText(const FStatInfo& Stats)
{
	GameplayInterface->StatInfoWidget->UpdateStatEquippedItemStatText(Stats);
}

void ARPGHUD::PopUpDamageWidget(const FVector_NetQuantize& PopupPosition, const int32 Damage)
{
	FVector2D ScreenLocation;
	GetOwningPlayerController()->ProjectWorldLocationToScreen(PopupPosition, ScreenLocation);

	for (int32 Idx = 0; Idx < DamageWidgetNumber; Idx++)
	{
		if (DamageWidgetArr[Idx]->IsWidgetUsable())
		{
			DamageWidgetArr[Idx]->SetPositionInViewport(ScreenLocation);
			DamageWidgetArr[Idx]->SetDamageTextAndPopup(Damage);
			return;
		}
	}
}
