#include "FFTConvolver2.h"


void FFTConvolverStereo2::Initialize(const TArray<float>& InIR, int32 InBlockSize)
{
    BlockSize = InBlockSize;
    IRLength = InIR.Num();
    OverlapSize = IRLength - 1;
    FFTSize = NextPowerOfTwo(OverlapSize + BlockSize);

    FwdCfg = kiss_fftr_alloc(FFTSize, false, nullptr, nullptr);
    InvCfg = kiss_fftr_alloc(FFTSize, true,  nullptr, nullptr);

    // Precompute IR FFT
    IRFreq.SetNumZeroed(FFTSize/2 + 1);
    TimeBuf.SetNumZeroed(FFTSize);
    FreqBuf.SetNumZeroed(FFTSize/2 + 1);
    OutTime.SetNumZeroed(FFTSize);
    OverlapLeft.Init(0.0f, OverlapSize);
    OverlapRight.Init(0.0f, OverlapSize);
    PrevTailLeft.Init(0.0f, FadeLength);
    PrevTailRight.Init(0.0f, FadeLength);

    FMemory::Memcpy(TimeBuf.GetData(), InIR.GetData(), IRLength * sizeof(float));
    FMemory::Memzero(TimeBuf.GetData() + IRLength,
                     (FFTSize - IRLength) * sizeof(float));
    kiss_fftr(FwdCfg, TimeBuf.GetData(), IRFreq.GetData());

    bIsInitialized = true;
}

void FFTConvolverStereo2::ProcessMono(const float* InBuffer, float* OutBuffer, TArray<float>& Overlap, TArray<float>& PrevTail)
{
    check(bIsInitialized);

    // 1) Prepare segment [overlap|block]
    FMemory::Memcpy(TimeBuf.GetData(), Overlap.GetData(), OverlapSize * sizeof(float));
    FMemory::Memcpy(TimeBuf.GetData() + OverlapSize, InBuffer, BlockSize * sizeof(float));
    FMemory::Memzero(TimeBuf.GetData() + OverlapSize + BlockSize,
                     (FFTSize - OverlapSize - BlockSize) * sizeof(float));

    // 2) Forward FFT
    kiss_fftr(FwdCfg, TimeBuf.GetData(), FreqBuf.GetData());

    // 3) Multiply in freq domain
    for (int32 k = 0; k < FFTSize/2 + 1; ++k)
    {
        float a_r = FreqBuf[k].r, a_i = FreqBuf[k].i;
        float b_r = IRFreq[k].r, b_i = IRFreq[k].i;
        FreqBuf[k].r = a_r * b_r - a_i * b_i;
        FreqBuf[k].i = a_r * b_i + a_i * b_r;
    }

    // 4) Inverse FFT
    kiss_fftri(InvCfg, FreqBuf.GetData(), OutTime.GetData());
    // Scale output
    const float Scale = 1.0f / float(FFTSize);
    for (int32 i = 0; i < FFTSize; ++i)
    {
        OutTime[i] *= Scale;
    }

    // 5) Extract valid part
    for (int32 i = 0; i < BlockSize; ++i) {
        if (i < FadeLength) {
            float t = float(i) / (FadeLength - 1);
            OutBuffer[i] = PrevTail[i] * (1.f - t) + OutTime[OverlapSize + i] * t;
        } else {
            OutBuffer[i] = OutTime[OverlapSize + i];
        }
    }

    // 6) Update overlap and prev tail for the channel
    Overlap.RemoveAt(0, BlockSize, false);
    Overlap.Append(InBuffer, BlockSize);
    for (int32 i = 0; i < FadeLength; ++i) {
        PrevTail[i] = OutTime[OverlapSize + BlockSize - FadeLength + i];
    }
}

void FFTConvolverStereo2::ProcessStereo(const float* InInterleaved, float* OutInterleaved)
{
    check(bIsInitialized);

    // Temporary mono buffers
    TArray<float> MonoIn, MonoOut;
    MonoIn.SetNumUninitialized(BlockSize);
    MonoOut.SetNumUninitialized(BlockSize);

    // Process left channel
    for (int32 i = 0; i < BlockSize; ++i) MonoIn[i] = InInterleaved[2*i];
    ProcessMono(MonoIn.GetData(), MonoOut.GetData(), OverlapLeft, PrevTailLeft);
    for (int32 i = 0; i < BlockSize; ++i) OutInterleaved[2*i] = MonoOut[i];

    // Process right channel
    for (int32 i = 0; i < BlockSize; ++i) MonoIn[i] = InInterleaved[2*i + 1];
    ProcessMono(MonoIn.GetData(), MonoOut.GetData(), OverlapRight, PrevTailRight);
    for (int32 i = 0; i < BlockSize; ++i) OutInterleaved[2*i + 1] = MonoOut[i];
}