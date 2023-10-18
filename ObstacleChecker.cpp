
#include "GameSystem/ObstacleChecker.h"
#include "DataAsset/MapNavDataAsset.h"
#include "Components/BoxComponent.h"
#include "../RPG.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

AObstacleChecker::AObstacleChecker()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("Area Box"));
	SetRootComponent(BoxComponent);
	BoxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoxComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);

	AssetPath = TEXT("/Game/_Assets/DataAsset/");
}

void AObstacleChecker::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (CheckMode != ECheckMode::ECM_None && !CheckAssetValidity())
		{
			InitMapSpecification();
			InitFieldLocations();
			bCheckGrid = true;
		}
	}
}

bool AObstacleChecker::CheckAssetValidity()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(AssetPath + AssetName + TEXT(".") + AssetName);

	if (AssetData.IsValid())
	{
		UMapNavDataAsset* Asset = Cast<UMapNavDataAsset>(AssetData.GetAsset());
		PLOG(TEXT("AssetData [ %s ] is already exist."), *AssetName);
		return true;
	}
	else
	{
		PLOG(TEXT("AssetData [ %s ] is not exist."), *AssetName);
	}
	return false;
}

// 변수 초기화
void AObstacleChecker::InitMapSpecification()
{
	GetActorBounds(true, Origin, Extent);

	GridWidthSize = FMath::TruncToInt((Extent.X * 2.f) / (float)GridDist) + 0.5f;  // 그리드 세로 개수
	GridLengthSize = FMath::TruncToInt((Extent.Y * 2.f) / (float)GridDist) + 0.5f; // 그리드 가로 개수
	TotalSize = GridWidthSize * GridLengthSize;

	BiasX = FMath::TruncToInt(((GridWidthSize * GridDist) / 2.f) - (GridDist / 2.f)); // 보정값
	BiasY = FMath::TruncToInt(((GridLengthSize * GridDist) / 2.f) - (GridDist / 2.f));
}

// 영역 내의 모든 그리드 위치 값 저장
void AObstacleChecker::InitFieldLocations()
{
	if (CheckMode == ECheckMode::ECM_ObstacleCheck)
	{
		FieldLocations.Init(FPos(), TotalSize); 	// 그리드 위치
		FieldHeights.Init(0.f, TotalSize); 		// 그리드 높이
		IsMovableArr.Init(false, TotalSize); 		// 그리드의 장애물 여부
		ExtraCost.Init(0, TotalSize); 			// 각 그리드에 부여된 가중치
		BlockedGrids.Reserve(TotalSize); 		// 통행 불가로 간주된 그리드

		for (int i = 0; i < GridLengthSize; i++)
		{
			for (int j = 0; j < GridWidthSize; j++)
			{
				int32 Y = Origin.Y + ((GridDist * i) - BiasY);
				int32 X = Origin.X + ((GridDist * j) - BiasX);

				FieldLocations[i * GridWidthSize + j] = FPos(Y, X);
			}
		}

		bCheckObstacle = true;
	}
	else if(CheckMode == ECheckMode::ECM_FlowFieldCheck)
	{
		FieldHeights.Init(0.f, TotalSize);
		IsMovableArr.Init(false, TotalSize);

		bCheckFlowField = true;
	}
}

void AObstacleChecker::DrawGridPointInBox()
{
	int32 Count = 0;
	while (Count < TotalSize)
	{
		int32 X = FieldLocations[Count].X;
		int32 Y = FieldLocations[Count].Y;
		DrawDebugPoint(GetWorld(), FVector(X, Y, 10.f), 5.f, FColor::Green, true);
		Count++;
	}
}

void AObstacleChecker::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (bCheckGrid)
		{
			CheckGridSequentially(DeltaTime);
		}
		else if (bGiveScore)
		{
			GiveExtraScoreToGrid(DeltaTime);
		}
	}
}

