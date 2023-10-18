
#include "GameSystem/WorldGridManagerComponent.h"
#include "GameSystem/EnemySpawner.h"
#include "DataAsset/MapNavDataAsset.h"
#include "Enemy/Character/RPGBaseEnemyCharacter.h"
#include "GameInstance/RPGGameInstance.h"
#include "../RPG.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DrawDebugHelpers.h"

UWorldGridManagerComponent::UWorldGridManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UWorldGridManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	InitWorldGrid();
}

void UWorldGridManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	
	//DrawGrid();
}

void UWorldGridManagerComponent::DrawGrid()
{
	int32 Count = 0;
	while (Count < GridWidthSize * GridLengthSize)
	{
		int32 X = FieldLocations[Count].X;
		int32 Y = FieldLocations[Count].Y;
		if (ObstacleExtraCost[Count] == 0)
			DrawDebugPoint(GetWorld(), FVector(X, Y, FieldHeights[Count] + 10.f), 5.f, FColor::Green, true);
		else if (ObstacleExtraCost[Count] < 8)
			DrawDebugPoint(GetWorld(), FVector(X, Y, FieldHeights[Count] + 10.f), 5.f, FColor::Yellow, true);
		else if (ObstacleExtraCost[Count] < 12)
			DrawDebugPoint(GetWorld(), FVector(X, Y, FieldHeights[Count] + 10.f), 5.f, FColor::Orange, true);
		else
			DrawDebugPoint(GetWorld(), FVector(X, Y, FieldHeights[Count] + 10.f), 5.f, FColor::Red, true);
		Count++;
	}
}

void UWorldGridManagerComponent::InitWorldGrid()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*MapNavDataReference);
	if (AssetData.IsValid())
	{
		MapNavDataAsset = Cast<UMapNavDataAsset>(AssetData.GetAsset());
		
		bMapNavDataUsable = true;

		NavOrigin = MapNavDataAsset->NavOrigin;

		GridDist = MapNavDataAsset->GridDist;
		GridWidthSize = MapNavDataAsset->GridWidthSize;
		GridLengthSize = MapNavDataAsset->GridLengthSize;

		BiasX = MapNavDataAsset->BiasX;
		BiasY = MapNavDataAsset->BiasY;

		FieldLocations = MapNavDataAsset->FieldLocations;
		FieldHeights = MapNavDataAsset->FieldHeights;
		IsMovableArr = MapNavDataAsset->IsMovableArr;
		ObstacleExtraCost = MapNavDataAsset->ExtraCost;

		EnemiesPathCost.Init(0, ObstacleExtraCost.Num());
	}
	else
	{
		ELOG(TEXT("Failed to load nav data asset!"));
		bMapNavDataUsable = false;
	}
}

void UWorldGridManagerComponent::GetAStarPath(const FVector& Start, const FVector& Dest, TArray<FPos>& PathToDest)
{
	double start = FPlatformTime::Seconds();

	PathToDest.Empty();

	const int32 DY = VectorToCoordinatesY(Dest.Y);
	const int32 DX = VectorToCoordinatesX(Dest.X);

	if (CanGo(FPos(DY, DX)) == false)
	{
		ELOG(TEXT("Can not move that point."));
		return;
	}

	const int32 SY = VectorToCoordinatesY(Start.Y);
	const int32 SX = VectorToCoordinatesX(Start.X);

	// 목적지가 현재 위치 보다 높은 곳 인지? (높은 곳이면 true)
	const bool bIsGoingUp = FieldHeights[DY * GridWidthSize + DX] - FieldHeights[SY * GridWidthSize + SX] > 0.f ? true : false;

	FPos StartPos(SY, SX);
	TArray<bool> Visited;
	Visited.Init(false, GridWidthSize * GridLengthSize);

	TArray<int32> Best;
	Best.Init(INT32_MAX, GridWidthSize * GridLengthSize);

	TMap<FPos, FPos> Parent;

	TArray<FAStarNode> HeapArr;
	HeapArr.Heapify();

	FPos DestPos(DY, DX);
	{
		int32 G = 0;
		int32 H = 10 * (abs(DestPos.Y - StartPos.Y) + abs(DestPos.X - StartPos.X));
		HeapArr.HeapPush(FAStarNode{ G + H, G, StartPos });
		Best[StartPos.Y * GridWidthSize + StartPos.X] = G + H;
		Parent.Add(StartPos, StartPos);
	}

	while (HeapArr.Num())
	{
		FAStarNode Node;
		HeapArr.HeapPop(Node);
		const int32 NodeIdx = Node.Pos.Y * GridWidthSize + Node.Pos.X;

		if (Visited[NodeIdx])
			continue;
		if (Best[NodeIdx] < Node.F)
			continue;

		Visited[NodeIdx] = true;

		if (Node.Pos == DestPos)
			break;

		for (int32 Dir = 0; Dir < 8; Dir++)
		{
			FPos NextPos = Node.Pos + Front[Dir];
			const int32 NextIdx = NextPos.Y * GridWidthSize + NextPos.X;

			if (CanGo(NextPos) == false)
				continue;
			if (Visited[NextIdx])
				continue;

			// 현재 그리드와 다음 그리드의 높이 차를 가중치로 부여
			const int32 HeightCost = FMath::TruncToInt(FieldHeights[NodeIdx] - FieldHeights[NextIdx]) * (bIsGoingUp ? 10 : -10);
			const int32 G = Node.G + Cost[Dir] + ObstacleExtraCost[NextIdx] + HeightCost;

			const int32 H = 10 * (abs(DestPos.Y - NextPos.Y) + abs(DestPos.X - NextPos.X));
			if (Best[NextIdx] <= G + H)
				continue;

			Best[NextIdx] = G + H;
			HeapArr.HeapPush(FAStarNode{ G + H, G, NextPos });
			Parent.Add(NextPos, Node.Pos);
		}
	}

	if (Parent.Find(DestPos) == nullptr)
	{
		ELOG(TEXT("Can not move that point."));
		return;
	}

	FPos NextPos = DestPos;
	while (NextPos != *Parent.Find(NextPos))
	{
		PathToDest.Add(FieldLocations[NextPos.Y * GridWidthSize + NextPos.X]);
		NextPos = Parent[NextPos];
	}

	Algo::Reverse(PathToDest);

	double end = FPlatformTime::Seconds();
	//PLOG(TEXT("time : %f"), end - start);

	// 경로 디버그
	/*for (int32 i = 0; i < PathToDest.Num(); i++)
	{
		int32 Y = VectorToCoordinatesY(PathToDest[i].Y);
		int32 X = VectorToCoordinatesY(PathToDest[i].X);
		DrawDebugPoint(GetWorld(), FVector(PathToDest[i].X, PathToDest[i].Y, FieldHeights[Y * GridWidthSize + X] + 10.f), 10.f, FColor::Blue, false, 2.f);
	}*/
}

