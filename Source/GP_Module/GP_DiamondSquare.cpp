// Fill out your copyright notice in the Description page of Project Settings.


#include "GP_DiamondSquare.h"

AGP_DiamondSquare::AGP_DiamondSquare()
{
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
}

void AGP_DiamondSquare::BeginPlay()
{
	Super::BeginPlay();
}

void AGP_DiamondSquare::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void AGP_DiamondSquare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CreateVertices();
	CreateTriangles();
	
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, TArray<FVector>(), UVs, TArray<FColor>(), TArray<FProcMeshTangent>(), true);
	ProceduralMesh->SetMaterial(0, Material);
}

void AGP_DiamondSquare::CreateVertices()
{
	for (int x = 0; x <= iXSize; ++x)
	{
		for (int y = 0; y <= iYSize; ++y)
		{
			float z = FMath::PerlinNoise2D(FVector2D(x + NoiseScale, y + NoiseScale)) * ZMultiplier;
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Perlin Noise: %f"), z));
			Vertices.Add(FVector(x * fScale, y * fScale, z));
			UVs.Add(FVector2D(x * fUVScale, y * fUVScale));
		}
	}
}

void AGP_DiamondSquare::CreateTriangles()
{
	int Vertex = 0;

	for (int x = 0; x < iXSize; ++x)
	{
		for (int y = 0; y < iYSize; ++y)
		{
			Triangles.Add(Vertex);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + iYSize + 1);
			Triangles.Add(Vertex + 1);
			Triangles.Add(Vertex + iYSize + 2);
			Triangles.Add(Vertex + iYSize + 1);

			++Vertex;
		}
		++Vertex;
	}
}