// 그리드를 0.05초당 200개씩 검사
void AObstacleChecker::CheckGridSequentially(float DeltaTime)
{
	CumulatedTime += DeltaTime;
	if (CumulatedTime >= 0.05f)
	{
		if (CheckMode == ECheckMode::ECM_ObstacleCheck)
		{
			if (bCheckObstacle)
			{
				CheckObstacle();
			}
			else if(bCheckHeightDifference)
			{
				CheckHeightDifference();
			}
		}
		else if (CheckMode == ECheckMode::ECM_FlowFieldCheck)
		{
			if (bCheckFlowField)
			{
				CheckFlowFieldData();
			}
			else if (bCheckHeightDifference)
			{
				CheckHeightDifference();
			}
		}
		CumulatedTime = 0.f;
	}
}

// Flow Field 전용 그리드 데이터 생성
void AObstacleChecker::CheckFlowFieldData()
{
	int32 Count = 0;
	FHitResult Hit;
	while (Count++ < 100 && LastIdx < TotalSize)
	{
		FVector Loc = FVector(Origin.X + (GridDist * (LastIdx % GridWidthSize)) - BiasX, Origin.Y + (GridDist * (LastIdx / GridWidthSize)) - BiasY, 500);
		FVector Loc2 = Loc;
		Loc2.Z = -500.f;

		UKismetSystemLibrary::BoxTraceSingle(
			this,
			Loc,
			Loc2,
			FVector(GridDist, GridDist, 0),
			FRotator::ZeroRotator,
			UEngineTypes::ConvertToTraceType(ECC_ObstacleCheck),
			false,
			TArray<AActor*>(),
			EDrawDebugTrace::None,
			Hit,
			true);

		IsMovableArr[LastIdx] = !Hit.bBlockingHit;

		if (Hit.bBlockingHit)
		{
			Loc.Z = 10;
			DrawDebugBox(GetWorld(), Loc, FVector(GridDist, GridDist, 1), FColor::Red, true, -1.f, 0, 2.f);
		}
		else
		{
			GetWorld()->LineTraceSingleByChannel(Hit, Loc, Loc2, ECC_HeightCheck);
			FieldHeights[LastIdx] = Hit.ImpactPoint.Z; // 높이값 저장
			Loc.Z = Hit.ImpactPoint.Z;
			DrawDebugBox(GetWorld(), Loc, FVector(GridDist, GridDist, 1), FColor::Blue, true, -1.f, 0, 2.f);
		}

		LastIdx++;
	}

	if (LastIdx >= TotalSize)
	{
		bCheckFlowField = false;
		bCheckHeightDifference = true;
		CumulatedTime = 0.1f;
		LastIdx = 0;
	}
}

// 각 그리드의 장애물 여부와 높이 값 체크
void AObstacleChecker::CheckObstacle()
{
	int32 Count = 0;
	FHitResult Hit;
	while (Count++ < 200 && LastIdx < TotalSize)
	{
		UKismetSystemLibrary::LineTraceSingle(
			this,
			FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, 1000),
			FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, -1000),
			UEngineTypes::ConvertToTraceType(ECC_ObstacleCheck),
			false,
			TArray<AActor*>(),
			EDrawDebugTrace::None,
			Hit,
			true
		);

		IsMovableArr[LastIdx] = !Hit.bBlockingHit;

		if (Hit.bBlockingHit)
		{
			ExtraCost[LastIdx] = ObstacleCost * 100;
			BlockedGrids.Add(LastIdx);

			DrawDebugPoint(GetWorld(), FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, Hit.ImpactPoint.Z + 10), 7.5f, FColor::Red, true);
		}
		else
		{
			UKismetSystemLibrary::LineTraceSingle(
				this,
				FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, 1000),
				FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, -1000),
				UEngineTypes::ConvertToTraceType(ECC_HeightCheck),
				false,
				TArray<AActor*>(),
				EDrawDebugTrace::None,
				Hit,
				true
			);
			
			FieldHeights[LastIdx] = Hit.ImpactPoint.Z; // 높이값 저장

			if (Hit.bBlockingHit)
			{
				DrawDebugPoint(GetWorld(), FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, Hit.ImpactPoint.Z + 10), 7.5f, FColor::Green, true);
			}
			else
			{
				IsMovableArr[LastIdx] = false;
				DrawDebugPoint(GetWorld(), FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, Hit.ImpactPoint.Z + 10), 7.5f, FColor::Red, true);
			}
		}

		LastIdx++;
	}

	if (LastIdx >= TotalSize)
	{
		bCheckObstacle = false;
		bCheckHeightDifference = true;
		LastIdx = 0;
		CumulatedTime = 0.1f;
	}
}

