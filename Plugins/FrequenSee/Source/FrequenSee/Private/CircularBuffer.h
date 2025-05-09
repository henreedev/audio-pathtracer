#pragma once
#include "CoreMinimal.h"


class FCircularAudioBuffer
{
public:
	FCircularAudioBuffer();
	FCircularAudioBuffer(int32 InSize);

	void SetSize(int32 InSize);
	int32 GetSize();
	void AddSample(float Sample);
	void AddSamples(const TArray<float>& NewSamples);
	void AddSamples(const float* NewSamples, int32 Count, int32 Offset = 0, int32 Stride = 1);
	void GetLastSamples(TArray<float>& OutSamples, int32 Count) const;

private:
	TArray<float> Buffer;
	int32 Size;
	int32 Head;  // Points to the next insert index
	bool Filled;
};