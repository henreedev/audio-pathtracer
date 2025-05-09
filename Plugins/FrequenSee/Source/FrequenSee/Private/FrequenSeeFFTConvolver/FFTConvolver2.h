/*
 * FFTConvolverStereo.h/.cpp
 * Implements Overlap-Save FFT convolution (OLS) for stereo signals using KissFFT
 * IR length: configurable (e.g. 48000 samples)
 * Block size: configurable (e.g. 1024 samples)
 */

#pragma once

#include "CoreMinimal.h"
#include "KissFFT/kiss_fftr.h"

class FFTConvolverStereo2
{

public:
    /**
     * Initialize the convolver with a given impulse response.
     * @param InIR         The impulse response samples (mono).
     * @param InBlockSize  The processing block size (e.g. 1024).
     */
    void Initialize(const TArray<float>& InIR, int32 InBlockSize = 1024);

    /**
     * Process one block of mono audio using OLS FFT convolution.
     * @param InBuffer     Input samples (length = BlockSize).
     * @param OutBuffer    Output samples (length = BlockSize).
     */
    void ProcessMono(const float* InBuffer, float* OutBuffer, TArray<float>& Overlap, TArray<float>& PrevTail);

    /**
     * Process one block of interleaved stereo audio.
     * Internally calls the mono ProcessMono twice (L and R).
     * @param InInterleaved    Interleaved input buffer [L,R,L,R,...]
     * @param OutInterleaved   Interleaved output buffer (same layout).
     */
    void ProcessStereo(const float* InInterleaved, float* OutInterleaved);

private:
    bool                        bIsInitialized = false;
    int32                       BlockSize = 0;
    int32                       IRLength = 0;
    int32                       OverlapSize = 0;
    int32                       FFTSize = 0;
    int32                       FadeLength = 256;

    // KissFFT configs
    kiss_fftr_cfg               FwdCfg = nullptr;
    kiss_fftr_cfg               InvCfg = nullptr;

    // Precomputed IR in frequency domain
    TArray<kiss_fft_cpx>        IRFreq;

    // Overlap state for mono processing
    TArray<float> OverlapLeft;
    TArray<float> OverlapRight;
    TArray<float> PrevTailLeft;
    TArray<float> PrevTailRight;

    // Reusable buffers
    TArray<float>               TimeBuf;
    TArray<kiss_fft_cpx>        FreqBuf;
    TArray<float>               OutTime;

    /** Compute next power-of-two >= x */
    static int32 NextPowerOfTwo(int32 x)
    {
        int32 v = 1;
        while (v < x) v <<= 1;
        return v;
    }
};