// Fill out your copyright notice in the Description page of Project Settings.


#include "FrequenSeeAudioComponent.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "FrequenSeeAudioOcclusionSettings.h"
#include "FrequenSeeAudioReverbSettings.h"
#include "GameFramework/DefaultPawn.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"


UFrequenSeeAudioComponent::UFrequenSeeAudioComponent()
{
	// PrimaryComponentTick.bCanEverTick = true;
	// bAutoActivate = true; // Make sure it activates and starts ticking
	
	bOverrideAttenuation = true;

	// Create new instance of your occlusion settings object
	OcclusionSettings = CreateDefaultSubobject<UFrequenSeeAudioOcclusionSettings>(TEXT("OcclusionSettings"));

	// Assign the new instance to the occlusion plugin settings array
	AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Empty();
	AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Add(OcclusionSettings);
	
	// You can also tweak other settings here if needed
	AttenuationOverrides.bEnableOcclusion = true;

	ReverbSettings = CreateDefaultSubobject<UFrequenSeeAudioReverbSettings>(TEXT("ReverbSettings"));

	AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Empty();
	AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Add(ReverbSettings);

	EnergyBuffer.SetNum(NumChannels);
	for (int Channel = 0; Channel < NumChannels; ++Channel)
	{
		EnergyBuffer[Channel].Init(0.0f, NumSamples);
	}
}

UFrequenSeeAudioComponent::~UFrequenSeeAudioComponent()
{
	AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Empty();
	AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Empty();
}

// Called when the game starts or when spawned
void UFrequenSeeAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	Player = nullptr;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		Player = Cast<ADefaultPawn>(*It);
		break; // Only need the first one
	}

	// Make player's hitbox bigger
	// Player->GetCollisionComponent()->SetRelativeScale3D(FVector(15.0f, 15.0f, 15.0f));

	// Play this source's sound
	Play();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, FString::Printf(TEXT("BEGUN PLAY")));
	}

	// Activate(); // Force activate this component
	// SetComponentTickEnabled(true);


}

void UFrequenSeeAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Cast rays at only certain intervals
	Timer -= DeltaTime;
	if (Timer <= 0.0f)
	{
		// Reset timer
		Timer = RaycastInterval;
		
		// Cast rays and update this source's volume
		UpdateSound();
	}
}

void DrawDebugLineWrapper(const UWorld* InWorld, 
	FVector const& LineStart, 
	FVector const& LineEnd, 
	FColor const& Color, 
	bool bPersistentLines = false, 
	float LifeTime = -1, 
	uint8 DepthPriority = 0, 
	float Thickness = 0)
{
	
}

void UFrequenSeeAudioComponent::DebuggingDrawRay(FVector Start, FVector End, const UWorld* World, FHitResult Hit, bool bHit, int BouncesLeft, float Energy, bool HitPlayer)
{

	// Start and end colors
	FLinearColor Green = FLinearColor(FColor::Green);
	FLinearColor Red = FLinearColor(FColor::Red);

	// Lerp in HSV space
	FColor HitColor = FLinearColor::LerpUsingHSV(Green, Red, 1 - Energy).ToFColor(true);
	if (HitPlayer)
	{
		HitColor = FColor::White;
	}

	int TotalBounces = RaycastBounces - BouncesLeft;
	const float RayBounceInterval = 0.08f;
	
	FTimerHandle TimerHandle;
	float Delay = TotalBounces * RayBounceInterval;
	
	// World->GetTimerManager().SetTimer(
	// 	TimerHandle,
	// 	FTimerDelegate::CreateLambda([=]()
	// 	{
			DrawDebugLine(
				World,
				Start,
				End,
				bHit ? HitColor : FColor::Purple,
				false,
				RayBounceInterval * 3,
				0,
				1.0f * BouncesLeft
			);
	// 	}),
	// 	Delay == 0 ? 0.01f : Delay,
	// 	false
	// );
	
	if (bHit)
	{
		if (GEngine)
		{
			// GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, FString::Printf(TEXT("HIT??")));
		}
		// Ray hit something
		AActor* HitActor = Hit.GetActor();
		if (HitActor)
		{
			// Do something with the hit actor
			// UE_LOG(LogTemp, Warning, TEXT("Hit actor: %s"), *HitActor->GetName());
		}
	}
	else
	{
		// Ray did not hit anything
		// UE_LOG(LogTemp, Warning, TEXT("Did not hit player."));
	}
}



