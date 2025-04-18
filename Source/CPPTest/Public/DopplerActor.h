// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DopplerActor.generated.h"

UCLASS()
class CPPTEST_API ADopplerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADopplerActor();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent *VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UAudioComponent* AudioComponent;

	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* SoundCue;

	UPROPERTY(VisibleAnywhere)
	APawn* Player;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
