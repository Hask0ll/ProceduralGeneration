// Fill out your copyright notice in the Description page of Project Settings.


#include "GP_DiamondSquare.h"
#include "FastNoiseWrapper.h"
#include "KismetProceduralMeshLibrary.h"

AGP_DiamondSquare::AGP_DiamondSquare()
{
	PrimaryActorTick.bCanEverTick = true;
	ProceduralMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralMesh"));
	RootComponent = ProceduralMesh;
	
	NoiseGenerator = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("FastNoiseWrapper"));
}

void AGP_DiamondSquare::BeginPlay()
{
	Super::BeginPlay();
	
	NoiseGenerator->SetupFastNoise(
		EFastNoise_NoiseType::Perlin,    // NoiseType
		Seed,                            // Seed
		Frequency,                       // Frequency
		EFastNoise_Interp::Quintic,     // Interpolation
		EFastNoise_FractalType::FBM,    // FractalType
		3,                              // FractalOctaves
		2.0f,                           // FractalLacunarity
		0.5f,                           // FractalGain
		1.0f,                           // CellularJitter
		EFastNoise_CellularDistanceFunction::Euclidean, // CellularDistanceFunction
		EFastNoise_CellularReturnType::Distance // CellularReturnType
	);

	CreateVertices();
	CreateTriangles();
	
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);
	
	ProceduralMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);
	ProceduralMesh->SetMaterial(0, Material);
}

void AGP_DiamondSquare::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGP_DiamondSquare::CreateVertices()
{
	for (int x = 0; x <= iXSize; ++x)
	{
		for (int y = 0; y <= iYSize; ++y)
		{
			float NoiseValue = NoiseGenerator->GetNoise2D(
				(x + NoiseScale) * NoiseGenerator->GetFrequency(),
				(y + NoiseScale) * NoiseGenerator->GetFrequency()
			);
			
			float Height = NoiseValue * ZMultiplier;
            
			Vertices.Add(FVector(x * fScale, y * fScale, Height));
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
