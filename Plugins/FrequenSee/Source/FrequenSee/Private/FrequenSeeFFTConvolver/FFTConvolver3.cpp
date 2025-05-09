#include "FFTConvolver3.h"

void FFTConvolver3::Initialize(const TArray<float>& InIR, int32 InBlockSize, int32 InPartitionSize)
{
    BlockSize = InBlockSize;
    PartitionSize = InPartitionSize;
    FFTSize = NextPowerOfTwo(PartitionSize + BlockSize - 1);

    // Allocate FFT configs
    FwdCfg = kiss_fftr_alloc(FFTSize, false, nullptr, nullptr);
    InvCfg = kiss_fftr_alloc(FFTSize, true,  nullptr, nullptr);

    // Build Hann analysis and synthesis windows
    AnalysisWindow.SetNumUninitialized(PartitionSize);
    SynthesisWindow.SetNumUninitialized(PartitionSize + BlockSize);
    // Hann: w[n] = 0.5*(1 - cos(2*pi*n/(N-1)))
    for (int32 n = 0; n < PartitionSize; ++n)
        AnalysisWindow[n] = 0.5f * (1.f - FMath::Cos(2.f * PI * n / (PartitionSize - 1)));
    // Synthesis window length = partition + hop
    int32 SynN = PartitionSize + BlockSize;
    SynthesisWindow.SetNumUninitialized(SynN);
    for (int32 n = 0; n < SynN; ++n)
        SynthesisWindow[n] = 0.5f * (1.f - FMath::Cos(2.f * PI * n / (SynN - 1)));

    // Partition IR and compute FFTs
    IRFreqPartitions.Empty();
    NumPartitions = (InIR.Num() + PartitionSize - 1) / PartitionSize;
    IRFreqPartitions.SetNum(NumPartitions);
    TimeBuf.SetNumZeroed(FFTSize);
    for (int32 p = 0; p < NumPartitions; ++p)
    {
        // copy partition
        int32 offset = p * PartitionSize;
        int32 length = FMath::Min(PartitionSize, InIR.Num() - offset);
        FMemory::Memzero(TimeBuf.GetData(), FFTSize * sizeof(float));
        FMemory::Memcpy(TimeBuf.GetData(), InIR.GetData() + offset, length * sizeof(float));
        IRFreqPartitions[p].SetNumZeroed(FFTSize/2 + 1);
        kiss_fftr(FwdCfg, TimeBuf.GetData(), IRFreqPartitions[p].GetData());
    }

    // Prepare input ring and buffers
    InputFreqRing.SetNum(NumPartitions);
    for (auto& ringBuf : InputFreqRing)
        ringBuf.SetNumZeroed(FFTSize/2 + 1);
    RingIndex = 0;

    Accum.SetNumZeroed(PartitionSize + BlockSize);
    FreqBuf.SetNumZeroed(FFTSize/2 + 1);
    ConvOut.SetNumZeroed(FFTSize);

    bIsInitialized = true;
}

void FFTConvolver3::Process(const float* InBuffer, float* OutBuffer)
{
    check(bIsInitialized);

    // 1) Prepare time-domain block of length PartitionSize
    TimeBuf.Reset(); TimeBuf.SetNumZeroed(FFTSize);
    for (int32 i = 0; i < PartitionSize; ++i)
        TimeBuf[i] = InBuffer[i] * AnalysisWindow[i];

    // 2) FFT input partition
    kiss_fftr(FwdCfg, TimeBuf.GetData(), InputFreqRing[RingIndex].GetData());

    // 3) Clear accumulation
    for (int32 n = 0; n < PartitionSize + BlockSize; ++n)
        Accum[n] = 0.f;

    // 4) For each partition: multiply and ifft
    int32 idx = RingIndex;
    for (int32 p = 0; p < NumPartitions; ++p)
    {
        // circular index backward
        idx = (idx - 1 + NumPartitions) % NumPartitions;
        // Freq multiply
        for (int32 k = 0; k < FFTSize/2 + 1; ++k)
        {
            float a_r = InputFreqRing[idx][k].r;
            float a_i = InputFreqRing[idx][k].i;
            float b_r = IRFreqPartitions[p][k].r;
            float b_i = IRFreqPartitions[p][k].i;
            FreqBuf[k].r = a_r * b_r - a_i * b_i;
            FreqBuf[k].i = a_r * b_i + a_i * b_r;
        }
        // IFFT
        kiss_fftri(InvCfg, FreqBuf.GetData(), ConvOut.GetData());
        // scale
        for (int32 n = 0; n < FFTSize; ++n)
            ConvOut[n] /= float(FFTSize);
        // Overlap-add with synthesis window
        for (int32 n = 0; n < PartitionSize + BlockSize; ++n)
        {
            Accum[n] += ConvOut[n] * SynthesisWindow[n];
        }
    }

    // 5) Output first BlockSize samples
    for (int32 i = 0; i < BlockSize; ++i)
        OutBuffer[i] = Accum[i];

    // 6) Shift InBuffer window by hop: rotate ring index
    RingIndex = (RingIndex + 1) % NumPartitions;
    // Client must supply overlapping input: next call's InBuffer starts at hop samples ahead.
}