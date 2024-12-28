// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "GP_DiamondSquare.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class GP_MODULE_API AGP_DiamondSquare : public AActor
{
	GENERATED_BODY()

	AGP_DiamondSquare();

	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	int iXSize = 0;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	int iYSize = 0;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	float ZMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0))
	float NoiseScale = 1.0f;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.000001))
	float fScale = 0.f;

	UPROPERTY(EditAnywhere, Meta = (ClampMin = 0.000001))
	float fUVScale = 0.0f;
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere)
	UMaterialInterface* Material;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override; 

private:
	UProceduralMeshComponent* ProceduralMesh;
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	
	void CreateVertices();
	void CreateTriangles();
};
