
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

	// ĳ������ ���� �׸���� ���� �׸��� ���� �� ���� 8ĭ�� ���� ���� ���� ���� (ĳ���� ���� �浹 ������ ����)
	void SetGridPassability(const FPos& _Pos, const bool IsPassable);

	void DrawScore(const FVector& Location);

private:

	// �̵� ���� ����
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

	// ���� �� ����ġ
	int16 Cost[8] = { 100,100,100,100,140,140,140,140 };

	// ������̼� �߽� ��ġ
	FVector NavOrigin;

	// �׸��� ����Ʈ �� �Ÿ�
	int32 GridDist = 0;

	// �׸��� ����,���� ����
	int32 GridWidthSize = 0;
	int32 GridLengthSize = 0;

	// �׸��� - ���� ��ȯ ���̾
	int32 BiasX;
	int32 BiasY;

	// �׸��� ����Ʈ �迭
	TArray<FPos> FieldLocations;

	// �׸��� ����
	TArray<float> FieldHeights;

	// �ش� �ε��� �׸����� ��ֹ� ����
	TArray<bool> IsMovableArr;

	// ��ֹ� ȸ�Ǹ� ���� �߰� ����ġ
	TArray<int16> ObstacleExtraCost;

	// ���� ��� ����� ���� ����ġ
	TArray<int16> EnemiesPathCost;

	UPROPERTY()
	UMapNavDataAsset* MapNavDataAsset;

	UPROPERTY(EditAnywhere, Category = "MapNavData")
	FString MapNavDataReference;

	bool bMapNavDataUsable = false;
};
