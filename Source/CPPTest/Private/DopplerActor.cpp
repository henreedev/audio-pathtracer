// Fill out your copyright notice in the Description page of Project Settings.


// #include "DopplerActor.h"
#include "CPPTest/Public/DopplerActor.h"

#include <iostream>

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "EngineUtils.h"
#include "GameFramework/DefaultPawn.h"

// Sets default values
ADopplerActor::ADopplerActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create and attach the audio component
	AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
	AudioComponent->SetupAttachment(RootComponent);
	AudioComponent->bAutoActivate = false; // Don't play on spawn unless you want to

	// Add a cube mesh
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	VisualMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeVisualAsset(TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));

	if (CubeVisualAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeVisualAsset.Object);
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	}
}

// Called when the game starts or when spawned
void ADopplerActor::BeginPlay()
{
	Super::BeginPlay();

	Player = nullptr;
	for (TActorIterator<APawn> It(GetWorld()); It; ++It)
	{
		Player = *It;
		break; // Only need the first one
	}

	if (Player)
	{
		UE_LOG(LogTemp, Warning, TEXT("Found first pawn: %s"), *Player->GetName());
	}
	
	if (SoundCue)
	{
		AudioComponent->SetSound(SoundCue);
		AudioComponent->Play();
	}

}

float PausedTime = 0.0f;

// Called every frame
void ADopplerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Check if the player character is valid
	if (Player)
	{
		// Access player character properties or call functions
		FVector PlayerVelocity = Player->GetVelocity();
		FVector PlayerToActor = (GetActorLocation() - Player->GetActorLocation()).GetSafeNormal();

		UWorld* World = GetWorld();
		FHitResult Hit;
		FVector Start = GetActorLocation(); // Or camera location, etc.
		FVector End = Player->GetActorLocation(); // Adjust distance as needed
		// End += End - Start;
		if (World)
		{
			FCollisionQueryParams QueryParams;
			QueryParams.AddIgnoredActor(this); // Ignore the actor performing the trace

			bool bHit = World->LineTraceSingleByChannel(
				Hit,
				Start,
				End,
				ECC_Visibility, // Or other collision channel
				QueryParams
			);

			if (bHit) {
				AudioComponent->SetPaused(true);
			} else {
				// Resume playback
				AudioComponent->SetPaused(false);
			}

			DrawDebugLine(
				World,
				Start,
				End,
				bHit ? FColor::Red : FColor::Green,
				false,
				0.1f,
				0,
				1.0f
			);

			if (bHit)
			{
				if (GEngine)
				{
					GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, FString::Printf(TEXT("HIT??")));
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
		
		float SpeedTowardsActor = PlayerVelocity.Dot(PlayerToActor);
		float NewPitch = 1.0f + SpeedTowardsActor * 0.0001f;
		AudioComponent->SetPitchMultiplier(NewPitch);
		UE_LOG(LogTemp, Warning, TEXT("NewPitch: %.2f"), NewPitch);
		std::cout << NewPitch << std::endl;
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, FString::Printf(TEXT("NewPitch: %.2f"), NewPitch));
		}

	}
}

