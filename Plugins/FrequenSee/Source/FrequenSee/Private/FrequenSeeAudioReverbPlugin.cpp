#include "FrequenSeeAudioReverbPlugin.h"
#include "Sound/SoundSubmix.h"
#include "FrequenSeeAudioModule.h"
#include "ConvolutionReverb.h"
#include "HAL/UnrealMemory.h"
#include "Components/AudioComponent.h"
#include "SubmixEffects/SubmixEffectConvolutionReverb.h"
#include "DSP/ConvolutionAlgorithm.h"


FFrequenSeeAudioReverbSource::FFrequenSeeAudioReverbSource()
	: bApplyReflections(true),
	PrevDuration(0.0f)
{}

FFrequenSeeAudioReverbSource::~FFrequenSeeAudioReverbSource()
{
}

void FFrequenSeeAudioReverbSource::ClearBuffers()
{
	if (!IndirectBuffer.AudioBuffer.IsEmpty())
	{
		IndirectBuffer.AudioBuffer.Empty();
	}
}

FFrequenSeeAudioReverbPlugin::FFrequenSeeAudioReverbPlugin()
	: SimulatedDuration(1.0f),
	ReverbSubmix(nullptr),
	ReverbSubmixEffect(nullptr)
{
}

FFrequenSeeAudioReverbPlugin::FFrequenSeeAudioReverbPlugin(FVTableHelper& Helper)
{
}

FFrequenSeeAudioReverbPlugin::~FFrequenSeeAudioReverbPlugin()
{
	kiss_fftr_free(ForwardCfg);
	kiss_fftr_free(InverseCfg);
}

FSoundEffectSubmixPtr FFrequenSeeAudioReverbPlugin::GetEffectSubmix()
{
	// UE_LOG(LogTemp, Display, TEXT("Getting reverb submix effect"));
	USoundSubmix* Submix = GetSubmix();
	USubmixEffectConvolutionReverbPreset *Preset = NewObject<USubmixEffectConvolutionReverbPreset>(Submix, TEXT("FrequenSee Audio Reverb Preset"));
	FSubmixEffectConvolutionReverbSettings InputSettings;
	FSoundEffectSubmixInitData InitData;
	InitData.SampleRate = SamplingRate;
	ReverbSubmixEffect = USoundEffectSubmixPreset::CreateInstance<FSoundEffectSubmixInitData, FSoundEffectSubmix>(InitData, *Preset);

	if (ReverbSubmixEffect)
	{
		// StaticCastSharedPtr<FFrequenSeeAudioReverbSubmixPlugin, FSoundEffectSubmix>(ReverbSubmixEffect)->SetReverbPlugin(this);
		ReverbSubmixEffect->SetEnabled(true);
	}

	return ReverbSubmixEffect;
}

USoundSubmix* FFrequenSeeAudioReverbPlugin::GetSubmix()
{
	// UE_LOG(LogTemp, Display, TEXT("Getting reverb submix"));
	
	static const FString DefaultSubmixName = TEXT("FrequenSee Audio Reverb Submix");
	ReverbSubmix = NewObject<USoundSubmix>(USoundSubmix::StaticClass(), *DefaultSubmixName);

	return ReverbSubmix.Get();
}

void FFrequenSeeAudioReverbPlugin::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
	SamplingRate = InitializationParams.SampleRate;
	FrameSize = InitializationParams.BufferLength;
	Sources.AddDefaulted(InitializationParams.NumSources);
	int IRSize = SamplingRate * SimulatedDuration;
	AudioTailBufferLeft.SetSize(IRSize - 1);
	AudioTailBufferRight.SetSize(IRSize - 1);
	int CurrTailSize = IRSize - 1 + FrameSize;
	CurrAudioTailLeft.SetNumZeroed(CurrTailSize);
	CurrAudioTailRight.SetNumZeroed(CurrTailSize);
	// ConvOutputLeft.SetNumZeroed(CurrTailSize + IRSize - 1);
	// ConvOutputRight.SetNumZeroed(CurrTailSize + IRSize - 1);
	ConvOutputLeft.SetNumZeroed(CurrTailSize);
	ConvOutputRight.SetNumZeroed(CurrTailSize);

	FFTSize = FMath::RoundUpToPowerOfTwo(CurrTailSize); // Next power of 2 ≥ 97022
	const int32 NumFreqBins = FFTSize / 2 + 1;
	ForwardCfg = kiss_fftr_alloc(FFTSize, 0, nullptr, nullptr);
	InverseCfg = kiss_fftr_alloc(FFTSize, 1, nullptr, nullptr);
	OutputFreq.SetNumZeroed(NumFreqBins);
	InputFreq.SetNumZeroed(NumFreqBins);
	IRFreq.SetNumZeroed(NumFreqBins);
	TimeDomainOutput.SetNumZeroed(FFTSize);
	IRPadded.SetNumZeroed(FFTSize);
	InputPadded.SetNumZeroed(FFTSize);

	UE_LOG(LogTemp, Warning, TEXT("Initializing reverb plugin"));
}

