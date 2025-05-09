#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AcousticMaterial.generated.h"

USTRUCT(BlueprintType)
struct FAcousticBand
{
	GENERATED_BODY()

	// 0…1 scalar per band
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
	float Value = 0.0f;
};

UCLASS(BlueprintType)
class FREQUENSEE_API UAcousticMaterial : public UDataAsset
{
	GENERATED_BODY()

public:
	// Six‑band default (125Hz‑4kHz). Resize if you need more.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics", meta = (FixedSize))
	TArray<FAcousticBand> Absorption { FAcousticBand(), FAcousticBand(), FAcousticBand()};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics", meta = (FixedSize))
	TArray<FAcousticBand> Transmission { Absorption };

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Acoustics", meta = (FixedSize))
	TArray<FAcousticBand> Scattering { Absorption };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acoustics")
	float ThicknessCm = 2.5f;  // in centimeters
};