/*
 * Returns the energy of an audio ray from this source from the player's perspective.
 */
float UFrequenSeeAudioComponent::CastAudioRay(FVector Dir, FVector StartPos, float MaxDistance, int Bounces, float Energy)
{
	if (Bounces == 0 || Energy <= 0.0f)
	{
		return 0.0f;
	}

	// Raycast recursively 
	const FVector Start = StartPos;
	FVector RayDir = Dir.GetSafeNormal();
	bool HitPlayer = false;

	UWorld* World = GetWorld();
	FHitResult Hit;

	// FVector End = FVector(0.1f) + RayDir * MaxDistance; // Adjust distance as needed
	FVector End = Start + RayDir * MaxDistance; // Adjust distance as needed
	if (World)
	{
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(GetOwner()); // Ignore the actor performing the trace

		FCollisionObjectQueryParams ObjectParams;
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
		ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic); // covers walls/floors
		ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic); // covers dynamic props

		bool bHit = World->LineTraceSingleByObjectType(
			Hit,
			Start,
			End,
			ObjectParams,
			QueryParams
		);
		
		
		if (bHit) {
			if (Hit.GetActor()->IsA<ADefaultPawn>())
			{
				// UE_LOG(LogTemp, Warning, TEXT("HIT THE PLAYER!!!"));
				DebuggingDrawRay(Start, Hit.ImpactPoint, World, Hit, bHit, Bounces, Energy, true);
				return Energy;
			} else
			{
				// Did not hit the player;
				// 1. Reduce energy TODO based on bounced material
				auto OldEnergy = Energy;
				Energy *= DampingFactor;
				Energy = FMath::Clamp(Energy, 0.0f, 1.0f);
				float DistanceLeft = MaxDistance - Hit.Distance;
				float TravelTime = (RaycastDistance - DistanceLeft) / 343.0f;
				if (TravelTime > SimulatedDuration)
				{
					return 0.0f;
				}

				// Direct ray to player - how much does the material absorb, how much continues, how much bounces?
				FVector DirToPlayer = Player->GetActorLocation() - Hit.ImpactPoint;
				DirToPlayer.Normalize();
				float DirectEnergy = CastDirectAudioRay(DirToPlayer, Hit.ImpactPoint, DistanceLeft, 1, Energy);
				
				// 2. Calculate new bounce direction and recursively cast a ray using
				// UE_LOG(LogTemp, Warning, TEXT("%f, %f, %f"), Hit.ImpactPoint[0], Hit.ImpactPoint[1], Hit.ImpactPoint[2]);
				FVector NewDir = FMath::GetReflectionVector(RayDir, Hit.ImpactNormal);
				FVector NewStart = Hit.ImpactPoint + Hit.ImpactNormal * 0.5f;

				
				DebuggingDrawRay(Start, NewStart, World, Hit, bHit, Bounces, OldEnergy);
				// UE_LOG(LogTemp, Warning, TEXT("CAST"));

				
				return CastAudioRay(NewDir, NewStart, DistanceLeft, Bounces - 1, Energy);
			}
		} else {
			// Return zero energy
			DebuggingDrawRay(Start, End, World, Hit, bHit, Bounces, Energy);
			return 0.0f;
		}
		
	}

	if (HitPlayer)
	{
		return Energy;		
	} else
	{
		return 0.0f;
	}
}

