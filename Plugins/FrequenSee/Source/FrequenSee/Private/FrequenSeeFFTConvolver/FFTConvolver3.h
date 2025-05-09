#pragma once

#include "CoreMinimal.h"
#include "KissFFT/kiss_fftr.h"

class  FFTConvolver3
{

public:
    /**
     * Initialize the OLA convolver with given IR, block (hop) size, and partition size.
     * @param InIR             Impulse response samples (mono).
     * @param InBlockSize      Hop size for OLA (e.g. 512).
     * @param InPartitionSize  Partition size for convolution (e.g. 1024).
     */
    void Initialize(const TArray<float>& InIR, int32 InBlockSize = 512, int32 InPartitionSize = 1024);

    /**
     * Process one block (hop) of mono audio via OLA partitioned convolution.
     * @param InBuffer     Input samples (length = BlockSize).
     * @param OutBuffer    Output samples (length = BlockSize).
     */
    void Process(const float* InBuffer, float* OutBuffer);

private:
    bool                            bIsInitialized = false;
    int32                           BlockSize = 0;
    int32                           PartitionSize = 0;
    int32                           NumPartitions = 0;
    int32                           FFTSize = 0;

    // KissFFT configs
    kiss_fftr_cfg                   FwdCfg = nullptr;
    kiss_fftr_cfg                   InvCfg = nullptr;

    // Windows
    TArray<float>                   AnalysisWindow;
    TArray<float>                   SynthesisWindow;

    // Partitioned IR in freq-domain [partition][k]
    TArray<TArray<kiss_fft_cpx>>    IRFreqPartitions;

    // Per-partition circular buffer for input FFTs
    TArray<TArray<kiss_fft_cpx>>    InputFreqRing;
    int32                           RingIndex = 0;

    // Accumulation buffer in time domain for overlap-add
    TArray<float>                   Accum;

    // Reusable buffers
    TArray<float>                   TimeBuf;
    TArray<kiss_fft_cpx>            FreqBuf;
    TArray<float>                   ConvOut;

    static int32 NextPowerOfTwo(int32 x)
    {
        int32 v = 1;
        while (v < x) v <<= 1;
        return v;
    }
};