// Fill out your copyright notice in the Description page of Project Settings.


#include "FrequenSeeAudioComponent.h"

#include "AudioRayTracingSubsystem.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "FrequenSeeAudioOcclusionSettings.h"
#include "FrequenSeeAudioReverbSettings.h"
#include "GameFramework/DefaultPawn.h"
#include "TimerManager.h"
#include "Components/SphereComponent.h"


UFrequenSeeAudioComponent::UFrequenSeeAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true; // Make sure it activates and starts ticking
	this->FadeOut(5.0f, /*FadeVolumeLevel=*/0.0f);

	bOverrideAttenuation = true;

	// Create new instance of your occlusion settings object
	// OcclusionSettings = CreateDefaultSubobject<UFrequenSeeAudioOcclusionSettings>(TEXT("OcclusionSettings"));

	// Assign the new instance to the occlusion plugin settings array
	// AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Empty();
	// AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Add(OcclusionSettings);
	
	// You can also tweak other settings here if needed
	AttenuationOverrides.bEnableOcclusion = true;

	// ReverbSettings = CreateDefaultSubobject<UFrequenSeeAudioReverbSettings>(TEXT("ReverbSettings"));

	// AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Empty();
	// AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Add(ReverbSettings);

	// EnergyBuffer.SetNum(NumChannels);
	ImpulseBuffer.SetNum(NumChannels);
	for (int Channel = 0; Channel < NumChannels; ++Channel)
	{
		ImpulseBuffer[Channel].Init(0.0f, NumSamples);
	}

	// under content
	FString ContentDir = FPaths::ProjectContentDir();
	FString ImpulsePath = ContentDir + TEXT("extracted_audio.txt");
	// FString ImpulsePath = ContentDir + TEXT("ir_44k.txt");
	ImpulseBuffer[0].Init(0.0f, NumSamples);
	ImpulseBuffer[1].Init(0.0f, NumSamples);
	// LoadFloatArray(ImpulsePath, ImpulseBuffer[0]);
	// NormalizeImpulseResponse(ImpulseBuffer[0]);
	// LoadFloatArray(ImpulsePath, ImpulseBuffer[1]);
	// NormalizeImpulseResponse(ImpulseBuffer[1]);
	// LoadFloatArray(FPaths::ProjectSavedDir() + TEXT("input_audio.txt"), AudioBuffer);
	AudioBufferNum++;

	// float MaxVal = 0.0f;
	// for (int32 i = 0; i < LoadedIR.Num(); ++i)
	// {
	// 	MaxVal = FMath::Max(MaxVal, FMath::Abs(LoadedIR[i]));
	// }
	// for (int32 i = 0; i < LoadedIR.Num(); ++i)
	// {
	// 	LoadedIR[i] /= MaxVal;
	// };

}

UFrequenSeeAudioComponent::~UFrequenSeeAudioComponent()
{
	// delete AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray[0];
	// delete AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray[0];
	// AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Empty();
	// AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Empty();
}

void UFrequenSeeAudioComponent::OnRegister()
{
	Super::OnRegister();
	if (UWorld const* World = GetWorld())
	{
		if (UAudioRayTracingSubsystem* SubSys = World->GetSubsystem<UAudioRayTracingSubsystem>())
		{
			SubSys->RegisterSource(this);
		}
	}
}

