#include "FrequenSeeAudioReverbPlugin.h"
#include "Sound/SoundSubmix.h"
#include "FrequenSeeAudioModule.h"
#include "HAL/UnrealMemory.h"
#include "Components/AudioComponent.h"


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

FFrequenSeeAudioReverbPlugin::~FFrequenSeeAudioReverbPlugin()
{
}

FSoundEffectSubmixPtr FFrequenSeeAudioReverbPlugin::GetEffectSubmix()
{
	UE_LOG(LogTemp, Display, TEXT("Getting reverb submix effect"));
	
	USoundSubmix* Submix = GetSubmix();
	UFrequenSeeAudioReverbSubmixPluginPreset *Preset = NewObject<UFrequenSeeAudioReverbSubmixPluginPreset>(Submix, TEXT("FrequenSee Audio Reverb Preset"));
	ReverbSubmixEffect = USoundEffectSubmixPreset::CreateInstance<FSoundEffectSubmixInitData, FSoundEffectSubmix>(FSoundEffectSubmixInitData(), *Preset);

	if (ReverbSubmixEffect)
	{
		StaticCastSharedPtr<FFrequenSeeAudioReverbSubmixPlugin, FSoundEffectSubmix>(ReverbSubmixEffect)->SetReverbPlugin(this);
		ReverbSubmixEffect->SetEnabled(true);
	}

	return ReverbSubmixEffect;
}

USoundSubmix* FFrequenSeeAudioReverbPlugin::GetSubmix()
{
	UE_LOG(LogTemp, Display, TEXT("Getting reverb submix"));
	
	static const FString DefaultSubmixName = TEXT("FrequenSee Audio Reverb Submix");
	ReverbSubmix = NewObject<USoundSubmix>(USoundSubmix::StaticClass(), *DefaultSubmixName);

	return ReverbSubmix.Get();
}

void FFrequenSeeAudioReverbPlugin::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
	SamplingRate = InitializationParams.SampleRate;
	FrameSize = InitializationParams.BufferLength;
	Sources.AddDefaulted(InitializationParams.NumSources);

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
	UE_LOG(LogTemp, Warning, TEXT("REVERB PLUGIN PROCESSING"));
	
	const UAudioComponent* AudioComponent = UAudioComponent::GetAudioComponentFromID(InputData.AudioComponentId);
	UFrequenSeeAudioComponent* FrequenSeeSourceComponent = AudioComponent->GetOwner()->FindComponentByClass<UFrequenSeeAudioComponent>();
	
	TArray<TArray<float>>& ImpulseBuffer = FrequenSeeSourceComponent->GetImpulseResponse();
	ConvolveStereo(InputData,
		ImpulseBuffer[0],
		ImpulseBuffer[1],
		OutputData);

	// copy input to output
	// const float* InBufferData = InputData.AudioBuffer->GetData();
	// float* OutBufferData = OutputData.AudioBuffer.GetData();
	// const int32 NumSamples = InputData.AudioBuffer->Num();
	// for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
	// {
	// 	OutBufferData[SampleIndex] = InBufferData[SampleIndex];
	// }
}

void FFrequenSeeAudioReverbPlugin::ConvolveStereo(const FAudioPluginSourceInputData& InputData,
	const TArray<float>& IR_Left, const TArray<float>& IR_Right, FAudioPluginSourceOutputData& OutputData)
{

	const float* DrySignal = InputData.AudioBuffer->GetData();
	float* OutWetSignal = OutputData.AudioBuffer.GetData();
	
	const int32 NumFrames = InputData.AudioBuffer->Num() / 2;
	const int32 IRSize = IR_Left.Num();
	
	for (int32 i = 0; i < NumFrames; ++i)
	{
		float AccumL = 0.0f;
		float AccumR = 0.0f;

		for (int32 j = 0; j < IRSize; ++j)
		{
			if (i - j < 0)
				break;

			AccumL += DrySignal[(i - j) * 2]     * IR_Left[j];
			AccumR += DrySignal[(i - j) * 2 + 1] * IR_Right[j];
		}

		OutWetSignal[i * 2]     = AccumL;
		OutWetSignal[i * 2 + 1] = AccumR;
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

	UE_LOG(LogTemp, Warning, TEXT("SUBMIX PLUGIN PROCESSING"));
	
}

/** Called to specify the singleton reverb plugin instance. */
void FFrequenSeeAudioReverbSubmixPlugin::SetReverbPlugin(FFrequenSeeAudioReverbPlugin* Plugin)
{
	ReverbPlugin = Plugin;
}

FFrequenSeeAudioReverbSubmixPluginSettings::FFrequenSeeAudioReverbSubmixPluginSettings()
	: bApplyReverb(true)
{}