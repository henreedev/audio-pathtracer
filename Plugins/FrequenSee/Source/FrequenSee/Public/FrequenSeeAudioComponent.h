// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "GameFramework/DefaultPawn.h"
#include "Audio.h"
#include "FrequenSeeAudioComponent.generated.h"

class UFrequenSeeAudioReverbSettings;
class UFrequenSeeAudioOcclusionSettings;

/**
 * UFrequenSeeAudioComponent is an audio component designed to simulate raycast-based sound propagation
 * and environmental audio interaction. This class enables functionality such as audio raycasting,
 * energy-based volume adjustment, and environment responsiveness, tailored for dynamic audio experiences.
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class FREQUENSEE_API UFrequenSeeAudioComponent : public UAudioComponent
{
	GENERATED_BODY()

public:

	UFrequenSeeAudioComponent();

	~UFrequenSeeAudioComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	UFrequenSeeAudioOcclusionSettings* OcclusionSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	UFrequenSeeAudioReverbSettings* ReverbSettings;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	bool bIsRaycasting = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	int RaycastsPerTick = 1500;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	int RaycastBounces = 10;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	float RaycastDistance = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	float DampingFactor = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	float AbsorbtionFactor = 0.0002f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	float AbsorbtionFactorAir = 0.0005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	bool bGenerateReverb = false;

	UPROPERTY(VisibleAnywhere)
	ADefaultPawn* Player;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	float RaycastInterval = 1.0f;
	
	
	
	/*
	 * Returns the energy of an audio ray from this source from the player's perspective.
	 */
	float CastAudioRay(FVector Dir, FVector StartPos, float MaxDistance, int Bounces, float Energy = 1.0f);

	/*
	 * Calculates the attenuation factor based on direct rays and transmittance.
	 */
	float CastDirectAudioRay(FVector Dir, FVector StartPos, float MaxDistance, int Bounces = 1, float Energy = 1.0f, AActor* DirectHitActor = nullptr);

	void DebuggingDrawRay(FVector Start, FVector End, const UWorld* World, FHitResult Hit, bool bHit, int BouncesLeft, float Energy, bool HitPlayer = false);
	/*
	 * Casts audio rays, adjusting the current output volume based on the average sound energy received by the listener.
	 */
	void UpdateSound();

	float GetOcclusionAttenuation() const { return OcclusionAttenuation; }
	TArray<TArray<float>>& GetImpulseResponse() { return ImpulseBuffer; }
	TArray<float>& GetAudioBuffer() { return AudioBuffer; }

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	int FrameCount = 0;
	int AudioBufferNum = 0;

	UFUNCTION(BlueprintCallable)
	void RunScript(const FString& FilePath);

private:
	float Timer = 0.0f;
	float OcclusionAttenuation = 1.f;

	// audio ray tracing sampling bins of float energy values
	// dummy values assuming 44.1kHz sampling rate and 2 seconds of audio
	int SampleRate = 48000;
	int NumChannels = 2;
	float SimulatedDuration = 1.0f;
	float BinDuration = 0.0001f; // 0.1ms
	int NumBins = FMath::CeilToInt(SimulatedDuration / BinDuration);  // = 10,000
	int NumSamples = FMath::CeilToInt(SimulatedDuration * SampleRate);
	// for energy responses per channel
	TArray<TArray<float>> EnergyBuffer;
	// for impulse responses per channel
	TArray<TArray<float>> ImpulseBuffer;
	TArray<float> AudioBuffer;

	void ClearEnergyBuffer();
	void Accumulate(float TimeSeconds, float Value, int32 Channel);
	void ReconstructImpulseResponse();
	void NormalizeImpulseResponse(TArray<float>& IR);
	void GenerateDummyImpulseResponse(TArray<float>& IR);
	// load from text file
	void LoadFloatArray(const FString& FilePath, TArray<float> &Data);
	void SaveArrayToFile(const TArray<float>& Array, const FString& FilePath);
};
