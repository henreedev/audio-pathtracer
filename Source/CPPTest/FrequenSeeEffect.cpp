// Fill out your copyright notice in the Description page of Project Settings.


#include "FrequenSeeEffect.h"


// Useful header for various DSP-related utility functions.
#include "DSP/Dsp.h"
static void MakeDefaultReverbIR(
    TArray<float>& IR,
    float         ReverbTime        = 5.0f,    // seconds
    int32         SampleRate        = 48000,
    float         ReflectionDelay   = 0.05f   // seconds → user‐tweakable!
)
{
    // 1) Do we really need to build one?
    bool bNeedIR = (IR.Num() == 0);
    if (!bNeedIR)
    {
        for (float v : IR)
            if (!FMath::IsNearlyZero(v))
                return;   // already has data
        bNeedIR = true;   // all zeros → rebuild
    }

    // 2) Allocate
    const int32 N = FMath::CeilToInt(ReverbTime * SampleRate);
    IR.Init(0.0f, N);

    // 3) Early reflections (now offset by ReflectionDelay)
    const float BaseTapsSec[] = { 0.003f, 0.008f, 0.011f };
    const float TapGain[]     = { 0.8f,   0.6f,   0.5f   };
    static_assert(UE_ARRAY_COUNT(BaseTapsSec) == UE_ARRAY_COUNT(TapGain), "mismatch");

    for (int32 i = 0; i < UE_ARRAY_COUNT(BaseTapsSec); ++i)
    {
        // shift each tap later by ReflectionDelay
        int32 TapSample = int32((BaseTapsSec[i] + ReflectionDelay) * SampleRate);
        if (TapSample < N)
            IR[TapSample] += TapGain[i];
    }

    // 4) Late‐reverb tail (unchanged)
    FRandomStream RNG(12345);
    const float DecayCoeff = -3.0f / (ReverbTime * SampleRate); // –60 dB @ T60
    for (int32 n = 0; n < N; ++n)
    {
        float env   = FMath::Exp(DecayCoeff * n);
        float noise = RNG.GetFraction() * 2.f - 1.f; // [–1,1]
        IR[n] += env * noise * 0.25f;                // –12 dB RMS
    }

    // 5) Normalize so direct path (sample 0) is 1.0
    if (FMath::IsNearlyZero(IR[0]))
        IR[0] = 1.f;

    UE_LOG(LogTemp, Warning,
        TEXT("IR generated: Size=%d, Delay=%.3f s → FirstSample=%f"),
        IR.Num(), ReflectionDelay, IR.Num() ? IR[0] : -1.f);
}


// -------------------------------------------------------------
//  Initialization: just grab channel count & build IR
// -------------------------------------------------------------
void FFrequenSeeEffect::Init(const FSoundEffectSourceInitData& InInitData)
{
    NumChannels = InInitData.NumSourceChannels;
    VolumeScale = 0.5f;
    bIsActive   = true;

    // Build a mono IR once:
    MakeDefaultReverbIR(IRTimeDomain,
                        /*ReverbTime=*/5.0f,
                        InInitData.SampleRate);

    // Leave FFT-config allocation until we know block-size
    BlockSize = 0;
    FwdCfg = InvCfg = nullptr;
}





void FFrequenSeeEffect::OnPresetChanged()
{
	// Macro to retrieve the current settings value of the parent preset asset.
	GET_EFFECT_SETTINGS(FrequenSeeEffect);

	// Update the instance's variables based on the settings values. 
	// Note that Settings variable was created by the GET_EFFECT_SETTINGS macro.
	VolumeScale = Audio::ConvertToLinear(Settings.VolumeAttenuationDb);
}