void FFrequenSeeAudioReverbPlugin::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId,
	const uint32 NumChannels, UReverbPluginSourceSettingsBase* InSettings)
{
	UE_LOG(LogTemp, Warning, TEXT("Initializing reverb source %d"), SourceId);
	
	FFrequenSeeAudioReverbSource& Source = Sources[SourceId];
}

void FFrequenSeeAudioReverbPlugin::OnReleaseSource(const uint32 SourceId)
{
	FFrequenSeeAudioReverbSource& Source = Sources[SourceId];
	Source.ClearBuffers();
}

void FFrequenSeeAudioReverbPlugin::ProcessSourceAudio(const FAudioPluginSourceInputData& InputData,
	FAudioPluginSourceOutputData& OutputData)
{
	const UAudioComponent* AudioComponent = UAudioComponent::GetAudioComponentFromID(InputData.AudioComponentId);
	UFrequenSeeAudioComponent* FrequenSeeSourceComponent = AudioComponent->GetOwner()->FindComponentByClass<UFrequenSeeAudioComponent>();
	if (!FrequenSeeSourceComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("FrequenSeeAudioComponent not found"));
		return;
	}
	bool bApplyReverb = FrequenSeeSourceComponent->bApplyReverb;
	if (!bApplyReverb)
	{
		FMemory::Memcpy(OutputData.AudioBuffer.GetData(), InputData.AudioBuffer->GetData(), sizeof(float) * FrameSize * 2);
		return;
	}
	
	// get the impulse response
	TArray<TArray<float>> &ImpulseResponse = FrequenSeeSourceComponent->GetImpulseResponse();
	
	const int TailSize = AudioTailBufferLeft.GetSize();

	// get the 47999 prev audio samples
	AudioTailBufferLeft.GetLastSamples(CurrAudioTailLeft, TailSize);
	AudioTailBufferRight.GetLastSamples(CurrAudioTailRight, TailSize);
	// update the audio tail buffer with current audio samples
	AudioTailBufferLeft.AddSamples(InputData.AudioBuffer->GetData(), FrameSize, 0, 2);
	AudioTailBufferRight.AddSamples(InputData.AudioBuffer->GetData(), FrameSize, 1, 2);
	// add the current audio samples to the current audio tail
	FMemory::Memcpy(CurrAudioTailLeft.GetData() + TailSize, InputData.AudioBuffer->GetData(), sizeof(float) * FrameSize);
	FMemory::Memcpy(CurrAudioTailRight.GetData() + TailSize, InputData.AudioBuffer->GetData(), sizeof(float) * FrameSize);

	// LEFT
	ConvolveFFT(ImpulseResponse[0], CurrAudioTailLeft, ConvOutputLeft);
	
	// RIGHT
	ConvolveFFT(ImpulseResponse[1], CurrAudioTailRight, ConvOutputRight);

	// interleave into output
	int CurrSampleStartIndex = TailSize;
	float* OutBufferData = OutputData.AudioBuffer.GetData();
	const float* InBufferData = InputData.AudioBuffer->GetData();
	const float* LeftBufferData = ConvOutputLeft.GetData();
	const float* RightBufferData = ConvOutputRight.GetData();
	const float MixAlpha = 0.8f;
	for (int SampleIndex = 0; SampleIndex < FrameSize; ++SampleIndex)
	{
		OutBufferData[SampleIndex * 2] = FMath::Clamp(LeftBufferData[CurrSampleStartIndex + SampleIndex], -1.0f, 1.0f) * MixAlpha +
			InBufferData[SampleIndex * 2] * (1.0f - MixAlpha);
		OutBufferData[SampleIndex * 2 + 1] = FMath::Clamp(RightBufferData[CurrSampleStartIndex + SampleIndex], -1.0f, 1.0f) * MixAlpha +
			InBufferData[SampleIndex * 2 + 1] * (1.0f - MixAlpha);
		// OutBufferData[SampleIndex * 2 + 1] = FMath::Clamp(LeftBufferData[CurrSampleStartIndex + SampleIndex], -1.0f, 1.0f);
	}

	// copy input to output
	// FMemory::Memcpy(OutBufferData, InputData.AudioBuffer->GetData(), sizeof(float) * FrameSize * 2);
}

