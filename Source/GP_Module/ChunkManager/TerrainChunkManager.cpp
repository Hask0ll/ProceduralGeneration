// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainChunkManager.h"
#include "Kismet/GameplayStatics.h"


ATerrainChunkManager::ATerrainChunkManager()
{
	PrimaryActorTick.bCanEverTick = true;

	NoiseGenerator = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("FastNoiseWrapper"));
}

void ATerrainChunkManager::BeginPlay()
{
	Super::BeginPlay();

	// Configuration du générateur de bruit
	NoiseGenerator->SetupFastNoise(
		EFastNoise_NoiseType::Perlin,
		Seed,
		Frequency,
		EFastNoise_Interp::Quintic,
		EFastNoise_FractalType::FBM,
		3,
		2.0f,
		0.5f,
		1.0f,
		EFastNoise_CellularDistanceFunction::Euclidean,
		EFastNoise_CellularReturnType::Distance
	);

	UpdateChunks();
}

// Called every frame
void ATerrainChunkManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Obtenir la position du joueur
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	FVector PlayerLocation = PC->GetPawn()->GetActorLocation();
	FIntPoint NewPlayerChunk = WorldToChunkCoord(PlayerLocation);

	if (NewPlayerChunk != CurrentPlayerChunk)
	{
		CurrentPlayerChunk = NewPlayerChunk;
		UpdateChunks();
	}
}

void ATerrainChunkManager::UpdateChunks()
{
	TSet<FIntPoint> ChunksToKeep;
    
	for (int32 X = -RenderDistance; X <= RenderDistance; X++)
	{
		for (int32 Y = -RenderDistance; Y <= RenderDistance; Y++)
		{
			FIntPoint ChunkCoord(
				CurrentPlayerChunk.X + X,
				CurrentPlayerChunk.Y + Y
			);
            
			if (IsChunkInRange(ChunkCoord))
			{
				ChunksToKeep.Add(ChunkCoord);
				if (!ActiveChunks.Contains(ChunkCoord))
				{
					CreateChunk(ChunkCoord);
				}
			}
		}
	}

	TArray<FIntPoint> ChunksToRemove;
	for (auto& Pair : ActiveChunks)
	{
		if (!ChunksToKeep.Contains(Pair.Key))
		{
			ChunksToRemove.Add(Pair.Key);
		}
	}

	for (const FIntPoint& ChunkCoord : ChunksToRemove)
	{
		RemoveChunk(ChunkCoord);
	}
}

void ATerrainChunkManager::CreateChunk(const FIntPoint& ChunkCoord)
{
    UProceduralMeshComponent* Chunk = NewObject<UProceduralMeshComponent>(this);
    Chunk->RegisterComponent();
    
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;

    // Générer les vertices
    for (int32 X = 0; X <= ChunkSize; X++)
    {
        for (int32 Y = 0; Y <= ChunkSize; Y++)
        {
            float WorldX = (ChunkCoord.X * ChunkSize) + X;
            float WorldY = (ChunkCoord.Y * ChunkSize) + Y;
            
            float NoiseValue = NoiseGenerator->GetNoise2D(
                (WorldX + NoiseScale) * NoiseGenerator->GetFrequency(),
                (WorldY + NoiseScale) * NoiseGenerator->GetFrequency()
            );
            
            float Height = NoiseValue * ZMultiplier;
            
            Vertices.Add(FVector(X * fScale, Y * fScale, Height));
            UVs.Add(FVector2D(X * fUVScale, Y * fUVScale));
        }
    }

    // Générer les triangles
    for (int32 Y = 0; Y < ChunkSize; Y++)
    {
        for (int32 X = 0; X < ChunkSize; X++)
        {
            int32 Vertex = X + Y * (ChunkSize + 1);
            
            Triangles.Add(Vertex);
            Triangles.Add(Vertex + ChunkSize + 1);
            Triangles.Add(Vertex + 1);
            
            Triangles.Add(Vertex + 1);
            Triangles.Add(Vertex + ChunkSize + 1);
            Triangles.Add(Vertex + ChunkSize + 2);
        }
    }

    // Calculer les normales et tangentes
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);

    // Créer la section de mesh
    Chunk->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, TArray<FColor>(), Tangents, true);
    Chunk->SetMaterial(0, Material);

    // Positionner le chunk
    Chunk->SetRelativeLocation(FVector(ChunkCoord.X * ChunkSize * fScale, ChunkCoord.Y * ChunkSize * fScale, 0));
    
    ActiveChunks.Add(ChunkCoord, Chunk);
}

void ATerrainChunkManager::RemoveChunk(const FIntPoint& ChunkCoord)
{
	if (UProceduralMeshComponent* Chunk = ActiveChunks[ChunkCoord])
	{
		Chunk->DestroyComponent();
		ActiveChunks.Remove(ChunkCoord);
	}
}

bool ATerrainChunkManager::IsChunkInRange(const FIntPoint& ChunkCoord)
{
	int32 DistanceX = FMath::Abs(ChunkCoord.X - CurrentPlayerChunk.X);
	int32 DistanceY = FMath::Abs(ChunkCoord.Y - CurrentPlayerChunk.Y);
	return FMath::Max(DistanceX, DistanceY) <= RenderDistance;
}

FIntPoint ATerrainChunkManager::WorldToChunkCoord(const FVector& WorldLocation)
{
	return FIntPoint(
		FMath::Floor(WorldLocation.X / ChunkSize),
		FMath::Floor(WorldLocation.Y / ChunkSize)
	);
}
