
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structs/Pos.h"
#include "ObstacleChecker.generated.h"

class UBoxComponent;

UENUM()
enum class ECheckMode : uint8
{
	ECM_None,
	ECM_ObstacleCheck,
	ECM_FlowFieldCheck,

	ECM_MAX
};

UCLASS()
class RPG_API AObstacleChecker : public AActor
{
	GENERATED_BODY()
	
public:	

	AObstacleChecker();

protected:

	virtual void BeginPlay() override;

	bool CheckAssetValidity();

	void InitMapSpecification();

	void InitFieldLocations();

	void DrawGridPointInBox();

public:	

	virtual void Tick(float DeltaTime) override;

protected:

	void CheckFlowFieldData();

	void CheckGridSequentially(float DeltaTime);

	void CheckObstacle();

	void CheckHeightDifference();

	void GiveExtraScoreToGrid(float DeltaTime);

	void BFS(int32 GridIdx);

	void CreateMapNavDataAsset();

private:

	UPROPERTY(EditInstanceOnly)
	UBoxComponent* BoxComponent;

	FVector Origin;
	FVector Extent;

	UPROPERTY(EditInstanceOnly, Category = "Nav Setting", meta = (ClampMin = "5", ClampMax = "500"))
	int32 GridDist = 25;

	int32 GridWidthSize = 0;
	int32 GridLengthSize = 0;
	int32 TotalSize = 0;
	int32 BlockedSize = 0;

	int32 BiasX = 0;
	int32 BiasY = 0;

	float CumulatedTime = 0.1f;

	TArray<FPos> FieldLocations;

	TArray<float> FieldHeights;

	TArray<bool> IsMovableArr;

	TArray<int32> BlockedGrids;

	TArray<int16> ExtraCost;

	int32 LastIdx = 0;

	FString AssetPath;

	UPROPERTY(EditInstanceOnly, Category = "Nav Setting")
	FString AssetName;

	UPROPERTY(EditInstanceOnly, Category = "Nav Setting")
	ECheckMode CheckMode;

	// 그리드 체크 허용
	bool bCheckGrid = false;

	// 장애물 체크
	bool bCheckObstacle = false;

	// 높이 차 체크
	bool bCheckHeightDifference = false;

	// 그리드 가중치 부여
	bool bGiveScore = false;

	// 플로우 필드 채우기
	bool bCheckFlowField = false;

	int16 ObstacleCost = 12;

	int8 HeightDifferenceLimit = 20;

	FPos Front[8] =
	{
		FPos { -1, 0},
		FPos { 0, -1},
		FPos { 1, 0},
		FPos { 0, 1},
		FPos {-1, -1},
		FPos {1, -1},
		FPos {1, 1},
		FPos {-1, 1},
	};
};