void FFrequenSeeAudioReverbPlugin::ConvolveFFT(const TArray<float>& IR, const TArray<float>& Input, TArray<float>& Output)
{
	const int32 IRSize = IR.Num();               // 48000
    const int32 InputSize = Input.Num();         // 49023
    // const int32 ConvSize = InputSize + IRSize - 1; // 97022
	const int32 ConvSize = Output.Num(); // 97022

    // const int32 FFTSize = FMath::RoundUpToPowerOfTwo(ConvSize); // Next power of 2 ≥ 97022
    const int32 NumFreqBins = FFTSize / 2 + 1;

    // Prepare zero-padded input and IR
    // TArray<float> InputPadded;
    // InputPadded.SetNumZeroed(FFTSize);
    FMemory::Memcpy(InputPadded.GetData(), Input.GetData(), sizeof(float) * InputSize);
	
    // IRPadded.SetNumZeroed(FFTSize);
    FMemory::Memcpy(IRPadded.GetData(), IR.GetData(), sizeof(float) * IRSize);

    // Allocate KissFFT configs
    // kiss_fftr_cfg ForwardCfg = kiss_fftr_alloc(FFTSize, 0, nullptr, nullptr);
    // kiss_fftr_cfg InverseCfg = kiss_fftr_alloc(FFTSize, 1, nullptr, nullptr);

    // Allocate FFT buffers
    // TArray<kiss_fft_cpx> InputFreq;
    // InputFreq.SetNumZeroed(NumFreqBins);

    // TArray<kiss_fft_cpx> IRFreq;
    // IRFreq.SetNumZeroed(NumFreqBins);

    // TArray<kiss_fft_cpx> OutputFreq;
    // OutputFreq.SetNumZeroed(NumFreqBins);

    // TArray<float> TimeDomainOutput;
    // TimeDomainOutput.SetNumZeroed(FFTSize);

    // Perform FFTs
    kiss_fftr(ForwardCfg, InputPadded.GetData(), InputFreq.GetData());
    kiss_fftr(ForwardCfg, IRPadded.GetData(), IRFreq.GetData());

    // Multiply frequency components
#pragma loop unroll 4
    for (int32 i = 0; i < NumFreqBins; ++i)
    {
        const kiss_fft_cpx& A = InputFreq[i];
        const kiss_fft_cpx& B = IRFreq[i];

        // (a + bi)(c + di) = (ac - bd) + (ad + bc)i
        OutputFreq[i].r = A.r * B.r - A.i * B.i;
        OutputFreq[i].i = A.r * B.i + A.i * B.r;
    }

    // IFFT
    kiss_fftri(InverseCfg, OutputFreq.GetData(), TimeDomainOutput.GetData());

    // Normalize (KissFFT does not normalize the IFFT)
    const float Scale = 1.0f / FFTSize;
    for (float& Sample : TimeDomainOutput)
    {
        Sample *= Scale;
    }

    // Resize and copy the valid convolution output
    // Output.SetNumUninitialized(ConvSize);
    FMemory::Memcpy(Output.GetData(), TimeDomainOutput.GetData(), sizeof(float) * ConvSize);

    // Free FFT plans
    // kiss_fftr_free(ForwardCfg);
    // kiss_fftr_free(InverseCfg);
}

