// Fill out your copyright notice in the Description page of Project Settings.


#include "TerrainChunkManager.h"
#include "Kismet/GameplayStatics.h"


ATerrainChunkManager::ATerrainChunkManager()
{
	PrimaryActorTick.bCanEverTick = true;

	NoiseGenerator = CreateDefaultSubobject<UFastNoiseWrapper>(TEXT("FastNoiseWrapper"));

	MaxVerticesPerChunk = (ChunkSize + 1) * (ChunkSize + 1);
}

void ATerrainChunkManager::InitializeBuffers()
{
	VertexBuffer.Reserve(MaxVerticesPerChunk);
	UVBuffer.Reserve(MaxVerticesPerChunk);
	IndexBuffer.Reserve(ChunkSize * ChunkSize * 6);  // 6 indices par quad
    
	// Initialisation du cache de bruit
	NoiseCache.Reserve(MaxVerticesPerChunk);
}

void ATerrainChunkManager::BeginPlay()
{
	Super::BeginPlay();

	InitializeBuffers();

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
    
	// Utiliser les buffers préalloués
	GenerateOptimizedVertices(ChunkCoord, VertexBuffer, UVBuffer);
	GenerateOptimizedIndices(IndexBuffer);
    
	// Calculer les normales et tangentes
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
		VertexBuffer, 
		IndexBuffer, 
		UVBuffer, 
		Normals, 
		Tangents
	);
    
	// Créer la section de mesh
	Chunk->CreateMeshSection(
		0, 
		VertexBuffer, 
		IndexBuffer, 
		Normals, 
		UVBuffer, 
		TArray<FColor>(), 
		Tangents, 
		true
	);
    
	Chunk->SetMaterial(0, Material);
    
	// Positionner le chunk
	Chunk->SetRelativeLocation(
		FVector(
			ChunkCoord.X * ChunkSize * fScale, 
			ChunkCoord.Y * ChunkSize * fScale, 
			0
		)
	);
    
	ActiveChunks.Add(ChunkCoord, Chunk);
    
	// Vider le cache de bruit après la création du chunk
	ClearNoiseCache();
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

float ATerrainChunkManager::GetCachedNoise(float X, float Y)
{
	FVector2D NoiseKey(X, Y);
	float* CachedValue = NoiseCache.Find(NoiseKey);
    
	if (CachedValue)
	{
		return *CachedValue;
	}
    
	float NoiseValue = NoiseGenerator->GetNoise2D(
		(X + NoiseScale) * NoiseGenerator->GetFrequency(),
		(Y + NoiseScale) * NoiseGenerator->GetFrequency()
	);
    
	NoiseCache.Add(NoiseKey, NoiseValue);
	return NoiseValue;
}

void ATerrainChunkManager::GenerateOptimizedVertices(const FIntPoint& ChunkCoord, TArray<FVector>& OutVertices, TArray<FVector2D>& OutUVs)
{
	OutVertices.Reset(MaxVerticesPerChunk);
	OutUVs.Reset(MaxVerticesPerChunk);
    
	// Utiliser SetNum pour éviter les réallocations
	OutVertices.SetNum(MaxVerticesPerChunk, false);
	OutUVs.SetNum(MaxVerticesPerChunk, false);
    
	// Calculer les offsets du chunk
	const float ChunkOffsetX = ChunkCoord.X * ChunkSize;
	const float ChunkOffsetY = ChunkCoord.Y * ChunkSize;
    
	// Optimisation SIMD pour le traitement par lots
	for (int32 Y = 0; Y <= ChunkSize; Y += 4)
	{
		for (int32 X = 0; X <= ChunkSize; X += 4)
		{
			// Traiter 4x4 vertices à la fois
			for (int32 SubY = 0; SubY < 4 && (Y + SubY) <= ChunkSize; SubY++)
			{
				for (int32 SubX = 0; SubX < 4 && (X + SubX) <= ChunkSize; SubX++)
				{
					const int32 CurrentX = X + SubX;
					const int32 CurrentY = Y + SubY;
					const int32 Index = CurrentX + CurrentY * (ChunkSize + 1);
                    
					const float WorldX = ChunkOffsetX + CurrentX;
					const float WorldY = ChunkOffsetY + CurrentY;
                    
					const float Height = GetCachedNoise(WorldX, WorldY) * ZMultiplier;
                    
					OutVertices[Index] = FVector(CurrentX * fScale, CurrentY * fScale, Height);
					OutUVs[Index] = FVector2D(CurrentX * fUVScale, CurrentY * fUVScale);
				}
			}
		}
	}
}

void ATerrainChunkManager::GenerateOptimizedIndices(TArray<int32>& OutIndices)
{
	// Réinitialiser le buffer d'indices
	const int32 NumIndices = ChunkSize * ChunkSize * 6;
	OutIndices.Reset(NumIndices);
	OutIndices.SetNum(NumIndices, false);
    
	int32 IndexCount = 0;
    
	// Générer les indices par lots de quads
	for (int32 Y = 0; Y < ChunkSize; Y++)
	{
		for (int32 X = 0; X < ChunkSize; X++)
		{
			const int32 Vertex = X + Y * (ChunkSize + 1);
            
			// Triangle 1
			OutIndices[IndexCount++] = Vertex;
			OutIndices[IndexCount++] = Vertex + ChunkSize + 1;
			OutIndices[IndexCount++] = Vertex + 1;
            
			// Triangle 2
			OutIndices[IndexCount++] = Vertex + 1;
			OutIndices[IndexCount++] = Vertex + ChunkSize + 1;
			OutIndices[IndexCount++] = Vertex + ChunkSize + 2;
		}
	}
}

void ATerrainChunkManager::ClearNoiseCache()
{
	NoiseCache.Empty();
}