// 높이 값 차이가 일정 수치 초과인 그리드를 장애물 그리드로 설정
void AObstacleChecker::CheckHeightDifference()
{
	int32 Count = 0;
	while (Count++ < 200 && LastIdx < TotalSize)
	{
		if (IsMovableArr[LastIdx])
		{
			const int32 CY = LastIdx / GridWidthSize, CX = LastIdx % GridWidthSize;
			for (int8 Idx = 0; Idx < 4; Idx++)
			{
				const int32 NextGrid = (CY + Front[Idx].Y) * GridWidthSize + (CX + Front[Idx].X);
				if (NextGrid < 0 || NextGrid >= TotalSize) continue; // 영역 바깥일 경우
				if (IsMovableArr[NextGrid] == false) continue; // 장애물이 있는 그리드인 경우
				if (FieldHeights[NextGrid] - FieldHeights[LastIdx] <= HeightDifferenceLimit) continue; // 인접 그리드와의 높이 차이가 일정 수치 이하인 경우
				if (CheckMode == ECheckMode::ECM_ObstacleCheck) // AStar 데이터
				{
					BlockedGrids.Add(LastIdx);
					ExtraCost[LastIdx] = (ObstacleCost - 2) * 100;
					DrawDebugPoint(GetWorld(), FVector(FieldLocations[LastIdx].X, FieldLocations[LastIdx].Y, FieldHeights[LastIdx] + 15.f), 7.5f, FColor::Cyan, true);
					break;
				}
				else if (CheckMode == ECheckMode::ECM_FlowFieldCheck) // Flow Field 데이터
				{
					IsMovableArr[LastIdx] = false;
					const FVector Loc = FVector(Origin.X + (GridDist * (LastIdx % GridWidthSize)) - BiasX, Origin.Y + (GridDist * (LastIdx / GridWidthSize)) - BiasY, FieldHeights[LastIdx] + 15.f);
					DrawDebugBox(GetWorld(), Loc, FVector(GridDist, GridDist, 1), FColor::Magenta, true, -1.f, 0, 2.f);
					
					for (int8 Adj = 0; Adj < 4; Adj++)
					{
						int32 NextGrid2 = (CY + Front[Adj].Y) * GridWidthSize + (CX + Front[Adj].X);
						if (NextGrid2 < 0 || NextGrid >= TotalSize) continue;
						IsMovableArr[NextGrid2] = false;
						const FVector Loc2 = FVector(Origin.X + (GridDist * (NextGrid2 % GridWidthSize)) - BiasX, Origin.Y + (GridDist * (NextGrid2 / GridWidthSize)) - BiasY, FieldHeights[NextGrid2] + 15.f);
						DrawDebugBox(GetWorld(), Loc2, FVector(GridDist, GridDist, 1), FColor::Magenta, true, -1.f, 0, 2.f);
					}
				}
			}
		}
		LastIdx++;
	}
	PLOG(TEXT("Check height difference is %d percent progressed."), (int)((LastIdx / (float)TotalSize) * 100.f));

	if (LastIdx >= TotalSize)
	{
		bCheckGrid = false;
		bCheckHeightDifference = false;
		LastIdx = 0;
		CumulatedTime = 0.1f;
		if (CheckMode == ECheckMode::ECM_ObstacleCheck)
		{
			bGiveScore = true;
			BlockedSize = BlockedGrids.Num();
		}
		else if (CheckMode == ECheckMode::ECM_FlowFieldCheck)
		{
			CreateMapNavDataAsset();
		}
	}
}