float UFrequenSeeAudioComponent::CastDirectAudioRay(FVector Dir, FVector StartPos, float MaxDistance, int Bounces,
	float Energy, AActor* DirectHitActor)
{
	if (Bounces == 0 || Energy <= 0.0f)
	{
		return 0.0f;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}
	
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	// hack
	QueryParams.AddIgnoredActor(DirectHitActor);
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic); // covers walls/floors
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic); // covers dynamic props

	FVector DirectStart = StartPos + Dir * 0.1f;
	FVector DirectEnd = DirectStart + Dir * MaxDistance;
		
	bool bDirectHit = World->LineTraceSingleByObjectType(
		Hit,
		DirectStart,
		DirectEnd,
		ObjectParams,
		QueryParams
	);
	DebuggingDrawRay(DirectStart, Hit.ImpactPoint, World, Hit, bDirectHit, Bounces, Energy);

	// NOTE: energy adjustments due to air should be accumulated (the distance rather) and calculated later based on the distance
	if (bDirectHit)
	{
		// moved inside the wall -> currently doesn't work
		if (Hit.GetActor() == DirectHitActor) 
		{
			// Energy *= (1.f - AbsorbtionFactor * fmax(Hit.Distance, 1.0f));
			return CastDirectAudioRay(Dir, Hit.ImpactPoint, MaxDistance - Hit.Distance, Bounces, Energy, Hit.GetActor());
		}
		// hit the player
		if (Hit.GetActor()->IsA<ADefaultPawn>())
		{
			// Energy *= (1.f - AbsorbtionFactorAir * fmax(Hit.Distance, 1.0f));
			// DebuggingDrawRay(DirectStart, Hit.ImpactPoint, World, Hit, bDirectHit, Bounces, Energy, true);
			float TravelTime = (RaycastDistance - MaxDistance + Hit.Distance) / 343.0f;
			if (TravelTime > SimulatedDuration)
			{
				return 0.0f;
			}
			// not sure about channels
			Accumulate(TravelTime, Energy, 0);
			Accumulate(TravelTime, Energy, 1);
			
			return Energy;
		}
		// hit a different obstacle, continue through it if there's distance or bounces
		else 
		{
			// Energy *= (1.f - AbsorbtionFactorAir * fmax(Hit.Distance, 1.0f));
			// DebuggingDrawRay(DirectStart, Hit.ImpactPoint, World, Hit, bDirectHit, Bounces, Energy);
			return CastDirectAudioRay(Dir, Hit.ImpactPoint, MaxDistance - Hit.Distance, Bounces - 1, Energy, Hit.GetActor());
		}
	}
	
	// no hit
	return 0.0f;
}


void UFrequenSeeAudioComponent::UpdateSound()
{
	// not sure
	ClearEnergyBuffer();
	
	float TotalEnergy = 0.0f;
	for (int i = 0; i < RaycastsPerTick; i++)
	{
		// auto RandDir = FMath::VRandCone(FVector(-1.0f, 0.0f, 0.0f), PI / 4.0f, 0.0001f); // help
		auto RandDir = FVector(0.0f, -1.0f, 0.0f).RotateAngleAxis(i * 165.0f / RaycastsPerTick + FMath::RandRange(0.0f, 0.5f), FVector(0.0f, 0.0f, 1.0f));
		float Energy = CastAudioRay(RandDir, GetComponentLocation(), RaycastDistance, RaycastBounces);
		TotalEnergy += Energy;
	}
	TotalEnergy /= RaycastsPerTick;

	FVector DirToPlayer = Player->GetActorLocation() - GetComponentLocation();
	DirToPlayer.Normalize();
	OcclusionAttenuation = CastDirectAudioRay(DirToPlayer, GetComponentLocation(), RaycastDistance, 10, 1.0f, GetOwner());
	
	// Set new volume. TODO Update this with more interesting functionality, like doing an IR on the sound buffer
	SetVolumeMultiplier(FMath::Clamp(TotalEnergy, 0.0f, 1.0f));
}

void UFrequenSeeAudioComponent::ClearEnergyBuffer()
{
	for (auto& ChannelBuffer : EnergyBuffer)
	{
		FMemory::Memset(ChannelBuffer.GetData(), 0, sizeof(float) * ChannelBuffer.Num());
	}
}

void UFrequenSeeAudioComponent::Accumulate(float TimeSeconds, float Value, int32 Channel)
{
	if (Channel >= NumChannels || Channel < 0) return;

	int32 Index = FMath::RoundToInt(TimeSeconds * SampleRate);
	if (Index >= 0 && Index < NumSamples)
	{
		EnergyBuffer[Channel][Index] += Value;
	}
}


