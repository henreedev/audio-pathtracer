// MaterialAcousticProcessor.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "kiss_fftr.h" // the kiss FFT header (UE4 path)
#include "MaterialAcousticProcessor.generated.h"

/// Frequency response curve over N/2+1 bins
USTRUCT(BlueprintType)
struct FFrequencyResponse
{
    GENERATED_BODY()

    /// responses.Num() must == NumFFTpoints/2+1
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics")
    TArray<float> Responses;
};

/// Material curves for absorption α(f), transmission τ(f), scattering σ(f)
USTRUCT(BlueprintType)
struct FMaterialAcousticFD
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics")
    FFrequencyResponse Absorption;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics")
    FFrequencyResponse Transmission;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Acoustics")
    FFrequencyResponse Scattering;
};

/// Outputs after one block pass
USTRUCT(BlueprintType)
struct FAcousticOutputs
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Acoustics")
    TArray<float> Specular;

    UPROPERTY(BlueprintReadOnly, Category="Acoustics")
    TArray<float> Diffuse;

    UPROPERTY(BlueprintReadOnly, Category="Acoustics")
    TArray<float> Transmitted;
};

/**
 *  Applies one FFT-block of frequency-dependent absorption/transmission/scattering.
 */
UCLASS( ClassGroup=(Audio), meta=(BlueprintSpawnableComponent) )
class CPPTEST_API UMaterialAcousticProcessor : public UActorComponent
{
    GENERATED_BODY()

public:
    /** 
     * Performs a single real→complex FFT, per-bin gains, and complex→real IFFT.
     * @param InBuffer  Mono input samples
     * @param Props     Material curves (all Responses arrays must have same length NumBins)
     * @return          Three time-domain output buffers (length == InBuffer.Num())
     */
    UFUNCTION(BlueprintCallable, Category="Acoustics")
    FAcousticOutputs ApplyMaterialFD(const TArray<float>& InBuffer, const FMaterialAcousticFD& Props);
};
