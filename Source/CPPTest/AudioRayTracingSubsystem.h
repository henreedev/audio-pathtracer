#pragma once

#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AcousticGeometryComponent.h"
#include "GameFramework/DefaultPawn.h"
#include "AudioRayTracingSubsystem.generated.h"

class UFrequenSeeAudioComponent;

USTRUCT()
struct FAudioOcclusionParams
{
	GENERATED_BODY()

	// [0…1] fraction of rays blocked
	float OcclusionGain = 1.f;

	// Cutoff frequencies in Hz
	float LowpassCutoff = 20000.f;
	float HighpassCutoff = 20.f;

	// (Optional) early-reflection taps
	struct FEarlyReflectionTap
	{
		float DelaySeconds;
		float Gain;
	};
	TArray<FEarlyReflectionTap> ReflectionTaps;
};

USTRUCT()
struct FActiveSource
{
	GENERATED_BODY()

	TWeakObjectPtr<UFrequenSeeAudioComponent> AudioComp;

	bool operator==(const FActiveSource& Other) const
	{
		return AudioComp == Other.AudioComp;
	}
};

USTRUCT()
struct FPathEnergyResult
{
	GENERATED_BODY()
	float DelaySeconds;
	float Gain;
};


USTRUCT()
struct FSoundPathNode
{
	GENERATED_BODY()
	FVector Position;
	// Surface Normal
	FVector Normal;
	// FVector Direction;
	// Material
	TWeakObjectPtr<UAcousticGeometryComponent> Material;

	// Probability
	float Probability = 0.f;	
};

USTRUCT()
struct FSoundPath
{
	GENERATED_BODY()
	TArray<FSoundPathNode> Nodes;
	
	
};

UCLASS()
class CPPTEST_API UAudioRayTracingSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:

	UAudioRayTracingSubsystem();
	/* --- Geometry registration --- */
	void RegisterGeometry(UAcousticGeometryComponent* Comp)   { Geometry.AddUnique(Comp); }
	void UnregisterGeometry(UAcousticGeometryComponent* Comp) { Geometry.Remove(Comp);   }

	/* --- Source registration helper --- */
	void RegisterSource(UFrequenSeeAudioComponent* InComp);
	void UnRegisterSource(UFrequenSeeAudioComponent* InComp);
	
	/* --- FTickableGameObject overrides --- */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAudioRayTracingSubsystem, STATGROUP_Tickables); }

private:
	void TraceAndApply(FAudioDevice* Device, const FVector& Listener, const FActiveSource& Src) const;
	
	UPROPERTY()  TArray<FActiveSource>              ActiveSources;
	UPROPERTY()  TArray<TWeakObjectPtr<UAcousticGeometryComponent>> Geometry;

	/** How many rays to cast per source every frame (editor‑tweakable) */
	UPROPERTY(EditAnywhere, Category = "Audio|RayTracing")
	int32 NumRays = 32;

	/** Fallback material if a piece of geometry has no explicit material */
	UPROPERTY(EditAnywhere, Category = "Audio|RayTracing")
	TObjectPtr<UAcousticMaterial> DefaultMaterial = nullptr;

	UPROPERTY(VisibleAnywhere)
	ADefaultPawn* Player;

	/** --- BIDIRECTIONAL PATH TRACING METHODS --- */
	void GeneratePath(const AActor* ActorToIgnore, FSoundPath& OutPath) const;
	bool ConnectSubpaths(FSoundPath& ForwardPath, FSoundPath& BackwardPath, FSoundPath& OutPath);
	void GenerateFullPaths(const FActiveSource& Src, TArray<FSoundPath>& OutPaths); 
	FPathEnergyResult EvaluatePath(FSoundPath& Path) const;
	TArray<float> GetEnergyBuffer(FActiveSource& Src) const;
	void UpdateSources();
};