void UFrequenSeeAudioComponent::OnUnregister()
{
	if (UWorld const* World = GetWorld())
	{
		if (UAudioRayTracingSubsystem* SubSys = World->GetSubsystem<UAudioRayTracingSubsystem>())
		{
			SubSys->UnRegisterSource(this);
		}
	}
	Super::OnUnregister();
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

	if (!OcclusionSettings.IsValid())
	{
		OcclusionSettings = NewObject<UFrequenSeeAudioOcclusionSettings>(this);
	}

	if (!ReverbSettings.IsValid())
	{
		ReverbSettings = NewObject<UFrequenSeeAudioReverbSettings>(this);
	}

	// Assign the new instance to the occlusion plugin settings array
	AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Empty();
	AttenuationOverrides.PluginSettings.OcclusionPluginSettingsArray.Add(OcclusionSettings.Get());
	AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Empty();
	AttenuationOverrides.PluginSettings.ReverbPluginSettingsArray.Add(ReverbSettings.Get());
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

	FrameCount++;
	// UE_LOG(LogTemp, Warning, TEXT("FRAME DELTA: %f"), DeltaTime);

	// UE_LOG(LogTemp, Warning, TEXT("Ticking FROMCOMP"));
	// Cast rays at only certain intervals
	// Timer -= DeltaTime;
	// if (Timer <= 0.0f)
	// {
	// 	// Reset timer
	// 	Timer = RaycastInterval;
	// 	
	// 	// Cast rays and update this source's volume
	// 	UpdateSound();
	// }
	UpdateSound();
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
	// temporarily disable debug drawing
	return;
	
	
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
			float DistanceLeft = MaxDistance - Hit.Distance;
			FVector DirToPlayer = Player->GetActorLocation() - Hit.ImpactPoint;
			float DistanceToPlayer = DirToPlayer.Size();
			// reducing scale by 100x
			float TravelTime = (RaycastDistance - DistanceLeft + DistanceToPlayer) * 0.01f / 343.0f;
			if (TravelTime > SimulatedDuration)
			{
				return 0.0f;
			}
		
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
				// Energy *= DampingFactor;
				// Energy = FMath::Clamp(Energy, 0.0f, 1.0f);

				// Direct ray to player - how much does the material absorb, how much continues, how much bounces?
				
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
			float TravelDistance = RaycastDistance - MaxDistance + Hit.Distance;
			// reducing scale by 10x
			TravelDistance *= 0.01f;
			float TravelTime = TravelDistance / 343.0f;
			if (TravelTime > SimulatedDuration)
			{
				return 0.0f;
			}
			// Energy *= 1.0f / std::max(TravelDistance, 1.0f);
			// { 0.0002f, 0.0017f, 0.0182f }
			Energy *= expf(-0.0017f * TravelDistance);
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
		auto RandDir = FMath::VRandCone(FVector(0.0f, -1.0f, 0.0f), PI, PI); // help
		// auto RandDir = FVector(0.0f, -1.0f, 0.0f).RotateAngleAxis(i * 165.0f / RaycastsPerTick + FMath::RandRange(0.0f, 0.5f), FVector(0.0f, 0.0f, 1.0f));
		float Energy = CastAudioRay(RandDir, GetComponentLocation(), RaycastDistance, RaycastBounces);
		TotalEnergy += Energy;
	}
	TotalEnergy /= RaycastsPerTick;

	FVector DirToPlayer = Player->GetActorLocation() - GetComponentLocation();
	DirToPlayer.Normalize();
	OcclusionAttenuation = CastDirectAudioRay(DirToPlayer, GetComponentLocation(), RaycastDistance, 10, 1.0f, GetOwner());
	// UE_LOG(LogTemp, Warning, TEXT("Occlusion attenuation FROMCOMP: %f"), OcclusionAttenuation);
	
	// Set new volume. TODO Update this with more interesting functionality, like doing an IR on the sound buffer
	// SetVolumeMultiplier(FMath::Clamp(TotalEnergy, 0.0f, 1.0f));

	// ReconstructImpulseResponse();
	if (bGenerateReverb)
	{
		SaveArrayToFile(ImpulseBuffer[0], TEXT("saved_ir.txt"));
		RunScript(TEXT("Scripts/test.sh"));
		bGenerateReverb = false;
	}
}

void UFrequenSeeAudioComponent::ClearEnergyBuffer()
{
	for (auto& ChannelBuffer : EnergyBuffer)
	{
		// FMemory::Memset(ChannelBuffer.GetData(), 0, sizeof(float) * ChannelBuffer.Num());
	}
}

void UFrequenSeeAudioComponent::Accumulate(float TimeSeconds, float Value, int32 Channel)
{
	if (Channel >= NumChannels || Channel < 0) return;

	// int32 Index = FMath::FloorToInt(TimeSeconds / BinDuration);
	// if (Index >= 0 && Index < NumBins)
	// {
	// 	EnergyBuffer[Channel][Index] += Value;
	// }
}

