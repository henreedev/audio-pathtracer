#pragma once

#include "CoreMinimal.h"
#include "KissFFT/kiss_fftr.h"

class FPartitionedFFTConvolver
{
public:
    FPartitionedFFTConvolver();
    ~FPartitionedFFTConvolver();

    // Initialize with MONO impulse response
    bool Initialize(const TArray<float>& ImpulseResponse, int32 InitialFFTSize = 512);
    
    // Process single channel audio block
    void ProcessBlock(const float* InputBuffer, float* OutputBuffer, int32 NumSamples);

private:
    struct FConvolutionSegment
    {
        int32 FFTSize;              // FFT size for this segment (must be even)
        int32 BlockSize;            // Valid output samples per block (FFTSize / 2)
        int32 DelaySamples;         // Output delay for alignment
        TArray<float> Window;       // Windowing function (FFTSize)
        TArray<kiss_fft_cpx> IRFFT; // Precomputed FFT of IR segment
        
        // Buffers
        TArray<float> InputBuffer;  // Circular buffer (FFTSize)
        TArray<float> OverlapBuffer;// Overlap from previous block (FFTSize / 2)
        
        kiss_fftr_cfg FFTConfig;
        kiss_fftr_cfg IFFTConfig;
    };

    TArray<FConvolutionSegment> Segments;
    bool bInitialized;
    int32 BlockSize;

    void CreateSegment(int32 FFTSize, int32 DelaySamples, const TArray<float>& IRSegment);
    void ProcessSegment(FConvolutionSegment& Segment, const float* Input, float* Output);
};