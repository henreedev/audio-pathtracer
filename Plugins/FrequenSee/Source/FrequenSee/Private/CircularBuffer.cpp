#include "CircularBuffer.h"


FCircularAudioBuffer::FCircularAudioBuffer()
	: Size(0), Head(0), Filled(true)
{
}

FCircularAudioBuffer::FCircularAudioBuffer(int32 InSize)
	: Size(InSize), Head(0), Filled(false)
{
	Buffer.SetNumZeroed(Size);
}

void FCircularAudioBuffer::SetSize(int32 InSize)
{
	Buffer.SetNumZeroed(InSize);
	Size = InSize;
	Head = 0;
	Filled = false;
}

int32 FCircularAudioBuffer::GetSize()
{
	return Size;
}

void FCircularAudioBuffer::AddSample(float Sample)
{
	Buffer[Head] = Sample;
	Head = (Head + 1) % Size;
	if (Head == 0) Filled = true;
}

void FCircularAudioBuffer::AddSamples(const TArray<float>& NewSamples)
{
	for (float Sample : NewSamples)
	{
		AddSample(Sample);
	}
}

void FCircularAudioBuffer::AddSamples(const float* NewSamples, int32 Count, int32 Offset, int32 Stride)
{
	check(Count <= Size);

	if (Offset < 0 || Offset + Count * Stride > Size)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddSamples: Invalid offset or count"));
		return;
	}

	for (int32 i = 0; i < Count; ++i)
	{
		AddSample(NewSamples[Offset + i * Stride]);
	}
}

void FCircularAudioBuffer::GetLastSamples(TArray<float>& OutSamples, int32 Count) const
{
	check(Count <= Size);

	if (OutSamples.Num() < Count)
	{
		OutSamples.SetNumUninitialized(Count);
	}
		
	int32 StartIndex = (Head - Count + Size) % Size;

	for (int32 i = 0; i < Count; ++i)
	{
		int32 Index = (StartIndex + i) % Size;
		OutSamples[i] = Buffer[Index];
	}
}