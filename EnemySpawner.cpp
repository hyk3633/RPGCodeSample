
#include "GameSystem/EnemySpawner.h"
#include "GameSystem/WorldGridManagerComponent.h"
#include "GameSystem/EnemyPooler.h"
#include "Enemy/Character/RPGBaseEnemyCharacter.h"
#include "Player/Character/RPGBasePlayerCharacter.h"
#include "DataAsset/MapNavDataAsset.h"
#include "../RPGGameModeBase.h"
#include "../RPG.h"
#include "Components/BoxComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"

AEnemySpawner::AEnemySpawner()
{
	PrimaryActorTick.bCanEverTick = true;

	AreaBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Area Box"));
	SetRootComponent(AreaBox);
	AreaBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	AreaBox->SetCollisionResponseToChannel(ECC_PlayerBody, ECR_Overlap);
}

void AEnemySpawner::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (HasAuthority() == false)
	{
		AreaBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	else
	{
		InitFlowField();

		AreaBox->OnComponentBeginOverlap.AddDynamic(this, &AEnemySpawner::OnAreaBoxBeginOverlap);
		AreaBox->OnComponentEndOverlap.AddDynamic(this, &AEnemySpawner::OnAreaBoxEndOverlap);
	}
}

void AEnemySpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		GetActorBounds(true, SpawnerOrigin, SpawnerExtent);
		if (bLoadingDataAssetSuccessful) SpawnEnemies();
	}
}

void AEnemySpawner::SpawnEnemies()
{
	for (auto Pair : EnemyToSpawnMap)
	{
		AEnemyPooler* EnemyPooler = GetWorld()->SpawnActor<AEnemyPooler>(FVector::ZeroVector, FRotator::ZeroRotator);
		const int8 PoolingNumber = FMath::Clamp(Pair.Value * (Pair.Key == EEnemyType::EET_Boss ? 1 : 3), 1, 15);
		EnemyPooler->CreatePool(PoolingNumber, Pair.Key, WaitingLocation);
		EnemyPoolerMap.Add(StaticCast<int8>(Pair.Key), EnemyPooler);

		for (ARPGBaseEnemyCharacter* Enemy : EnemyPooler->GetEnemyArr())
		{
			if (Pair.Key != EEnemyType::EET_Boss)
			{
				Enemy->DOnDeactivate.AddUFunction(this, FName("AddEnemyToRespawnQueue"));
			}
			Enemy->SetSpawner(this);
		}

		for (int8 Idx = 0; Idx < Pair.Value; Idx++)
		{
			FVector SpawnLocation = GetActorLocation();
			if (Pair.Key != EEnemyType::EET_Boss)
			{
				if (GetSpawnLocation(SpawnLocation) == false)
				{
					ELOG(TEXT("no place to respawn"));
					continue;
				}
			}

			ARPGBaseEnemyCharacter* Enemy = EnemyPooler->GetPooledEnemy();
			if (Enemy)
			{
				Enemy->SetActorRotation(FRotator(0, 180, 0));
				Enemy->ActivateEnemy(SpawnLocation);
			}
		}
	}
}

bool AEnemySpawner::GetSpawnLocation(FVector& SpawnLocation)
{
	FHitResult Hit;
	const FVector Top = FVector(0, 0, 500);
	const FVector Bottom = FVector(0, 0, -500);
	for (int8 I = 0; I < 10; I++)
	{
		SpawnLocation = FMath::RandPointInBox(FBox(SpawnerOrigin - SpawnerExtent, SpawnerOrigin + SpawnerExtent - FVector(100, 100, 0)));
		SpawnLocation.Z = 10.f;

		const int32 Idx = GetConvertCurrentLocationToIndex(SpawnLocation);
		if (IsMovableArr[Idx] == false) continue;

		UKismetSystemLibrary::BoxTraceSingle(
			this,
			SpawnLocation,
			SpawnLocation,
			FVector(100, 100, 300),
			FRotator::ZeroRotator,
			UEngineTypes::ConvertToTraceType(ECC_IsSafeToSpawn),
			false,
			TArray<AActor*>(),
			EDrawDebugTrace::None,
			Hit,
			true
		);

		if (!Hit.bBlockingHit)
		{
			GetWorld()->LineTraceSingleByChannel(Hit, SpawnLocation + Top, SpawnLocation + Bottom, ECC_HeightCheck);
			SpawnLocation.Z = Hit.ImpactPoint.Z;
			return true;
		}
	}
	return false;
}

void AEnemySpawner::InitFlowField()
{
	if (FlowFieldDataReference.Len())
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*FlowFieldDataReference);
		if (AssetData.IsValid())
		{
			UMapNavDataAsset* FlowFieldDA = Cast<UMapNavDataAsset>(AssetData.GetAsset());

			OriginLocation = FlowFieldDA->NavOrigin;

			GridDist = FlowFieldDA->GridDist;
			GridWidth = FlowFieldDA->GridWidthSize;
			GridLength = FlowFieldDA->GridLengthSize;
			TotalSize = GridWidth * GridLength;

			BiasY = FlowFieldDA->BiasY;
			BiasX = FlowFieldDA->BiasX;

			IsMovableArr = FlowFieldDA->IsMovableArr;
			FieldHeights = FlowFieldDA->FieldHeights;

			bLoadingDataAssetSuccessful = true;

			return;
		}
	}
	
	ELOG(TEXT("Failed to load Nav data asset!"));
}

