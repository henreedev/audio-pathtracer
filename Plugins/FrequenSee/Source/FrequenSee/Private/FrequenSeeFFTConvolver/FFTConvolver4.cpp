#include "FFTConvolver4.h"

void UFFTConvolver4::InitializeReverb()
{
	if (!Reverb.IsValid())
	{
		// You can customize InitData before this if needed
		Reverb = Audio::FConvolutionReverb::CreateConvolutionReverb(InitData);
	}
}

void UFFTConvolver4::SetImpulseResponse(const TArray<float>& IRBuffer, float SampleRate, int32 NumChannels)
{
	if (Reverb.IsValid())
	{
		UE_LOG(LogTemp, Display, TEXT("Reverb SetImpulseResponse"));
	}
}
