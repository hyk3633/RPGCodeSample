
#include "Player/RPGPlayerController.h"
#include "Player/Character/RPGBasePlayerCharacter.h"
#include "Player/RPGPlayerState.h"
#include "Item/RPGItem.h"
#include "UI/RPGHUD.h"
#include "UI/RPGDamageWidget.h"
#include "GameInstance/RPGGameInstance.h"
#include "../RPGGameModeBase.h"
#include "../RPG.h"
#include "Enums/PressedKey.h"
#include "GameInstance/RPGGameInstance.h"
#include "GameFramework/HUD.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"

ARPGPlayerController::ARPGPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Obj_DefaultContext (TEXT("/Game/_Assets/Input/IMC_DefaulnputMappingContext.IMC_DefaulnputMappingContext"));
	if (Obj_DefaultContext.Succeeded()) DefaultMappingContext = Obj_DefaultContext.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_LeftClick (TEXT("/Game/_Assets/Input/IA_LeftClick.IA_LeftClick"));
	if (Obj_LeftClick.Succeeded()) LeftClickAction = Obj_LeftClick.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_RightClick (TEXT("/Game/_Assets/Input/IA_RightClick.IA_RightClick"));
	if (Obj_RightClick.Succeeded()) RightClickAction = Obj_RightClick.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_Q (TEXT("/Game/_Assets/Input/IA_Q.IA_Q"));
	if (Obj_Q.Succeeded()) QPressedAction = Obj_Q.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_W (TEXT("/Game/_Assets/Input/IA_W.IA_W"));
	if (Obj_W.Succeeded()) WPressedAction = Obj_W.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_E (TEXT("/Game/_Assets/Input/IA_E.IA_E"));
	if (Obj_E.Succeeded()) EPressedAction = Obj_E.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_R (TEXT("/Game/_Assets/Input/IA_R.IA_R"));
	if (Obj_R.Succeeded()) RPressedAction = Obj_R.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_I(TEXT("/Game/_Assets/Input/IA_I.IA_I"));
	if (Obj_I.Succeeded()) IPressedAction = Obj_I.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_S(TEXT("/Game/_Assets/Input/IA_S.IA_S"));
	if (Obj_S.Succeeded()) SPressedAction = Obj_S.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction> Obj_MouseWheelScroll (TEXT("/Game/_Assets/Input/IA_ZoomInOut.IA_ZoomInOut"));
	if (Obj_MouseWheelScroll.Succeeded()) MouseWheelScroll = Obj_MouseWheelScroll.Object;
}

void ARPGPlayerController::PostInitializeComponents()
{
	Super::PostInitializeComponents();

}

void ARPGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void ARPGPlayerController::OnCharacterSelected(ECharacterType Type)
{
	OnCharacterSelectedServer(Type);
}

void ARPGPlayerController::OnCharacterSelectedServer_Implementation(ECharacterType Type)
{
	GetWorld()->GetAuthGameMode<ARPGGameModeBase>()->SpawnPlayerCharacterAndPossess(this, Type);
}

void ARPGPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	MyCharacter = Cast<ARPGBasePlayerCharacter>(InPawn);

	if (HasAuthority())
	{
		ReloadCharacterAndEquipmentStat();
	}
}

void ARPGPlayerController::ReloadCharacterAndEquipmentStat()
{
	if (MyCharacter)
	{
		const FStatInfo& Stats = GetPlayerState<ARPGPlayerState>()->GetCurrentCharacterStats();
		MyCharacter->InitCharacterStats(Stats);

		const FStatInfo& EquipmentStats = GetPlayerState<ARPGPlayerState>()->GetEquippedItemStats();
		MyCharacter->SetEquipmentArmourStats(EquipmentStats.DefenseivePower, EquipmentStats.Dexterity, EquipmentStats.MaxHP, EquipmentStats.MaxMP);
		MyCharacter->SetEquipmentAccessoriesStats(EquipmentStats.StrikingPower, EquipmentStats.SkillPower, EquipmentStats.AttackSpeed);

		MyCharacter->ResetHealthMana();
	}
}