void UFrequenSeeAudioComponent::ReconstructImpulseResponse()
{
	// const TArray<float>& EnergyNorms = EnergyBuffer;
	// TArray<float>& ImpulseResponse = ImpulseBuffer[0];
	// ImpulseResponse.SetNumZeroed(ImpulseResponse.Num());
	// const float Pi4 = FMath::Sqrt(4.0f * PI);
	// check(ImpulseResponse.Num() == EnergyNorms.Num());
	// for (int32 i = 0; i < ImpulseResponse.Num(); ++i)
	// {
	// 	float Energy = EnergyNorms[i] / sqrtf(EnergyNorms[i] * Pi4);
	// 	float PrevEnergy = 0.0f;
	// 	if (i == 0)
	// 	{
	// 		PrevEnergy = Energy;
	// 	}
	// 	else if (fabsf(EnergyNorms[i - 1]) >= 0.0f && fabsf(EnergyNorms[i - 1]) >= 0.0f)
	// 	{
	// 		PrevEnergy = EnergyNorms[i - 1] / sqrtf(EnergyNorms[i - 1] * Pi4);
	// 	}
	// 	ImpulseResponse[i] = 0.5 * (PrevEnergy + Energy);
	// }
	// ImpulseResponse[0] = 1.0f;
	// ImpulseBuffer[1] = ImpulseBuffer[0];
	// EnergyBuffer.SetNumZeroed(EnergyBuffer.Num());
	
	constexpr float kEnergyThreshold = 1e-6f;
	const float Pi4 = FMath::Sqrt(4.0f * PI);
	const int32 NumSamplesPerBin = FMath::CeilToInt(BinDuration * SampleRate);
	const TArray<float>& EnergyNorms = EnergyBuffer;
	
	for (int32 Channel = 0; Channel < NumChannels; ++Channel) 
	{
		const TArray<float>& EnergyResponse = EnergyBuffer;
		TArray<float>& ImpulseResponse = ImpulseBuffer[Channel];
		if (ImpulseBuffer[Channel].Num() == 0)
		{
			ImpulseBuffer[Channel].SetNumZeroed(NumSamples);
		}
		FMemory::Memset(ImpulseResponse.GetData(), 0, sizeof(float) * ImpulseResponse.Num());
	 	
		for (int32 Bin = 0; Bin < NumBins; ++Bin) 
		{
			const float EnergyNorm = EnergyNorms[Bin];
			const float NumBinSamples = std::min(NumSamplesPerBin, NumSamples - Bin * NumSamplesPerBin);
			float Normalization = 1.0f;
			float Energy = 0.0f;
			if (fabsf(EnergyResponse[Bin]) >= kEnergyThreshold && fabsf(EnergyNorm) >= kEnergyThreshold)
			{
				Energy = EnergyResponse[Bin] / sqrtf(EnergyNorm * Pi4);
			}
			float PrevEnergy = 0.0f;
			if (Bin == 0)
			{
				PrevEnergy = Energy;
			}
			else if (fabsf(EnergyResponse[Bin - 1]) >= kEnergyThreshold && fabsf(EnergyNorms[Bin - 1]) >= kEnergyThreshold)
			{
				PrevEnergy = EnergyResponse[Bin - 1] / sqrtf(EnergyNorms[Bin - 1] * Pi4);
			}
	
			for (int32 BinSample = 0, Sample = Bin * NumSamplesPerBin; BinSample < NumBinSamples; ++BinSample, ++Sample)
			{
				const float Weight = static_cast<float>(BinSample) / static_cast<float>(NumSamplesPerBin);
				const float SampleEnergy = (1.0f - Weight) * PrevEnergy + Weight * Energy;
	
				ImpulseResponse[Sample] = SampleEnergy;
			}
		}
	
		const float FilterCoefficient = 0.25f; // Tune between (0, 1)
		TArray<float> Filtered;
		Filtered.SetNumUninitialized(ImpulseResponse.Num());
	
		// First sample is unchanged (or initialize as needed)
		Filtered[0] = ImpulseResponse[0];
		for (int32 i = 1; i < ImpulseResponse.Num(); ++i)
		{
			Filtered[i] = FilterCoefficient * ImpulseResponse[i] + (1.0f - FilterCoefficient) * Filtered[i - 1];
		}
	
		ImpulseResponse = MoveTemp(Filtered);
		// NormalizeImpulseResponse(ImpulseResponse);
	}
	

	// test
	// for (int32 Channel = 0; Channel < NumChannels; ++Channel)
	// {
	// 	for (int32 Sample = 44000; Sample < 48000; ++Sample)
	// 	{
	// 		ImpulseBuffer[Channel][Sample] = 1.f;
	// 	}
	// 	for (int32 Sample = 4000; Sample < 44000; ++Sample)
	// 	{
	// 		ImpulseBuffer[Channel][Sample] = 1.5f;
	// 	}
	// 	// Normalize the impulse response
	// 	// NormalizeImpulseResponse(ImpulseBuffer[Channel]);
	// }
	
	// // log sum of energy
	// float EnergySumRight = 0.0f;
	// float EnergySumLeft = 0.0f;
	// for (int32 i = 0; i < EnergyBuffer[0].Num(); ++i)
	// {
	// 	EnergySumRight += EnergyBuffer[0][i];
	// 	EnergySumLeft += EnergyBuffer[1][i];
	// }
	// UE_LOG(LogTemp, Warning, TEXT("Energy sum right: %f, left: %f"), EnergySumRight, EnergySumLeft);
	//
	// // log sum of impulse
	// float ImpulseSumRight = 0.0f;
	// float ImpulseSumLeft = 0.0f;
	// for (int32 i = 0; i < ImpulseBuffer[0].Num(); ++i)
	// {
	// 	ImpulseSumRight += ImpulseBuffer[0][i];
	// 	ImpulseSumLeft += ImpulseBuffer[1][i];
	// }
	// UE_LOG(LogTemp, Warning, TEXT("Impulse sum right: %f, left: %f"), ImpulseSumRight, ImpulseSumLeft);
}