void UWorldGridManagerComponent::GetAStarPath(const FVector& Start, const FVector& Dest, TArray<FPos>& PathToDest, TArray<int32>& GridIndexArr)
{
	double start = FPlatformTime::Seconds();

	PathToDest.Empty();
	GridIndexArr.Empty();

	const int32 DY = VectorToCoordinatesY(Dest.Y);
	const int32 DX = VectorToCoordinatesX(Dest.X);

	if (CanGo(FPos(DY, DX)) == false)
	{
		ELOG(TEXT("Can not move that point."));
		return;
	}

	const int32 SY = VectorToCoordinatesY(Start.Y);
	const int32 SX = VectorToCoordinatesX(Start.X);

	// 목적지가 현재 위치 보다 높은 곳 인지? (높은 곳이면 true)
	const bool bIsGoingUp = FieldHeights[DY * GridWidthSize + DX] - FieldHeights[SY * GridWidthSize + SX] > 0.f ? true : false;

	FPos StartPos(SY, SX);
	TArray<bool> Visited;
	Visited.Init(false, GridWidthSize * GridLengthSize);

	TArray<int32> Best;
	Best.Init(INT32_MAX, GridWidthSize * GridLengthSize);

	TMap<FPos, FPos> Parent;

	TArray<FAStarNode> HeapArr;
	HeapArr.Heapify();

	FPos DestPos(DY, DX);
	{
		int32 G = 0;
		int32 H = 10 * (abs(DestPos.Y - StartPos.Y) + abs(DestPos.X - StartPos.X));
		HeapArr.HeapPush(FAStarNode{ G + H, G, StartPos });
		Best[StartPos.Y * GridWidthSize + StartPos.X] = G + H;
		Parent.Add(StartPos, StartPos);
	}

	FPos LastVisitedPos;
	float MinDest = 10000.f;
	while (HeapArr.Num())
	{
		FAStarNode Node;
		HeapArr.HeapPop(Node);
		const int32 NodeIdx = Node.Pos.Y * GridWidthSize + Node.Pos.X;

		if (Visited[NodeIdx])
			continue;
		if (Best[NodeIdx] < Node.F)
			continue;

		Visited[NodeIdx] = true;

		if (Node.Pos == DestPos)
			break;

		for (int32 Dir = 0; Dir < 8; Dir++)
		{
			FPos NextPos = Node.Pos + Front[Dir];
			const int32 NextIdx = NextPos.Y * GridWidthSize + NextPos.X;

			if (CanGo(NextPos) == false)
				continue;
			if (Visited[NextIdx])
				continue;

			// 현재 그리드와 다음 그리드의 높이 차를 가중치로 부여
			const int32 HeightCost = FMath::TruncToInt(FieldHeights[NodeIdx] - FieldHeights[NextIdx]) * (bIsGoingUp ? 100.f : -100.f);
			const int32 G = Node.G + Cost[Dir] + ObstacleExtraCost[NextIdx] + EnemiesPathCost[NextIdx] + HeightCost;

			const int32 H = 10 * (abs(DestPos.Y - NextPos.Y) + abs(DestPos.X - NextPos.X));
			if (Best[NextIdx] <= G + H)
				continue;

			Best[NextIdx] = G + H;
			HeapArr.HeapPush(FAStarNode{ G + H, G, NextPos });
			Parent.Add(NextPos, Node.Pos);

			// 목적지에 도달 불가능한 경우를 대비해 목적지에 가장 가까운 그리드를 저장해둠
			if (DestPos.GetDistance(NextPos) < MinDest)
			{
				MinDest = DestPos.GetDistance(NextPos);
				LastVisitedPos = NextPos;
			}
		}
	}

	int32 GridIdx;
	FPos NextPos = Parent.Find(DestPos) ? DestPos : LastVisitedPos;
	while (NextPos != *Parent.Find(NextPos))
	{
		GridIdx = NextPos.Y * GridWidthSize + NextPos.X;
		PathToDest.Add(FieldLocations[GridIdx]);
		GridIndexArr.Add(GridIdx);
		EnemiesPathCost[GridIdx] += 140;

		NextPos = Parent[NextPos];
	}

	Algo::Reverse(PathToDest);

	double end = FPlatformTime::Seconds();
	//PLOG(TEXT("time : %f"), end - start);

	// 경로 디버그
	/*for (int32 i = 0; i < PathToDest.Num(); i++)
	{
		int32 Y = VectorToCoordinatesY(PathToDest[i].Y);
		int32 X = VectorToCoordinatesY(PathToDest[i].X);
		DrawDebugPoint(GetWorld(), FVector(PathToDest[i].X, PathToDest[i].Y, FieldHeights[Y * GridWidthSize + X] + 10.f), 10.f, FColor::Blue, false, 2.f);
	}*/
}

