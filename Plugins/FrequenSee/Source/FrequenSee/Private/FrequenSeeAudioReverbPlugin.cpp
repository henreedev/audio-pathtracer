#include "FrequenSeeAudioReverbPlugin.h"
#include "Sound/SoundSubmix.h"
#include "FrequenSeeAudioModule.h"
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
	: ReverbSubmix(nullptr),
	ReverbSubmixEffect(nullptr)
{
	
}

FFrequenSeeAudioReverbPlugin::FFrequenSeeAudioReverbPlugin(FVTableHelper& Helper)
{
}

FFrequenSeeAudioReverbPlugin::~FFrequenSeeAudioReverbPlugin()
{
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

	InLeft.SetNumUninitialized(FrameSize);
	InRight.SetNumUninitialized(FrameSize);
	OutLeft.SetNumUninitialized(FrameSize);
	OutRight.SetNumUninitialized(FrameSize);

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
	// UE_LOG(LogTemp, Warning, TEXT("REVERB PLUGIN PROCESSING"));
	
	const UAudioComponent* AudioComponent = UAudioComponent::GetAudioComponentFromID(InputData.AudioComponentId);
	UFrequenSeeAudioComponent* FrequenSeeSourceComponent = AudioComponent->GetOwner()->FindComponentByClass<UFrequenSeeAudioComponent>();
	const int RenderFrame =  FrequenSeeSourceComponent->FrameCount;
	if (RenderFrame != CurrentFrame)
	{
		CurrentFrame = RenderFrame;
		CurrentSample = 0;
	}
	CurrentSample++;

	TArray<TArray<float>>& ImpulseBuffer = FrequenSeeSourceComponent->GetImpulseResponse();
	// ConvolveStereo(InputData,
	// 	ImpulseBuffer[0],
	// 	ImpulseBuffer[1],
	// 	OutputData);
	// ConvolveFFTStereo(InputData, ImpulseBuffer[0], OutputData);

	// copy input to output
	const float* InBufferData = InputData.AudioBuffer->GetData();
	float* OutBufferData = OutputData.AudioBuffer.GetData();
	const int32 FrameSize = InputData.AudioBuffer->Num();
	for (int32 SampleIndex = 0; SampleIndex < FrameSize; ++SampleIndex)
	{
		// OutBufferData[SampleIndex] = InBufferData[SampleIndex];
		OutBufferData[SampleIndex] = 0.0f;
		for (int32 Sample = 0; Sample < ImpulseBuffer[0].Num() / 16; ++Sample)
		{
			OutBufferData[SampleIndex] += InBufferData[SampleIndex] * ImpulseBuffer[0][Sample];
		}
	}
}
void NormalizeImpulseResponse(TArray<float>& IR);
void FFrequenSeeAudioReverbPlugin::ConvolveStereo(const FAudioPluginSourceInputData& InputData,
	const TArray<float>& IR_Left, const TArray<float>& IR_Right, FAudioPluginSourceOutputData& OutputData)
{
	const float* DrySignal = InputData.AudioBuffer->GetData();
	float* OutWetSignal = OutputData.AudioBuffer.GetData();

	const int32 FrameSize = InputData.AudioBuffer->Num(); // Interleaved stereo
	const int32 ChannelFrameSize = FrameSize / 2;

	// Deinterleave dry signal into separate Left and Right arrays
	TArray<float> DryLeft, DryRight;
	DryLeft.SetNumZeroed(ChannelFrameSize);
	// DryRight.SetNumZeroed(ChannelFrameSize);

	for (int32 i = 0; i < ChannelFrameSize; ++i)
	{
		DryLeft[i] = DrySignal[i * 2];
		// DryRight[i] = DrySignal[i * 2 + 1];
	}
	
	// Simple time-domain convolution: y[n] = sum_{k=0}^{N-1} x[n-k] * h[k]
	auto Convolve = [&](const TArray<float>& Input, const TArray<float>& IR, TArray<float>& Output)
	{
		// 48k sample rate, 1 sec of IR
		const int32 IrLength = IR.Num() / 16;
		// in length = out length = 1024
		const int32 InputLength = Input.Num();
		const int32 OutputLength = Output.Num();
		
		// Perform full convolution
		
	};

	Convolve(DryLeft, IR_Left, OutLeft);
	// Convolve(DryRight, IR_Right, OutRight);
	
	// Interleave back to output buffer
	for (int32 i = 0; i < ChannelFrameSize; ++i)
	{
		OutWetSignal[i * 2] = OutLeft[i];
		// OutWetSignal[i * 2 + 1] = OutRight[i];
		OutWetSignal[i * 2 + 1] = OutLeft[i];
	}
}