void UFrequenSeeAudioComponent::NormalizeImpulseResponse(TArray<float>& IR)
{
	// float MaxAbs = 0.f;
	// for (float Sample : IR)
	// {
	// 	MaxAbs = FMath::Max(MaxAbs, FMath::Abs(Sample));
	// }
	// // Step 2: Avoid division by zero
	// if (MaxAbs > KINDA_SMALL_NUMBER)
	// {
	// 	for (float& Sample : IR)
	// 	{
	// 		Sample /= MaxAbs;
	// 	}
	// }
	
	// Step 1: Compute L2 norm (sqrt of sum of squares)
	float SumSquares = 0.0f;
	for (float Sample : IR)
	{
		SumSquares += Sample * Sample;
	}
	
	float Norm = FMath::Sqrt(SumSquares);
	
	// UE_LOG(LogTemp, Warning, TEXT("Norm: %f"), Norm);
	
	// IR.Insert(1.0f, IR.Num());
	
	
	// Avoid division by zero
	if (Norm < KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	// Step 2: Normalize each sample
	for (float& Sample : IR)
	{
		Sample /= Norm;
		// Sample = 1.0f; // FIXME
		// if (Sample > 0.0f)
		// {
			// Log samples above 1.0
			// UE_LOG(LogTemp, Warning, TEXT("Sample above 0: %f"), Sample);
			// Sample = 1.0f;
		// }
		// Sample = 0.0f; // FIXME
	}
}

void UFrequenSeeAudioComponent::GenerateDummyImpulseResponse(TArray<float>& IR)
{
	const int32 IrLength = static_cast<int32>(SampleRate * SimulatedDuration);
	const float DirectSoundAttenuation = 0.7f;
	const float EarlyReflectionAttenuation = 0.5f;
	const float DecayFactor = 0.999f;

	// Direct sound + early reflections + exponential decay
	IR[0] = DirectSoundAttenuation;
    
	// Early reflections
	IR[static_cast<int32>(0.005f * SampleRate)] = EarlyReflectionAttenuation * 0.9f;
	IR[static_cast<int32>(0.012f * SampleRate)] = EarlyReflectionAttenuation * 0.7f;
	IR[static_cast<int32>(0.025f * SampleRate)] = EarlyReflectionAttenuation * 0.5f;

	// Exponential decay tail
	float currentDecay = DirectSoundAttenuation;
	for(int32 i = 1; i < IrLength; ++i)
	{
		currentDecay *= DecayFactor;
		IR[i] = FMath::Max(IR[i], currentDecay);
	}

	TArray<float> InvIR;
	InvIR.SetNumUninitialized(IrLength);
	// Invert the impulse response
	// for(int32 i = 0; i < IrLength / 2; ++i)
	// {
	// 	InvIR[i] = IR[IrLength - 1 - i];
	// }

	// Add random small variations (0.5% noise)
	for(int32 i = 0; i < IrLength; ++i)
	{
		IR[i] += FMath::FRandRange(-0.005f, 0.005f);
		IR[i] = FMath::Clamp(IR[i], -1.0f, 1.0f);
		// randomly set to 0
		IR[i] = FMath::FRandRange(0.0f, 1.0f) < 0.9f ? 0.0f : IR[i];
 	}

	// test
	FMemory::Memset(IR.GetData(), 0, sizeof(float) * IR.Num());
	IR[0] = 1.0f;
	IR[IrLength - 1] = 1.0f;
	// FMemory::Memset(IR.GetData() + (IR.Num() - 24000), 1, sizeof(float) * 24000);
	
	// invert original IR
	for(int32 i = 0; i < IrLength / 2; ++i)
	{
		// IR[i] = IR[IrLength - 1 - i];
	}
}

void UFrequenSeeAudioComponent::LoadFloatArray(const FString& FilePath, TArray<float> &Data)
{
	// impulse response in a text file with one float per line
	TArray<float> ImpulseResponse;
	FString FileContent;
	if (FFileHelper::LoadFileToString(FileContent, *FilePath))
	{
		TArray<FString> Lines;
		FileContent.ParseIntoArray(Lines, TEXT("\n"), true);
		for (const FString& Line : Lines)
		{
			float Value = FCString::Atof(*Line);
			ImpulseResponse.Add(Value);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load impulse response file: %s"), *FilePath);
	}

	// validate
	if (ImpulseResponse.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Impulse response file is empty or invalid: %s"), *FilePath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Loaded impulse response file: %s with %d samples"), *FilePath, ImpulseResponse.Num());
	}

	if (ImpulseResponse.Num() != Data.Num())
	{
		Data.SetNumUninitialized(ImpulseResponse.Num());
	}

	FMemory::Memcpy(Data.GetData(), ImpulseResponse.GetData(), sizeof(float) * ImpulseResponse.Num());
}

