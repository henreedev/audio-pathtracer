#pragma once

#include "CoreMinimal.h"
#include "ConvolutionReverb.h"
#include "UObject/Object.h"
#include "FFTConvolver4.generated.h"

UCLASS(BlueprintType)
class FREQUENSEE_API UFFTConvolver4 : public UObject
{
	GENERATED_BODY()

public:
	// Initializes the convolution reverb with given init data
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void InitializeReverb();

	// Applies an impulse response buffer to the reverb
	UFUNCTION(BlueprintCallable, Category = "Audio")
	void SetImpulseResponse(const TArray<float>& IRBuffer, float SampleRate, int32 NumChannels);

protected:
	// Unique pointer to the internal convolution reverb
	TUniquePtr<Audio::FConvolutionReverb> Reverb;

	// Internal init data
	Audio::FConvolutionReverbInitData InitData;
};