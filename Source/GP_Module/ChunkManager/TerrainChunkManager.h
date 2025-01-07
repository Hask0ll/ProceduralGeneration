// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "FastNoiseWrapper.h"
#include "KismetProceduralMeshLibrary.h"
#include "TerrainChunkManager.generated.h"

UCLASS()
class GP_MODULE_API ATerrainChunkManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ATerrainChunkManager();

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	int32 ChunkSize = 100;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	int32 RenderDistance = 3;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	float fScale = 100.0f;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	float fUVScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	float ZMultiplier = 1000.0f;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	float NoiseScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	float Frequency = 0.01f;

	UPROPERTY(EditAnywhere, Category = "Terrain Generation")
	UMaterialInterface* Material;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY()
	UFastNoiseWrapper* NoiseGenerator;

	TMap<FIntPoint, UProceduralMeshComponent*> ActiveChunks;
	FIntPoint CurrentPlayerChunk;

	void UpdateChunks();
	void CreateChunk(const FIntPoint& ChunkCoord);
	void RemoveChunk(const FIntPoint& ChunkCoord);
	bool IsChunkInRange(const FIntPoint& ChunkCoord);
	FIntPoint WorldToChunkCoord(const FVector& WorldLocation);
};