void ARPGPlayerController::OnRep_MyCharacter()
{
	if (RPGHUD == nullptr)
	{
		ARPGHUD* TempHUD = Cast<ARPGHUD>(GetHUD());
		if (TempHUD)
		{
			RPGHUD = TempHUD;
			RPGHUD->InitHUD();
		}
		else
		{
			WLOG(TEXT("Character cast is Failed!"));
		}
	}
	else
	{
		RPGHUD->ReloadHUD();
	}

	if (MyCharacter)
	{
		MyCharacter->ResetHealthManaUI();
	}

	bIsInventoryOn = false;

	if (TracedItem)
	{
		TracedItem->SetRenderCustomDepthOff();
		TracedItem = nullptr;
	}
}

void ARPGPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MyCharacter)
	{
		ItemTrace();
	}
}

void ARPGPlayerController::ItemTrace()
{
	if (IsValid(TracedItem))
	{
		TracedItem->SetItemNameTagVisibility(false);
		TracedItem->SetRenderCustomDepthOff();
	}
	if (MyCharacter->GetAiming())
	{
		TracedItem = nullptr;
		return;
	}

	FHitResult ItemTraceHit;
	GetHitResultUnderCursor(ECC_ItemTrace, false, ItemTraceHit);

	if (ItemTraceHit.bBlockingHit)
	{
		TracedItem = Cast<ARPGItem>(ItemTraceHit.GetActor());
		if (IsValid(TracedItem))
		{
			TracedItem->SetItemNameTagVisibility(true);
			TracedItem->SetRenderCustomDepthOn(200);
		}
	}
	else
	{
		TracedItem = nullptr;
	}
}

void ARPGPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Started, this, &ARPGPlayerController::LeftClickAction_StopMove);
		EnhancedInputComponent->BindAction(LeftClickAction, ETriggerEvent::Completed, this, &ARPGPlayerController::LeftClickAction_SetPath);
		EnhancedInputComponent->BindAction(RightClickAction, ETriggerEvent::Completed, this, &ARPGPlayerController::RightClick_AttackOrSetAbilityPoint);
		EnhancedInputComponent->BindAction(QPressedAction, ETriggerEvent::Completed, this, &ARPGPlayerController::QPressedAction_Cast);
		EnhancedInputComponent->BindAction(WPressedAction, ETriggerEvent::Completed, this, &ARPGPlayerController::WPressedAction_Cast);
		EnhancedInputComponent->BindAction(EPressedAction, ETriggerEvent::Completed, this, &ARPGPlayerController::EPressedAction_Cast);
		EnhancedInputComponent->BindAction(RPressedAction, ETriggerEvent::Completed, this, &ARPGPlayerController::RPressedAction_Cast);
		EnhancedInputComponent->BindAction(IPressedAction, ETriggerEvent::Completed, this, &ARPGPlayerController::IPressedAction_ToggleInventory);
		EnhancedInputComponent->BindAction(SPressedAction, ETriggerEvent::Completed, this, &ARPGPlayerController::SPressedAction_ToggleStatInfo);
		EnhancedInputComponent->BindAction(MouseWheelScroll, ETriggerEvent::Triggered, this, &ARPGPlayerController::MouseWheelScroll_ZoomInOut);
	}
}

/** -------------- 왼쪽 클릭 : 이동, 스킬 취소, 아이템 획득 -------------- */

void ARPGPlayerController::LeftClickAction_StopMove()
{
	if (MyCharacter == nullptr || MyCharacter->GetStunned()) return;
	MyCharacter->StopMove();
}

