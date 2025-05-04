#include "FrequenSeeAudioReverbPlugin.h"
#include "Sound/SoundSubmix.h"
#include "FrequenSeeAudioModule.h"
#include "HAL/UnrealMemory.h"


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

FFrequenSeeAudioReverbSubmixPluginSettings::FFrequenSeeAudioReverbSubmixPluginSettings()
	: bApplyReverb(true)
{}