void AEnemySpawner::DrawDebugGrid()
{
	int32 Count = 0;
	while (Count < TotalSize)
	{
		FVector Loc = FVector(OriginLocation.X + (GridDist * (Count % GridWidth)) - BiasX, OriginLocation.Y + (GridDist * (Count / GridWidth)) - BiasY, FieldHeights[Count]);
		Loc.Z += 10.f;
		if (IsMovableArr[Count])
		{
			DrawDebugBox(GetWorld(), Loc, FVector(GridDist, GridDist, 1), FColor::Red, true, -1.f, 0, 1.5f);
		}
		else
		{
			DrawDebugBox(GetWorld(), Loc, FVector(GridDist, GridDist, 1), FColor::Green, true, -1.f, 0, 1.5f);
		}
		Count++;
	}
}

void AEnemySpawner::PlayerDead(ACharacter* PlayerCharacter)
{
	DOnPlayerOut.Broadcast(PlayerCharacter);
	PlayersInArea.Remove(PlayerCharacter);
	ARPGBasePlayerCharacter* RPGPlayerCharacter = Cast<ARPGBasePlayerCharacter>(PlayerCharacter);
	if(RPGPlayerCharacter) RPGPlayerCharacter->DOnPlayerDeath.Clear();
}

void AEnemySpawner::OnAreaBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
	if (PlayerCharacter)
	{
		ARPGBasePlayerCharacter* RPGPlayerCharacter = Cast<ARPGBasePlayerCharacter>(PlayerCharacter);
		if (RPGPlayerCharacter)
		{
			RPGPlayerCharacter->DOnPlayerDeath.AddUFunction(this, FName("PlayerDead"));
		}

		PlayersInArea.Add(PlayerCharacter);
		TargetsFlowVectors.Add(PlayerCharacter, FFlowVector(TotalSize));
	}
}

void AEnemySpawner::OnAreaBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
	if (PlayerCharacter)
	{
		ARPGBasePlayerCharacter* RPGPlayerCharacter = Cast<ARPGBasePlayerCharacter>(PlayerCharacter);
		if (RPGPlayerCharacter) RPGPlayerCharacter->DOnPlayerDeath.Clear();
		
		if (PlayersInArea.Find(PlayerCharacter) != INDEX_NONE)
		{
			PlayersInArea.Remove(PlayerCharacter);
			TargetsFlowVectors.Remove(PlayerCharacter);
		}
	}
	DOnPlayerOut.Broadcast(PlayerCharacter);
}

void AEnemySpawner::AddEnemyToRespawnQueue(EEnemyType Type, ARPGBaseEnemyCharacter* Enemy)
{
	RespawnWaitingQueue.Enqueue(Type);
}

void AEnemySpawner::EnemyRespawn()
{
	if (RespawnWaitingQueue.IsEmpty()) return;

	EEnemyType TypeToRespawn;
	RespawnWaitingQueue.Dequeue(TypeToRespawn);

	const int32 Index = StaticCast<int32>(TypeToRespawn);
	if (EnemyPoolerMap.Contains(Index) == false) return;

	FVector SpawnLocation;
	const bool bIsSafeLocation = GetSpawnLocation(SpawnLocation);
	AEnemyPooler* EnemyPooler = *EnemyPoolerMap.Find(Index);
	ARPGBaseEnemyCharacter* Enemy = EnemyPooler->GetPooledEnemy();
	if (bIsSafeLocation)
	{
		if (Enemy)
		{
			Enemy->SetActorRotation(FRotator(0, 180, 0));
			Enemy->ActivateEnemy(SpawnLocation);
		}
	}
	else
	{
		RespawnWaitingQueue.Enqueue(TypeToRespawn);
		EnemyPooler->AddDeactivatedNum();
		if (Enemy) Enemy->RespawnDelay();
	}
}

void AEnemySpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

FVector* AEnemySpawner::GetFlowVector(ACharacter* TargetCharacter, ACharacter* EnemyCharacter)
{
	const int32 Idx = GetConvertCurrentLocationToIndex(EnemyCharacter->GetActorLocation());
	FFlowVector* FlowVector = TargetsFlowVectors.Find(TargetCharacter);
	if (FlowVector) return &FlowVector->GridFlows[Idx];
	else return nullptr;
}

FPos AEnemySpawner::LocationToPos(const FVector& Location)
{
	const int32 DY = FMath::Floor(((Location.Y - OriginLocation.Y + BiasY) / GridDist) + 0.5f);
	const int32 DX = FMath::Floor(((Location.X - OriginLocation.X + BiasX) / GridDist) + 0.5f);

	return FPos(DY, DX);
}

