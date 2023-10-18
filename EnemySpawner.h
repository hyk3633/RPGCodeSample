
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Structs/Pos.h"
#include "Enums/EnemyType.h"
#include "EnemySpawner.generated.h"

class ARPGBaseEnemyCharacter;
class UWorldGridManagerComponent;
class AEnemyPooler;
class UBoxComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerOutDelegate, ACharacter* PlayerCharacter);

USTRUCT()
struct FFlowVector
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FVector> GridFlows;

	UPROPERTY()
	TArray<int32> Score;

	FFlowVector() {}
	FFlowVector(int32 Size) 
	{ 
		GridFlows.Init(FVector(), Size);
		Score.Init(-1, Size);
	}
};

UCLASS()
class RPG_API AEnemySpawner : public AActor
{
	GENERATED_BODY()
	
public:	

	AEnemySpawner();

	FOnPlayerOutDelegate DOnPlayerOut;

protected:

	virtual void PostInitializeComponents() override;

	virtual void BeginPlay() override;

	void SpawnEnemies();

	bool GetSpawnLocation(FVector& SpawnLocation);

	void InitFlowField();

	void DrawDebugGrid();

	UFUNCTION()
	void PlayerDead(ACharacter* PlayerCharacter);

	UFUNCTION()
	void OnAreaBoxBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAreaBoxEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION()
	void AddEnemyToRespawnQueue(EEnemyType Type, ARPGBaseEnemyCharacter* Enemy);

public:

	FORCEINLINE TArray<ACharacter*>& GetPlayersInArea() { return PlayersInArea; };

	void EnemyRespawn();

	virtual void Tick(float DeltaTime) override;

	FVector* GetFlowVector(ACharacter* TargetCharacter, ACharacter* EnemyCharacter);

	FPos LocationToPos(const FVector& Location);

protected:

	int32 GetConvertCurrentLocationToIndex(const FVector& Location);

	void CalculateFlowVector(ACharacter* TargetCharacter);

private:

	UPROPERTY(EditAnywhere)
	UBoxComponent* AreaBox;

	UPROPERTY(EditAnywhere, Category = "Enemy Spawner")
	FString FlowFieldDataReference;

	bool bLoadingDataAssetSuccessful;

	UPROPERTY(EditInstanceOnly, Category = "Enemy Spawner")
	TMap<EEnemyType, int8> EnemyToSpawnMap;

	UPROPERTY()
	TMap<int32, AEnemyPooler*> EnemyPoolerMap;

	UPROPERTY()
	TArray<ACharacter*> PlayersInArea;

	FVector SpawnerOrigin;
	FVector SpawnerExtent;

	FPos TopLeftPos;
	FPos BottomRightPos;

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

	FVector OriginLocation;

	int32 GridDist = 0;
	int32 GridWidth = 0;
	int32 GridLength = 0;
	int32 TotalSize = 0;

	float BiasY = 0;
	float BiasX = 0;

	TArray<bool> IsMovableArr;

	TArray<float> FieldHeights;

	UPROPERTY()
	TMap<ACharacter*, FFlowVector> TargetsFlowVectors;

	float CumulTime = 0.f;

	TQueue<EEnemyType> RespawnWaitingQueue;

	UPROPERTY(EditInstanceOnly, Category = "Enemy Spawner")
	FVector WaitingLocation;
};
