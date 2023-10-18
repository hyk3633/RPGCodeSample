
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Structs/Pos.h"
#include "WorldGridManagerComponent.generated.h"

class UMapNavDataAsset;
class ARPGBaseEnemyCharacter;
class AEnemySpawner;

USTRUCT()
struct FAStarNode
{
	GENERATED_BODY()

	int32 F;
	int32 G;
	FPos Pos;

	FAStarNode(int32 _F, int32 _G, FPos _Pos) : F(_F), G(_G), Pos(_Pos) {}
	FAStarNode() {}
	bool operator<(const FAStarNode& Other) const { return F < Other.F; }
	bool operator>(const FAStarNode& Other) const { return F > Other.F; }
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_API UWorldGridManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	UWorldGridManagerComponent();

	FORCEINLINE bool GetIsNavigationEnable() const { return bMapNavDataUsable; }

	void DrawGrid();

protected:

	virtual void InitializeComponent() override;
	
	virtual void BeginPlay() override;

	void InitWorldGrid();

public:

	void GetAStarPath(const FVector& Start, const FVector& Dest, TArray<FPos>& PathToDest);

	void GetAStarPath(const FVector& Start, const FVector& Dest, TArray<FPos>& PathToDest, TArray<int32>& GridIndexArr);

	void ClearEnemiesPathCost(TArray<int32>& GridIndexArr);

	int32 VectorToCoordinatesY(const double& VectorComponent);

	int32 VectorToCoordinatesX(const double& VectorComponent);

	int32 CoordinatesToVectorY(const int32 Coordinates);

	int32 CoordinatesToVectorX(const int32 Coordinates);

	bool CanGo(const FPos& _Pos);

	// 캐릭터의 현재 그리드와 이전 그리드 노드와 그 주위 8칸의 통행 가능 여부 설정 (캐릭터 끼리 충돌 방지를 위해)
	void SetGridPassability(const FPos& _Pos, const bool IsPassable);

	void DrawScore(const FVector& Location);

private:

	// 이동 가능 방향
	FPos Front[8] =
	{
		FPos { -1, 0},
		FPos { 0, 1},
		FPos { 1, 0},
		FPos { 0, -1},
		FPos {-1, -1},
		FPos {-1, 1},
		FPos {1, 1},
		FPos {1, -1},
	};

	// 방향 별 가중치
	int16 Cost[8] = { 100,100,100,100,140,140,140,140 };

	// 내비게이션 중심 위치
	FVector NavOrigin;

	// 그리드 포인트 간 거리
	int32 GridDist = 0;

	// 그리드 가로,세로 길이
	int32 GridWidthSize = 0;
	int32 GridLengthSize = 0;

	// 그리드 - 벡터 변환 바이어스
	int32 BiasX;
	int32 BiasY;

	// 그리드 포인트 배열
	TArray<FPos> FieldLocations;

	// 그리드 높이
	TArray<float> FieldHeights;

	// 해당 인덱스 그리드의 장애물 여부
	TArray<bool> IsMovableArr;

	// 장애물 회피를 위한 추가 가중치
	TArray<int16> ObstacleExtraCost;

	// 적의 경로 계산을 위한 가중치
	TArray<int16> EnemiesPathCost;

	UPROPERTY()
	UMapNavDataAsset* MapNavDataAsset;

	UPROPERTY(EditAnywhere, Category = "MapNavData")
	FString MapNavDataReference;

	bool bMapNavDataUsable = false;
};