// 장애물 그리드 근처의 인전 그리드에 가중치 부여
void AObstacleChecker::GiveExtraScoreToGrid(float DeltaTime)
{
	CumulatedTime += DeltaTime;
	if (CumulatedTime >= 0.1f)
	{
		int8 Count = 0;
		while (Count++ < 100 && LastIdx < BlockedSize)
		{
			BFS(BlockedGrids[LastIdx++]);
		}

		CumulatedTime = 0.f;

		if (LastIdx >= BlockedSize)
		{
			bCheckGrid = false;
			bGiveScore = false;
			CreateMapNavDataAsset();
		}
	}
}

// BFS 방식으로 가중치 부여
void AObstacleChecker::BFS(int32 GridIdx)
{
	TArray<int32> NextGrid;
	NextGrid.Add(GridIdx);
	int16 CurrentCost = ExtraCost[GridIdx] ? (ExtraCost[GridIdx] / 100) - 2 : ObstacleCost - 2;
	while (!NextGrid.IsEmpty() && CurrentCost >= 4)
	{
		TArray<int32> TempArr;
		for (int32 Idx_L = 0; Idx_L < NextGrid.Num(); Idx_L++)
		{
			int32 CY = NextGrid[Idx_L] / GridWidthSize, CX = NextGrid[Idx_L] % GridWidthSize;
			for (int8 Idx = 0; Idx < 8; Idx++)
			{
				int32 NextIdx = (CY + Front[Idx].Y) * GridWidthSize + (CX + Front[Idx].X);
				if (NextIdx < 0 || NextIdx >= TotalSize) continue;
				if (ExtraCost[NextIdx] / 100 >= CurrentCost) continue;
				
				ExtraCost[NextIdx] = CurrentCost * 100;

				if (CurrentCost > 4) TempArr.Add(NextIdx);
				if (CurrentCost >= 8)
				{
					DrawDebugPoint(GetWorld(), FVector(FieldLocations[NextIdx].X, FieldLocations[NextIdx].Y, FieldHeights[NextIdx] + 10.f), 7.5f, FColor::Orange, true);
				}
				else
				{
					DrawDebugPoint(GetWorld(), FVector(FieldLocations[NextIdx].X, FieldLocations[NextIdx].Y, FieldHeights[NextIdx] + 10.f), 7.5f, FColor::Yellow, true);
				}	
			}
		}
		NextGrid = TempArr;
		CurrentCost -= 2;
	}
}

// 데이터 에셋 생성 후 데이터 값 저장
void AObstacleChecker::CreateMapNavDataAsset()
{
	if (AssetName.Len() == 0)
	{
		ELOG(TEXT("No asset name designated!"));
		return;
	}

	AssetPath += AssetName;

	UPackage* Package = CreatePackage(nullptr, *AssetPath);
	UMapNavDataAsset* NewDataAsset = NewObject<UMapNavDataAsset>
		(
			Package, 
			UMapNavDataAsset::StaticClass(), 
			*AssetName, 
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone
		);

	if (CheckMode == ECheckMode::ECM_ObstacleCheck)
	{
		NewDataAsset->FieldLocations = FieldLocations;
		NewDataAsset->ExtraCost = ExtraCost;
	}

	NewDataAsset->NavOrigin = Origin;
	NewDataAsset->IsMovableArr = IsMovableArr;
	NewDataAsset->FieldHeights = FieldHeights;
	NewDataAsset->GridDist = GridDist;
	NewDataAsset->GridWidthSize = GridWidthSize;
	NewDataAsset->GridLengthSize = GridLengthSize;
	NewDataAsset->BiasX = BiasX;
	NewDataAsset->BiasY = BiasY;
	
	FAssetRegistryModule::AssetCreated(NewDataAsset);
	NewDataAsset->MarkPackageDirty();

	FString FilePath = FString::Printf(TEXT("%s%s%s"), *AssetPath, *AssetName, *FPackageName::GetAssetPackageExtension());
	bool IsSuccess = UPackage::SavePackage(Package, NewDataAsset, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *FilePath);
	if (IsSuccess)
	{
		PLOG(TEXT("DataAsset [ %s ] successfully created!"), *AssetName);
	}
	else
	{
		PLOG(TEXT("Failed to create DataAsset [ %s ]!"), *AssetName);
	}
}


