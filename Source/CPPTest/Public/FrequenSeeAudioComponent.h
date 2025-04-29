// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "FrequenSeeAudioComponent.generated.h"

/**
 * UFrequenSeeAudioComponent is an audio component designed to simulate raycast-based sound propagation
 * and environmental audio interaction. This class enables functionality such as audio raycasting,
 * energy-based volume adjustment, and environment responsiveness, tailored for dynamic audio experiences.
 */
UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class CPPTEST_API UFrequenSeeAudioComponent : public UAudioComponent
{
	GENERATED_BODY()

public:

	UFrequenSeeAudioComponent();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	bool bIsRaycasting = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FrequenSeeAudioComponent")
	int RaycastsPerTick = 5;
	
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

	UPROPERTY(VisibleAnywhere)
	APawn* Player;

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

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	float Timer = 0.0f;
	float OcclusionAttenuation = 1.f;

};