void UWorldGridManagerComponent::ClearEnemiesPathCost(TArray<int32>& GridIndexArr)
{
	for (int32 Idx : GridIndexArr)
	{
		EnemiesPathCost[Idx] = FMath::Max(EnemiesPathCost[Idx] - 140, 0);
	}
}

int32 UWorldGridManagerComponent::VectorToCoordinatesY(const double& VectorComponent)
{
	return FMath::Floor(((VectorComponent - NavOrigin.Y + BiasY) / GridDist) + 0.5f);
}

int32 UWorldGridManagerComponent::VectorToCoordinatesX(const double& VectorComponent)
{
	return FMath::Floor(((VectorComponent - NavOrigin.X + BiasX) / GridDist) + 0.5f);
}

int32 UWorldGridManagerComponent::CoordinatesToVectorY(const int32 Coordinates)
{
	return (GridDist * Coordinates) - BiasY;
}

int32 UWorldGridManagerComponent::CoordinatesToVectorX(const int32 Coordinates)
{
	return (GridDist * Coordinates) - BiasX;
}

bool UWorldGridManagerComponent::CanGo(const FPos& _Pos)
{
	if (_Pos.Y >= 0 && _Pos.Y < GridLengthSize && _Pos.X >= 0 && _Pos.X < GridWidthSize)
	{
		if(IsMovableArr[_Pos.Y * GridWidthSize + _Pos.X]) return true;
	}
	return false;
}

// 캐릭터의 현재 그리드와 이전 그리드 노드와 그 주위 8칸의 통행 가능 여부 설정 (캐릭터 끼리 충돌 방지를 위해)
// true면 가중치 감소, false면 가중치 증가
void UWorldGridManagerComponent::SetGridPassability(const FPos& _Pos, const bool IsPassable)
{
	const int32 Y = VectorToCoordinatesY(_Pos.Y);
	const int32 X = VectorToCoordinatesX(_Pos.X);

	EnemiesPathCost[Y * GridWidthSize + X] = 
		FMath::Max(EnemiesPathCost[Y * GridWidthSize + X] + (IsPassable ? -140 : 140), 0);
	FPos NewPos, CurrentPos{ Y,X };
	for (int8 Idx = 0; Idx < 8; Idx++)
	{
		NewPos = CurrentPos + Front[Idx];
		if (CanGo(NewPos))
		{
			EnemiesPathCost[NewPos.Y * GridWidthSize + NewPos.X] = 
				FMath::Max(EnemiesPathCost[NewPos.Y * GridWidthSize + NewPos.X] + (IsPassable ? -140 : 140), 0);
		}
	}
}

void UWorldGridManagerComponent::DrawScore(const FVector& Location)
{
	int32 Y = VectorToCoordinatesY(Location.Y) - 5;
	int32 X = VectorToCoordinatesX(Location.X) - 5;

	for (int32 i = 0; i < 10; i++)
	{
		for (int32 j = 0; j < 10; j++)
		{
			int32 NewIdx = (Y + i) * GridWidthSize + (X + j);
			if (NewIdx >= 0 && NewIdx < GridWidthSize * GridLengthSize)
			{
				DrawDebugString(GetWorld(),
					FVector(FieldLocations[NewIdx].X, FieldLocations[NewIdx].Y, FieldHeights[NewIdx]),
					FString::Printf(TEXT("%d"), EnemiesPathCost[NewIdx]),
					0,
					FColor::Black,
					0.1f,
					false,
					1.f);
			}
		}
	}
}