int32 AEnemySpawner::GetConvertCurrentLocationToIndex(const FVector& Location)
{
	FPos Pos = LocationToPos(Location);
	return (Pos.Y * GridWidth + Pos.X);
}

void AEnemySpawner::CalculateFlowVector(ACharacter* TargetCharacter)
{
	//double start = FPlatformTime::Seconds();

	FFlowVector* FVArr = TargetsFlowVectors.Find(TargetCharacter);
	if (FVArr == nullptr) return;

	const int32 DestIdx = GetConvertCurrentLocationToIndex(TargetCharacter->GetActorLocation());
	if (DestIdx >= TotalSize) return;

	// 다른 함수 찾기
	FVArr->Score.Init(-1, TotalSize);

	int32* DistScore = FVArr->Score.GetData();
	FVector* FlowVector = FVArr->GridFlows.GetData();
	if (DistScore == nullptr || FlowVector == nullptr) return;

	*(DistScore + DestIdx) = 0;

	TQueue<FPos> Next;
	Next.Enqueue(FPos(DestIdx / GridWidth, DestIdx % GridWidth));

	// 그리드에 목적지로 부터 증가하는 점수 부여
	while (!Next.IsEmpty())
	{
		FPos CPos;
		Next.Dequeue(CPos);

		DrawDebugString(GetWorld(),
			FVector(OriginLocation.X + (GridDist * CPos.X) - BiasX, OriginLocation.Y + (GridDist * CPos.Y) - BiasY, FieldHeights[(CPos.Y * GridWidth + CPos.X)] + 10.f),
			FString::Printf(TEXT("%d"), *(DistScore + CPos.Y * GridWidth + CPos.X)), nullptr, FColor::Black, 1.f);

		for (int8 Idx = 0; Idx < 8; Idx++)
		{
			FPos NextPos = CPos + Front[Idx];
			const int32 NextIdx = NextPos.Y * GridWidth + NextPos.X;
			if (NextPos.Y >= TopLeftPos.Y && NextPos.Y <= BottomRightPos.Y &&
				NextPos.X >= BottomRightPos.X && NextPos.X <= TopLeftPos.X)
			{
				if (*(DistScore + NextIdx) != -1) continue;
				if (IsMovableArr[NextIdx] == false) continue;
				const int32 HeightScore = FMath::Abs(FMath::TruncToInt(FieldHeights[NextIdx] - FieldHeights[DestIdx]));

				*(DistScore + NextIdx) = *(DistScore + (CPos.Y * GridWidth + CPos.X)) + 1 + HeightScore;
				Next.Enqueue(NextPos);
			}
		}
	}

	// 각 그리드의 인접 그리드 중 가장 점수가 작은 그리드로 향하는 방향 벡터 계산
	for (int32 CY = 0; CY < GridLength; CY++)
	{
		for (int32 CX = 0; CX < GridWidth; CX++)
		{
			if (CY * GridWidth + CX == DestIdx) continue;
			bool Flag = false;
			int32 Min = INT_MAX, Idx = 0;
			for (int8 Dir = 0; Dir < 8; Dir++)
			{
				FPos NextPos = FPos(CY, CX) + Front[Dir];
				if (NextPos.Y >= TopLeftPos.Y && NextPos.Y <= BottomRightPos.Y &&
					NextPos.X >= BottomRightPos.X && NextPos.X <= TopLeftPos.X)
				{
					int32 NextIdx = NextPos.Y * GridWidth + NextPos.X;
					if (Min > *(DistScore + NextIdx) && *(DistScore + NextIdx) >= 0)
					{
						Min = *(DistScore + NextIdx);
						Idx = Dir;
					}
				}
			}

			// 현재 그리드
			FVector Loc = FVector(OriginLocation.X + (GridDist * CX) - BiasX, OriginLocation.Y + (GridDist * CY) - BiasY, 0);
			// 인접한 그리드 중 가장 값이 작은 그리드
			FVector Loc2 = FVector(OriginLocation.X + (GridDist * (CX + Front[Idx].X)) - BiasX, OriginLocation.Y + (GridDist * (CY + Front[Idx].Y)) - BiasY, 0);
			*(FlowVector + (CY * GridWidth + CX)) = (Loc2 - Loc).GetSafeNormal();

			if (Min != INT_MAX)
			{
				FVector MidPoint = 0.5f * (Loc + Loc2);
				FVector HalfwayPoint = MidPoint + (Loc2 - MidPoint) * 0.5f;
				Loc.Z = FieldHeights[(CY + Front[Idx].Y) * GridWidth + (CX + Front[Idx].X)] + 10.f;
				HalfwayPoint.Z = FieldHeights[(CY + Front[Idx].Y) * GridWidth + (CX + Front[Idx].X)] + 10.f;
				DrawDebugDirectionalArrow(GetWorld(), Loc, HalfwayPoint, 20, FColor::Blue, false, 1.f, 0, 1.5f);
			}
		}
	}

	//double end = FPlatformTime::Seconds();
	//PLOG(TEXT("time : %f"), end - start);
}