// void FFrequenSeeAudioReverbPlugin::ConvolveFFT(const TArray<float>& IR, const TArray<float>& Input, TArray<float>& Output)
// {
// 	const int BlockSize = 1024;
// 	const int BlockSizeMin = 100;
// 	Convolver.init(BlockSize, IR.GetData(), IR.Num());
// 	TArray<float> InBuf;
// 	InBuf.SetNumZeroed(BlockSize);
//
// 	const float *In = Input.GetData();
// 	float *Out = Output.GetData();
// 	
// 	size_t ProcessedOut = 0;
// 	size_t ProcessedIn = 0;
// 	while (ProcessedOut < Output.Num())
// 	{
// 		const size_t CurrBlockSize = BlockSizeMin + (static_cast<size_t>(rand()) % (1+(BlockSize-BlockSizeMin))); 
//       
// 		const size_t RemainingOut = Output.Num() - ProcessedOut;
// 		const size_t RemainingIn = Input.Num() - ProcessedIn;
//       
// 		const size_t ProcessingOut = std::min(RemainingOut, CurrBlockSize);
// 		const size_t ProcessingIn = std::min(RemainingIn, CurrBlockSize);
//
// 		FMemory::Memset(InBuf.GetData(), 0, InBuf.Num() * sizeof(float));
// 		if (ProcessingIn > 0)
// 		{
// 			FMemory::Memcpy(InBuf.GetData(), &In[ProcessedIn], ProcessingIn * sizeof(float));
// 		}
//       
// 		Convolver.process(InBuf.GetData(), &Out[ProcessedOut], ProcessingOut);
//       
// 		ProcessedOut += ProcessingOut;
// 		ProcessedIn += ProcessingIn;
// 	}
// }

void NormalizeImpulseResponse(TArray<float>& IR)
{
	float SumSquares = 0.0f;
	for (float Value : IR)
	{
		SumSquares += Value * Value;
	}
	float Norm = FMath::Sqrt(SumSquares);
	if (Norm > 0.0f)
	{
		for (float& Value : IR)
		{
			Value /= Norm;
		}
	}
}

FString FFrequenSeeAudioReverbPluginFactory::GetDisplayName()
{
	static FString DisplayName = FString(TEXT("FrequenSee Audio Reverb"));
	return DisplayName;
}

bool FFrequenSeeAudioReverbPluginFactory::SupportsPlatform(const FString& PlatformName)
{
	return true;
}

TAudioReverbPtr FFrequenSeeAudioReverbPluginFactory::CreateNewReverbPlugin(FAudioDevice* OwningDevice)
{
	UE_LOG(LogTemp, Warning, TEXT("Creating new reverb plugin"));
    
	return MakeShared<FFrequenSeeAudioReverbPlugin>();
}

UClass* FFrequenSeeAudioReverbPluginFactory::GetCustomReverbSettingsClass() const
{
	return UFrequenSeeAudioReverbSettings::StaticClass();
}

FFrequenSeeAudioReverbSubmixPlugin::FFrequenSeeAudioReverbSubmixPlugin() {}

FFrequenSeeAudioReverbSubmixPlugin::~FFrequenSeeAudioReverbSubmixPlugin() {}

/** Returns the number of channels to use for input and output. */
uint32 FFrequenSeeAudioReverbSubmixPlugin::GetDesiredInputChannelCountOverride() const
{
	return 2;
}

/** Processes the audio flowing through the submix. */
void FFrequenSeeAudioReverbSubmixPlugin::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{

	// UE_LOG(LogTemp, Warning, TEXT("SUBMIX PLUGIN PROCESSING"));
	
}

/** Called to specify the singleton reverb plugin instance. */
void FFrequenSeeAudioReverbSubmixPlugin::SetReverbPlugin(FFrequenSeeAudioReverbPlugin* Plugin)
{
	ReverbPlugin = Plugin;
}

FFrequenSeeAudioReverbSubmixPluginSettings::FFrequenSeeAudioReverbSubmixPluginSettings()
	: bApplyReverb(true)
{}