void FFrequenSeeAudioReverbPlugin::ConvolveFFTStereo(const FAudioPluginSourceInputData& InputData,
	const TArray<float>& IR, FAudioPluginSourceOutputData& OutputData)
{
	int Frames = FrameSize / 2;
	const float* InterleavedInput = InputData.AudioBuffer->GetData();
	float* InterleavedOutput = OutputData.AudioBuffer.GetData();
	
	// Deinterleave
	for(int i=0; i<Frames; i++){
		InLeft[i] = InterleavedInput[i*2];
		InRight[i] = InterleavedInput[i*2+1];
	}

	FPartitionedFFTConvolver Convolver;
	Convolver.Initialize(IR);

	// Process channels
	Convolver.ProcessBlock(InLeft.GetData(), OutLeft.GetData(), Frames);
	Convolver.ProcessBlock(InRight.GetData(), OutRight.GetData(), Frames);

	// Interleave output
	for(int i=0; i<Frames; i++){
		InterleavedOutput[i*2] = FMath::Tanh(OutLeft[i]);
		InterleavedOutput[i*2+1] = FMath::Tanh(OutRight[i]);
	}
}

// void FFrequenSeeAudioReverbPlugin::ConvolveFFTStereo(const FAudioPluginSourceInputData& InputData,
// 	const TArray<float>& IR, FAudioPluginSourceOutputData& OutputData)
// {
// 	bool Success = Convolver.init(256*4, IR.GetData(), IR.Num());
// 	if (!Success)
// 	{
// 		UE_LOG(LogTemp, Error, TEXT("Failed to initialize FFT convolver"));
// 		return;
// 	}
// 	float* DrySignal = InputData.AudioBuffer->GetData();
// 	float* OutWetSignal = OutputData.AudioBuffer.GetData();
//
// 	const int32 FrameSize = InputData.AudioBuffer->Num(); // Interleaved stereo
// 	const int32 ChannelFrameSize = FrameSize / 2;
//
// 	// Deinterleave dry signal into separate Left and Right arrays
// 	TArray<float> DryLeft, DryRight;
// 	DryLeft.SetNumZeroed(ChannelFrameSize);
// 	DryRight.SetNumZeroed(ChannelFrameSize);
//
// 	for (int32 i = 0; i < ChannelFrameSize; ++i)
// 	{
// 		DryLeft[i] = DrySignal[i * 2];
// 		DryRight[i] = DrySignal[i * 2 + 1];
// 	}
//
// 	// Prepare output buffers (wet signal per channel)
// 	TArray<float> WetLeft, WetRight;
// 	WetLeft.SetNumZeroed(ChannelFrameSize);
// 	WetRight.SetNumZeroed(ChannelFrameSize);
//
// 	fftconvolver::FFTConvolver &ConvRef = Convolver; 
// 	auto Convolve = [&](float* In, float* Out)
// 	{
// 		size_t processedOut = 0;
// 		size_t processedIn = 0;
// 		while (processedOut < ChannelFrameSize)
// 		{
// 			const size_t blockSize = 256; 
//       
// 			const size_t remainingOut = ChannelFrameSize - processedOut;
// 			const size_t remainingIn =  ChannelFrameSize - processedIn;
//       
// 			const size_t processingOut = std::min(remainingOut, blockSize);
// 			const size_t processingIn = std::min(remainingIn, blockSize);
//       
// 			FMemory::Memset(In, 0, ChannelFrameSize * sizeof(fftconvolver::Sample));
// 			if (processingIn > 0)
// 			{
// 				memcpy(In, &In[processedIn], processingIn * sizeof(fftconvolver::Sample));
// 			}
//       
// 			ConvRef.process(In, &Out[processedOut], processingOut);
//       
// 			processedOut += processingOut;
// 			processedIn += processingIn;
// 		}
// 	};
//
// 	// Convolve(DryLeft.GetData(), WetLeft.GetData());
// 	// Convolve(DryRight.GetData(), WetRight.GetData());
// 	Convolver.process(DryLeft.GetData(), WetLeft.GetData(), 1024);
// 	Convolver.reset();
//
// 	for (int32 i = 0; i < ChannelFrameSize; ++i)
// 	{
// 		OutWetSignal[i * 2] = FMath::Tanh(WetLeft[i]);
// 		OutWetSignal[i * 2 + 1] = FMath::Tanh(WetLeft[i]);
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