#pragma once

#include "CoreMinimal.h"
#include "AcousticMaterial.h"
#include "Components/ActorComponent.h"
#include "AcousticGeometryComponent.generated.h"


UCLASS(ClassGroup = (Audio), meta = (BlueprintSpawnableComponent))
class UAcousticGeometryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    // Acoustic data the manager will read each tick
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics")
    TObjectPtr<UAcousticMaterial> Material = nullptr;

protected:
    virtual void OnRegister() override;   // auto‑hook into subsystem
    virtual void OnUnregister() override;
};