void ARPGPlayerController::LeftClickAction_SetPath()
{
	if (MyCharacter == nullptr || MyCharacter->GetStunned()) return;
	if (MyCharacter->IsAbilityERMontagePlaying()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else if (IsValid(TracedItem) && TracedItem->GetDistanceTo(MyCharacter) < 500.f)
	{
		PickupItemServer(TracedItem);
	}
	else if(bIsInventoryOn == false)
	{
		MyCharacter->SetDestinationAndPath();
	}
}
/** -------------- 아이템 획득 -------------- */

void ARPGPlayerController::PickupItemServer_Implementation(ARPGItem* Item)
{
	PickupItem(Item);
}

void ARPGPlayerController::PickupItem(ARPGItem* Item)
{
	const int32 Num = GetPlayerState<ARPGPlayerState>()->AddItem(Item);

	const EItemType Type = Item->GetItemInfo().ItemType;
	switch (Type)
	{
	case EItemType::EIT_Coin:
		PickupCoinsClient(GetPlayerState<ARPGPlayerState>()->GetCoins());
		break;
	case EItemType::EIT_HealthPotion:
	case EItemType::EIT_ManaPotion:
		PickupPotionClient(Num, Type, GetPlayerState<ARPGPlayerState>()->GetItemCount(Num));
		break;
	case EItemType::EIT_Armour:
	case EItemType::EIT_Accessories:
		PickupEquipmentClient(Num, Type);
		break;
	}

	Item->DeactivateItemFromAllClients();
}

void ARPGPlayerController::PickupCoinsClient_Implementation(const int32 CoinAmount)
{
	if (RPGHUD)
	{
		const EItemType TracedItemType = TracedItem->GetItemInfo().ItemType;
		RPGHUD->AddCoins(CoinAmount);
	}
}

void ARPGPlayerController::PickupPotionClient_Implementation(const int32 UniqueNum, const EItemType PotionType, const int32 PotionCount)
{
	if (RPGHUD)
	{
		RPGHUD->AddPotion(UniqueNum, PotionType, PotionCount);
	}
}

void ARPGPlayerController::PickupEquipmentClient_Implementation(const int32 UniqueNum, const EItemType ItemType)
{
	if (RPGHUD)
	{
		RPGHUD->AddEquipment(UniqueNum, ItemType);
	}
}

/** -------------- 아이템 사용 -------------- */

void ARPGPlayerController::UseItem(const int32& UniqueNum)
{
	UseItemServer(UniqueNum);
}

void ARPGPlayerController::UseItemServer_Implementation(const int32 UniqueNum)
{
	GetPlayerState<ARPGPlayerState>()->UseItem(UniqueNum);
	UniqueNum == 0 ? MyCharacter->RecoveryHealth(200) : MyCharacter->RecoveryMana(200);
	const int32& ItemCount = GetPlayerState<ARPGPlayerState>()->GetItemCount(UniqueNum);
	UpdateItemInfoClient(UniqueNum, ItemCount);
}

void ARPGPlayerController::UpdateItemInfoClient_Implementation(const int32 UniqueNum, const int32 ItemCount)
{
	if (RPGHUD == nullptr) return;

	if (UniqueNum < 2)
	{
		RPGHUD->UpdateItemCount(UniqueNum, ItemCount);
	}
	else
	{
		RPGHUD->UpdateItemCount(UniqueNum, 0);
	}
}

/** -------------- 아이템 장착 -------------- */

void ARPGPlayerController::DoEquipItem(const int32& UniqueNum)
{
	EquipItemServer(UniqueNum);
}

void ARPGPlayerController::EquipItemServer_Implementation(const int32 UniqueNum)
{
	EquipItem(UniqueNum);
}

void ARPGPlayerController::EquipItem(const int32& UniqueNum)
{
	// 플레이어 스테이트에 장착 아이템 정보 저장
	const bool bSuccess = GetPlayerState<ARPGPlayerState>()->EquipItem(UniqueNum);
	if (bSuccess == false) return;

	FItemInfo ItemInfo;
	const bool bIsExist = GetPlayerState<ARPGPlayerState>()->GetItemStatInfo(UniqueNum, ItemInfo);

	if (IsValid(MyCharacter) && bIsExist)
	{
		if (ItemInfo.ItemType == EItemType::EIT_Armour)
		{
			MyCharacter->SetEquipmentArmourStats(ItemInfo.ItemStatArr[0], ItemInfo.ItemStatArr[1], ItemInfo.ItemStatArr[2], ItemInfo.ItemStatArr[3]);
		}
		else
		{
			MyCharacter->SetEquipmentAccessoriesStats(ItemInfo.ItemStatArr[0], ItemInfo.ItemStatArr[1], ItemInfo.ItemStatArr[2]);
		}
	}
}

/** -------------- 아이템 장착 해제 -------------- */

void ARPGPlayerController::DoUnequipItem(const int32& UniqueNum)
{
	UnequipItemServer(UniqueNum);
}

void ARPGPlayerController::UnequipItemServer_Implementation(const int32 UniqueNum)
{
	UnequipItem(UniqueNum);
}

void ARPGPlayerController::UnequipItem(const int32& UniqueNum)
{
	FItemInfo ItemInfo;
	const bool bIsExist = GetPlayerState<ARPGPlayerState>()->GetItemStatInfo(UniqueNum, ItemInfo);

	if (IsValid(MyCharacter) && bIsExist)
	{
		if (ItemInfo.ItemType == EItemType::EIT_Armour)
		{
			MyCharacter->InitializeEquipmentArmourStats();
		}
		else
		{
			MyCharacter->InitializeEquipmentAccessoriesStats();
		}
	}

	GetPlayerState<ARPGPlayerState>()->UnequipItem(UniqueNum);
}

/** -------------- 아이템 버리기 -------------- */

void ARPGPlayerController::DiscardItem(const int32& UniqueNum)
{
	DiscardItemServer(UniqueNum);
}

void ARPGPlayerController::DiscardItemServer_Implementation(const int32 UniqueNum)
{
	// 아이템 스포너로 아이템 스폰
	FItemInfo ItemInfo;
	const bool bIsItemExist = GetPlayerState<ARPGPlayerState>()->GetItemInfo(UniqueNum, ItemInfo);
	if (bIsItemExist == false) return;

	FVector Location = MyCharacter->GetActorLocation();
	SpawnLocation.X += 50.f;
	SpawnLocation.Z += 100.f;
	GetWorld()->GetAuthGameMode<ARPGGameModeBase>()->DropItem(ItemInfo, Location);

	// 장착된 아이템일 경우 장착 해제
	if (GetPlayerState<ARPGPlayerState>()->IsEquippedItem(UniqueNum))
	{
		UnequipItem(UniqueNum);
	}

	// 플레이어 스테이트에서 아이템 제거
	GetPlayerState<ARPGPlayerState>()->DiscardItem(UniqueNum);
	const int32& ItemCount = GetPlayerState<ARPGPlayerState>()->GetItemCount(UniqueNum);
	UpdateItemInfoClient(UniqueNum, ItemCount);
}

/** -------------- 캐릭터 스탯 정보 업데이트 -------------- */

void ARPGPlayerController::DoUpdateCharacterStat()
{
	UpdateCharacterStatServer();
}

void ARPGPlayerController::UpdateCharacterStatServer_Implementation()
{
	UpdateCharacterStat();
}

void ARPGPlayerController::UpdateCharacterStat()
{
	const FStatInfo& Stats = GetPlayerState<ARPGPlayerState>()->GetCurrentCharacterStats();
	if (MyCharacter)
	{
		MyCharacter->InitCharacterStats(Stats);
	}
	UpdateCharacterStatClient(Stats);
}

void ARPGPlayerController::UpdateCharacterStatClient_Implementation(const FStatInfo& Stats)
{
	CallHUDUpdateCharacterStatText(Stats);
}

void ARPGPlayerController::CallHUDUpdateCharacterStatText(const FStatInfo& Stats)
{
	if (RPGHUD)
	{
		RPGHUD->UpdateStatCharacterStatText(Stats);
	}
}

/** -------------- 장착 아이템 스탯 정보 업데이트 -------------- */

void ARPGPlayerController::DoUpdateEquippedItemStat()
{
	UpdateEquippedItemStatServer();
}

void ARPGPlayerController::UpdateEquippedItemStatServer_Implementation()
{
	UpdateEquippedItemStat();
}

void ARPGPlayerController::UpdateEquippedItemStat()
{
	const FStatInfo& EquippedItemStat = GetPlayerState<ARPGPlayerState>()->GetEquippedItemStats();
	UpdateEquippedItemStatClient(EquippedItemStat);
}

void ARPGPlayerController::UpdateEquippedItemStatClient_Implementation(const FStatInfo& Info)
{
	CallHUDUpdateEquippedItemStatText(Info);
}

void ARPGPlayerController::CallHUDUpdateEquippedItemStatText(const FStatInfo& Info)
{
	if (RPGHUD)
	{
		RPGHUD->UpdateStatEquippedItemStatText(Info);
	}
}

/** -------------- 아이템 정보 가져오기 -------------- */

void ARPGPlayerController::GetItemInfoStruct(const int32& UniqueNum)
{
	GetItemInfoStructServer(UniqueNum);
}

void ARPGPlayerController::GetItemInfoStructServer_Implementation(const int32 UniqueNum)
{
	FItemInfo ItemInfo;
	const bool bIsItemExist = GetPlayerState<ARPGPlayerState>()->GetItemInfo(UniqueNum, ItemInfo);
	if (bIsItemExist)
	{
		GetItemInfoStructClient(ItemInfo);
	}
}

void ARPGPlayerController::GetItemInfoStructClient_Implementation(const FItemInfo& Info)
{
	if (RPGHUD)
	{
		RPGHUD->ShowItemStatTextBox(Info);
	}
}

/** -------------- 데미지 수치 위젯 띄우기 -------------- */

void ARPGPlayerController::ReceiveDamageInfo(const FVector_NetQuantize& PopupPosition, const int32& Damage)
{
	CallHUDPopUpDamageWidgetClient(PopupPosition, Damage);
}

void ARPGPlayerController::CallHUDPopUpDamageWidgetClient_Implementation(const FVector_NetQuantize& PopupPosition, const int32 Damage)
{
	if (RPGHUD)
	{
		RPGHUD->PopUpDamageWidget(PopupPosition, Damage);
	}
}

/** -------------- 오른쪽 클릭 : 스킬 타격 위치 지정, 일반 공격 -------------- */

void ARPGPlayerController::RightClick_AttackOrSetAbilityPoint()
{
	if (bIsInventoryOn || MyCharacter == nullptr || MyCharacter->GetStunned()) return;
	if (MyCharacter->IsAnyMontagePlaying() && !MyCharacter->IsNormalAttackMontagePlaying()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->GetCursorHitResultCastAbility();
	}
	else
	{
		MyCharacter->DoNormalAttack();
	}
}

/** -------------- 스킬 Q, W, E, R -------------- */

void ARPGPlayerController::QPressedAction_Cast()
{
	if (bIsInventoryOn || MyCharacter == nullptr || MyCharacter->IsAnyMontagePlaying() || MyCharacter->GetStunned()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else if(MyCharacter->IsAbilityAvailable(EPressedKey::EPK_Q))
	{
		MyCharacter->ReadyToCastAbilityByKey(EPressedKey::EPK_Q);
	}
}

void ARPGPlayerController::WPressedAction_Cast()
{
	if (bIsInventoryOn || MyCharacter == nullptr && MyCharacter->IsAnyMontagePlaying() || MyCharacter->GetStunned()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else if (MyCharacter->IsAbilityAvailable(EPressedKey::EPK_W))
	{
		MyCharacter->ReadyToCastAbilityByKey(EPressedKey::EPK_W);
	}
}

void ARPGPlayerController::EPressedAction_Cast()
{
	if (bIsInventoryOn || MyCharacter == nullptr || MyCharacter->IsAnyMontagePlaying() || MyCharacter->GetStunned()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else if (MyCharacter->IsAbilityAvailable(EPressedKey::EPK_E))
	{
		MyCharacter->ReadyToCastAbilityByKey(EPressedKey::EPK_E);
	}
}

void ARPGPlayerController::RPressedAction_Cast()
{
	if (bIsInventoryOn || MyCharacter == nullptr || MyCharacter->IsAnyMontagePlaying() || MyCharacter->GetStunned()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else if (MyCharacter->IsAbilityAvailable(EPressedKey::EPK_R))
	{
		MyCharacter->ReadyToCastAbilityByKey(EPressedKey::EPK_R);
	}
}

/** -------------- 인벤토리 I -------------- */

void ARPGPlayerController::IPressedAction_ToggleInventory()
{
	if (RPGHUD == nullptr || MyCharacter == nullptr || MyCharacter->IsAnyMontagePlaying() || MyCharacter->GetStunned()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else
	{
		bIsInventoryOn = !bIsInventoryOn;
		RPGHUD->InventoryWidgetToggle(bIsInventoryOn);
	}
}

/** -------------- 스탯 정보 S -------------- */

void ARPGPlayerController::SPressedAction_ToggleStatInfo()
{
	if (RPGHUD == nullptr || MyCharacter == nullptr || MyCharacter->IsAnyMontagePlaying()) return;

	if (MyCharacter->GetAiming())
	{
		MyCharacter->CancelAbility();
	}
	else
	{
		RPGHUD->ToggleStatInfoWidget();
	}
}

/** -------------- 줌 인/아웃 마우스 휠 -------------- */

void ARPGPlayerController::MouseWheelScroll_ZoomInOut(const FInputActionValue& Value)
{
	if (MyCharacter == nullptr) return;

	MyCharacter->CameraZoomInOut(-Value.Get<float>());
}

void ARPGPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARPGPlayerController, MyCharacter);
}
