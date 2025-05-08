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
	float TotalLength = 0.0f;
	float EnergyContribution = 0.0f;

	// Use pointers instead of references
	FSoundPathNode* ConnectionNodeForward = nullptr;
	FSoundPathNode* ConnectionNodeBackward = nullptr;
};

UCLASS()
class CPPTEST_API UAudioRayTracingSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	
	
	UAudioRayTracingSubsystem();
	
	/* --- WorldSubsystem overrides --- */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
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
	
	TWeakObjectPtr<APawn> PlayerPawn;
	/** --- BIDIRECTIONAL PATH TRACING METHODS --- */
	void GeneratePath(const AActor* ActorToIgnore, FSoundPath& OutPath) const;
	bool ConnectSubpaths(FSoundPath& ForwardPath, FSoundPath& BackwardPath, FSoundPath& OutPath);
	void GenerateFullPaths(const FActiveSource& Src, TArray<FSoundPath>& ForwardPathsOut, TArray<FSoundPath>& BackwardPathsOut, TArray<FSoundPath>& ConnectedPathsOut); 
	FPathEnergyResult EvaluatePath(FSoundPath& Path) const;
	TArray<float> GetEnergyBuffer(FActiveSource& Src) const;
	void UpdateSources(float DeltaTime);

	/** --- PATH VISUALIZATION METHODS --- */

	/**
	 * Given an FSoundPath, visualizes its entire travel over the course of the given duration.
	 * Does so by calling Unreal's DrawDebugLine for each segment travelled along the path as time passes.
	 * Line segments need to be manually cleaned up later if bPersistent is true.
	 */
	void VisualizePath(const FSoundPath& Path, float Duration, FColor Color, bool bPersistent = true) const;

	/**
	 * Draws a segmented line from Start to End at the given speed. 
	 * Line segments need to be manually cleaned up later if bPersistent is true.
	 */
	float DrawSegmentedLine(const FVector& Start, const FVector& End, float Speed, FColor Color,
	                        float DrawDelay, bool bPersistent = true) const;
	const int DEBUG_RAY_FPS = 60; 
	/**
	 * Given forward, backward, and connected paths, shows the complete process of:
	 * 1. Sending out forward and backward paths
	 * 2. Connecting them at their endpoints -- add a new, different-colored debug line between endpoints
	 * 3. Evaluating their contribution -- redraw paths colored by their energy contribution
	 * Apportions some of the total duration to each step. 
	 */
	void VisualizeBDPT(const TArray<FSoundPath>& ForwardPaths, const TArray<FSoundPath>& BackwardPaths, const TArray<FSoundPath>& ConnectedPaths, float TotalDuration) const; 

	const float VISUALIZE_DURATION = 10.0f;
	float VisualizeTimer = 0.0f;
};