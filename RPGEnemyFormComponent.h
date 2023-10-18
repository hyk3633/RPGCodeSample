

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enums/EnemyType.h"
#include "Enums/EnemyAttackType.h"
#include "Structs/EnemyInfo.h"
#include "Structs/EnemyAssets.h"
#include "RPGEnemyFormComponent.generated.h"

class ARPGBaseEnemyCharacter;
class ARPGEnemyAIController;
class URPGEnemyAnimInstance;
class ARPGBaseProjectile;
class UProjectilePoolerComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class RPG_API URPGEnemyFormComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	
	URPGEnemyFormComponent();

	void InitEnemyFormComponent(EEnemyType Type);

protected:
	
	void GetEnemyInfoAndInitialize(EEnemyType Type);

	void GetEnemyAssetsAndInitialize(EEnemyType Type);

	void CreateProjectilePooler();

public:	

	ARPGBaseEnemyCharacter* CreateNewEnemy(const FVector& WaitingLocation);

protected:

	void InitEnemy(ARPGBaseEnemyCharacter* SpawnedEnemy);

public:
	
	FORCEINLINE const FEnemyInfo& GetEnemyInfo() const { return EnemyInfo; }
	FORCEINLINE const FEnemyAssets& GetEnemyAssets() const { return EnemyAssets; }
	FORCEINLINE EEnemyType GetEnemyType() const { return EnemyType; }

	bool MeleeAttack(ARPGBaseEnemyCharacter* Attacker, FVector& ImpactPoint);

	void RangedAttack(ARPGBaseEnemyCharacter* Attacker, APawn* HomingTarget = nullptr);

protected:

	void GetSocketLocationAndSpawn(ARPGBaseEnemyCharacter* Attacker, APawn* HomingTarget = nullptr);

	void SpawnProjectile(ARPGBaseEnemyCharacter* Attacker, const FVector& SpawnLocation, const FRotator& SpawnRotation, APawn* HomingTarget = nullptr);

public:

	void StraightMultiAttack(ARPGBaseEnemyCharacter* Attacker, const FVector& LocationToAttack, TArray<FVector>& ImpactLocation, TArray<FRotator>& ImpactRotation);
	
	void SphericalRangeAttack(ARPGBaseEnemyCharacter* Attacker, const int32& Radius, TArray<FVector>& ImpactLocation, TArray<FRotator>& ImpactRotation);

	void RectangularRangeAttack(ARPGBaseEnemyCharacter* Attacker, const int32& Length, TArray<FVector>& ImpactLocation, TArray<FRotator>& ImpactRotation);

private:

	FEnemyInfo EnemyInfo;

	FEnemyAssets EnemyAssets;

	EEnemyType EnemyType;

	bool bIsWeaponed = false;

	UPROPERTY()
	UProjectilePoolerComponent* ProjectilePooler;
};