void UFrequenSeeAudioComponent::SaveArrayToFile(const TArray<float>& Array, const FString& FilePath)
{
	FString Directory = FPaths::ProjectSavedDir(); // You can change this path
	FString FullPath = Directory / FilePath;

	TArray<FString> Lines;
	for (float Value : Array)
	{
		Lines.Add(FString::SanitizeFloat(Value));
	}

	FString FileContent = FString::Join(Lines, TEXT("\n"));

	FFileHelper::SaveStringToFile(FileContent, *FullPath);
}

void UFrequenSeeAudioComponent::RunScript(const FString& FilePath)
{
	FString ScriptPath = FPaths::ProjectDir() / FilePath;
	FString Params = "";

	// void* ReadPipe;
	void* WritePipe = nullptr;
	// FPlatformProcess::CreatePipe(ReadPipe, WritePipe);
	
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*ScriptPath,
		*Params,
		true,   // bLaunchDetached
		false,  // bLaunchHidden
		false,  // bLaunchReallyHidden
		nullptr,
		2,
		nullptr,
		WritePipe
	);

	if (ProcHandle.IsValid())
	{
		// callback for when done
		FPlatformProcess::WaitForProc(ProcHandle);
		FPlatformProcess::CloseProc(ProcHandle);
		// FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
		LoadFloatArray(FPaths::ProjectSavedDir() / TEXT("output_audio.txt"), AudioBuffer);
		AudioBufferNum++;
		UE_LOG(LogTemp, Warning, TEXT("AudioBufferNum = %d"), AudioBufferNum);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to launch script."));
	}
}


