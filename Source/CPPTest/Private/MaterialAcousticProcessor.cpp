// MaterialAcousticProcessor.cpp

#include "MaterialAcousticProcessor.h"
#include "Engine/Engine.h" // for UE_LOG

FAcousticOutputs UMaterialAcousticProcessor::ApplyMaterialFD(
    const TArray<float>& InBuffer,
    const FMaterialAcousticFD& PropsIn
)
{
    const int32 L = InBuffer.Num();
    // next power of two
    int32 N = 1;
    while (N < L) N <<= 1;
    const int32 NumBins = N/2 + 1;

    // quick error check
    if (PropsIn.Absorption.Responses.Num()   != NumBins ||
        PropsIn.Transmission.Responses.Num() != NumBins ||
        PropsIn.Scattering.Responses.Num()   != NumBins)
    {
        UE_LOG(LogTemp, Error, TEXT("All response curves must have length %d"), NumBins);
        return {};
    }

    // --- 1) prepare time-domain buffer (zero-padded) ---
    TArray<float> TimeIn; TimeIn.AddZeroed(N);
    FMemory::Memcpy(TimeIn.GetData(), InBuffer.GetData(), sizeof(float)*L);

    // allocate freq-domain arrays
    TArray<kiss_fft_cpx> FreqIn;   FreqIn.SetNumZeroed(NumBins);
    TArray<kiss_fft_cpx> FreqSpec; FreqSpec.SetNumZeroed(NumBins);
    TArray<kiss_fft_cpx> FreqDiff; FreqDiff.SetNumZeroed(NumBins);
    TArray<kiss_fft_cpx> FreqTrans;FreqTrans.SetNumZeroed(NumBins);

    // allocate time-domain outputs
    TArray<float> TimeSpec; TimeSpec.AddZeroed(N);
    TArray<float> TimeDiff; TimeDiff.AddZeroed(N);
    TArray<float> TimeTrans;TimeTrans.AddZeroed(N);

    // --- 2) create KissFFT configs ---
    kiss_fftr_cfg FFTFwd = kiss_fftr_alloc(N, 0, nullptr, nullptr);
    kiss_fftr_cfg FFTInv = kiss_fftr_alloc(N, 1, nullptr, nullptr);

    // --- 3) forward FFT ---
    kiss_fftr(FFTFwd, TimeIn.GetData(), FreqIn.GetData());

    // --- 4) per-bin gains ---
    for (int32 b = 0; b < NumBins; ++b)
    {
        const float α = PropsIn.Absorption.Responses[b];
        float       τ = PropsIn.Transmission.Responses[b];
        const float σ = PropsIn.Scattering.Responses[b];

        // energy conservation
        const float Refl = 1.0f - α;
        if (Refl + τ > 1.0f) τ = 1.0f - Refl;

        const float SpecGain = Refl * (1.0f - σ);
        const float DiffGain = Refl * σ;
        const float TransGain= τ;

        kiss_fft_cpx InC = FreqIn[b];
        FreqSpec[b].r = InC.r * SpecGain;
        FreqSpec[b].i = InC.i * SpecGain;
        FreqDiff[b].r = InC.r * DiffGain;
        FreqDiff[b].i = InC.i * DiffGain;
        FreqTrans[b].r= InC.r * TransGain;
        FreqTrans[b].i= InC.i * TransGain;
    }

    // --- 5) inverse FFTs ---
    kiss_fftri(FFTInv, FreqSpec.GetData(), TimeSpec.GetData());
    kiss_fftri(FFTInv, FreqDiff.GetData(), TimeDiff.GetData());
    kiss_fftri(FFTInv, FreqTrans.GetData(),TimeTrans.GetData());

    // --- 6) normalize & copy to outputs ---
    FAcousticOutputs Out;
    Out.Specular.SetNum(L);
    Out.Diffuse.SetNum(L);
    Out.Transmitted.SetNum(L);

    for (int32 i = 0; i < L; ++i)
    {
        const float Scale = 1.0f / float(N);
        Out.Specular[i]    = TimeSpec[i]  * Scale;
        Out.Diffuse[i]     = TimeDiff[i]  * Scale;
        Out.Transmitted[i] = TimeTrans[i] * Scale;
    }

    // --- 7) cleanup ---
    free(FFTFwd);
    free(FFTInv);

    return Out;
}
