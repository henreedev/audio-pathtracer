#include "FFTConvolver.h"

FPartitionedFFTConvolver::FPartitionedFFTConvolver() 
    : bInitialized(false), BlockSize(1024) {}

FPartitionedFFTConvolver::~FPartitionedFFTConvolver()
{
    for (auto& Segment : Segments)
    {
        kiss_fftr_free(Segment.FFTConfig);
        kiss_fftr_free(Segment.IFFTConfig);
    }
}

bool FPartitionedFFTConvolver::Initialize(const TArray<float>& ImpulseResponse, int32 InitialFFTSize)
{
    if (ImpulseResponse.Num() == 0) return false;

    // Cleanup existing state
    Segments.Empty();
    bInitialized = false;

    int32 IRSamplesRemaining = ImpulseResponse.Num();
    const float* IRPtr = ImpulseResponse.GetData();
    int32 CumulativeDelay = 0;
    int32 CurrentFFTSize = InitialFFTSize;

    // Create segments with doubling FFT sizes
    while (IRSamplesRemaining > 0)
    {
        const int32 MaxSegmentLength = CurrentFFTSize / 2; // 50% overlap
        const int32 ActualLength = FMath::Min(MaxSegmentLength, IRSamplesRemaining);
        
        TArray<float> SegmentIR;
        SegmentIR.Append(IRPtr, ActualLength);
        CreateSegment(CurrentFFTSize, CumulativeDelay, SegmentIR);

        IRPtr += ActualLength;
        IRSamplesRemaining -= ActualLength;
        
        CumulativeDelay += CurrentFFTSize / 2; // Delay increases by block size
        CurrentFFTSize *= 2; // Double FFT size for next segment
    }

    bInitialized = true;
    return true;
}

void FPartitionedFFTConvolver::CreateSegment(int32 FFTSize, int32 DelaySamples, const TArray<float>& IRSegment)
{
    FConvolutionSegment NewSegment;
    NewSegment.FFTSize = FFTSize;
    NewSegment.BlockSize = FFTSize / 2;
    NewSegment.DelaySamples = DelaySamples;

    // Initialize FFT plans
    NewSegment.FFTConfig = kiss_fftr_alloc(FFTSize, 0, nullptr, nullptr);
    NewSegment.IFFTConfig = kiss_fftr_alloc(FFTSize, 1, nullptr, nullptr);

    // Generate Hann window
    NewSegment.Window.SetNum(FFTSize);
    for (int32 i = 0; i < FFTSize; ++i) {
        NewSegment.Window[i] = 0.5f * (1.0f - FMath::Cos(2.0f * PI * i / (FFTSize - 1)));
    }

    // Prepare and transform IR
    TArray<float> PaddedIR;
    PaddedIR.AddZeroed(FFTSize);
    FMemory::Memcpy(PaddedIR.GetData(), IRSegment.GetData(), IRSegment.Num() * sizeof(float));
    
    // Apply window to IR
    for (int32 i = 0; i < FFTSize; ++i) {
        PaddedIR[i] *= NewSegment.Window[i];
    }
    
    NewSegment.IRFFT.SetNum(FFTSize / 2 + 1);
    kiss_fftr(NewSegment.FFTConfig, PaddedIR.GetData(), NewSegment.IRFFT.GetData());

    // Initialize buffers
    NewSegment.InputBuffer.AddZeroed(FFTSize);
    NewSegment.OverlapBuffer.AddZeroed(FFTSize / 2);

    Segments.Add(NewSegment);
}

void FPartitionedFFTConvolver::ProcessBlock(const float* InputBuffer, float* OutputBuffer, int32 NumSamples)
{
    if (!bInitialized || NumSamples != BlockSize) return;

    // Clear output
    FMemory::Memset(OutputBuffer, 0, NumSamples * sizeof(float));

    // Process all segments
    for (FConvolutionSegment& Segment : Segments)
    {
        ProcessSegment(Segment, InputBuffer, OutputBuffer);
    }
}

void FPartitionedFFTConvolver::ProcessSegment(FConvolutionSegment& Segment, const float* Input, float* Output)
{
    // Shift old input samples (50% overlap)
    FMemory::Memmove(
        Segment.InputBuffer.GetData(),
        &Segment.InputBuffer[Segment.BlockSize],
        Segment.BlockSize * sizeof(float)
    );
    
    // Copy new input samples
    FMemory::Memcpy(
        &Segment.InputBuffer[Segment.BlockSize],
        Input,
        Segment.BlockSize * sizeof(float)
    );

    // Apply window to input
    TArray<float> WindowedInput;
    WindowedInput.Append(Segment.InputBuffer);
    for (int32 i = 0; i < Segment.FFTSize; ++i) {
        WindowedInput[i] *= Segment.Window[i];
    }

    // Forward FFT
    TArray<kiss_fft_cpx> FFTData;
    FFTData.SetNum(Segment.FFTSize / 2 + 1);
    kiss_fftr(Segment.FFTConfig, WindowedInput.GetData(), FFTData.GetData());

    // Frequency-domain multiplication
    TArray<kiss_fft_cpx> ProductFFT;
    ProductFFT.SetNum(Segment.FFTSize / 2 + 1);
    for (int32 i = 0; i <= Segment.FFTSize / 2; ++i) {
        ProductFFT[i].r = FFTData[i].r * Segment.IRFFT[i].r - 
                         FFTData[i].i * Segment.IRFFT[i].i;
        ProductFFT[i].i = FFTData[i].r * Segment.IRFFT[i].i + 
                         FFTData[i].i * Segment.IRFFT[i].r;
    }

    // Inverse FFT
    TArray<float> TimeDomain;
    TimeDomain.AddZeroed(Segment.FFTSize);
    kiss_fftri(Segment.IFFTConfig, ProductFFT.GetData(), TimeDomain.GetData());

    // Scale output
    const float Scale = 1.0f / Segment.FFTSize;
    for (int32 i = 0; i < Segment.FFTSize; ++i) {
        TimeDomain[i] *= Scale;
    }

    // Overlap-add with previous block
    for (int32 i = 0; i < Segment.FFTSize; ++i) {
        if (i < Segment.BlockSize) {
            TimeDomain[i] += Segment.OverlapBuffer[i];
        }
    }

    // Store new overlap
    FMemory::Memcpy(
        Segment.OverlapBuffer.GetData(),
        &TimeDomain[Segment.BlockSize],
        Segment.BlockSize * sizeof(float)
    );

    // Add valid output with delay compensation
    const int32 OutputOffset = Segment.DelaySamples;
    const int32 ValidSamples = FMath::Min(Segment.BlockSize, BlockSize - OutputOffset);
    for (int32 i = 0; i < ValidSamples; ++i) {
        Output[OutputOffset + i] += TimeDomain[i];
    }
}