// -------------------------------------------------------------
//  ProcessAudio: overlap-add convolution w/ dynamic FFT size
// -------------------------------------------------------------
void FFrequenSeeEffect::ProcessAudio(
    const FSoundEffectSourceInputData& InData,
    float*                             OutAudio)
{
    // If block-size changed (or first time), recompute FFT size & configs
    if (BlockSize != InData.NumSamples)
    {
        // 1) Update block size
        BlockSize = InData.NumSamples;

        // 2) Compute FFT size = smallest power-of-two ≥ (block + IR – 1)
        FFTSize = 1u << FMath::CeilLogTwo(BlockSize + IRTimeDomain.Num() - 1);

        // 3) Free any old kiss-fft configs
        if (FwdCfg) kiss_fft_free(FwdCfg);
        if (InvCfg) kiss_fft_free(InvCfg);

        // 4) Allocate new configs
        FwdCfg = kiss_fft_alloc(FFTSize, /*inverse=*/0, nullptr, nullptr);
        InvCfg = kiss_fft_alloc(FFTSize, /*inverse=*/1, nullptr, nullptr);

        // 5) Resize all our circular buffers
        Tail       .SetNumZeroed(FFTSize - BlockSize);
        IROverlap  .SetNumZeroed(BlockSize);

        FFTInput   .Init(kiss_fft_cpx{0,0}, FFTSize);
        FFTResult  .Init(kiss_fft_cpx{0,0}, FFTSize);
        FFTIR      .Init(kiss_fft_cpx{0,0}, FFTSize);

        // 6) Pre-FFT the IR
        {
            TArray<kiss_fft_cpx> tmp;
            tmp.Init({0,0}, FFTSize);
            for (int32 i = 0; i < IRTimeDomain.Num(); ++i)
                tmp[i].r = IRTimeDomain[i];
            kiss_fft(FwdCfg, tmp.GetData(), FFTIR.GetData());
        }
    }

    // ------- ZERO the entire output buffer to start clean -------
    FMemory::Memzero(OutAudio, sizeof(float) * BlockSize);

    // ------- 0) Copy only as much of the old tail as fits -------
    int32 NumToCopy = FMath::Min<int32>(Tail.Num(), BlockSize);
    for (int32 n = 0; n < NumToCopy; ++n)
        OutAudio[n] = Tail[n];

    // ------- 1) Pack the new block into FFTInput -------------
    for (int32 n = 0; n < BlockSize; ++n)
    {
        FFTInput[n].r = InData.InputSourceEffectBufferPtr[n];
        FFTInput[n].i = 0.f;
    }
    for (int32 n = BlockSize; n < FFTSize; ++n)
        FFTInput[n] = {0,0};

    // ------- 2) FFT → multiply by IR → IFFT -------------
    kiss_fft(FwdCfg, FFTInput.GetData(), FFTResult.GetData());
    for (int32 n = 0; n < FFTSize; ++n)
    {
        float ar = FFTResult[n].r, ai = FFTResult[n].i;
        float br = FFTIR[n].r,     bi = FFTIR[n].i;
        FFTResult[n].r = ar*br - ai*bi;
        FFTResult[n].i = ar*bi + ai*br;
    }
    kiss_fft(InvCfg, FFTResult.GetData(), FFTInput.GetData()); // reuse buffer

    // ------- 3) Overlap-add (and scale by 1/FFTSize) -------
    const float Scale = 1.f / FFTSize;
    // add current block
    for (int32 n = 0; n < BlockSize; ++n)
        OutAudio[n] += FFTInput[n].r * Scale;

    // stash the new tail for next time
    for (int32 n = 0; n < Tail.Num(); ++n)
        Tail[n] = FFTInput[n + BlockSize].r * Scale;

    // ------- 4) Apply global gain -------------
    for (int32 n = 0; n < BlockSize; ++n)
        OutAudio[n] *= VolumeScale;
}



void UFrequenSeeEffectPreset::SetSettings(const FFrequenSeeEffectSettings& InSettings)
{
	// Performs necessary broadcast to effect instances
	UpdateSettings(InSettings);
}
