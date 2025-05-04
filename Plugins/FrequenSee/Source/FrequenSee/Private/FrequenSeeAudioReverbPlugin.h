#pragma once
#include "FrequenSeeAudioComponent.h"
#include "Sound/SoundEffectSubmix.h"
#include "FrequenSeeAudioReverbSettings.h"
#include "FrequenSeeAudioReverbPlugin.generated.h"

struct FFrequenSeeAudioReverbSource
{
	FFrequenSeeAudioReverbSource();

	~FFrequenSeeAudioReverbSource();

	bool bApplyReflections;
	
	/** Ambisonic buffer with reflections applied. */
	FAudioPluginSourceOutputData IndirectBuffer;

	float PrevDuration;
	
	void ClearBuffers();
};

class FFrequenSeeAudioReverbPlugin : IAudioReverb
{
public:
	FFrequenSeeAudioReverbPlugin();

	~FFrequenSeeAudioReverbPlugin();
	
	virtual FSoundEffectSubmixPtr GetEffectSubmix() override;
	
	virtual USoundSubmix* GetSubmix() override;
	
	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
		
	virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels,
		UReverbPluginSourceSettingsBase* InSettings) override;
	
	virtual void OnReleaseSource(const uint32 SourceId) override;
	
	virtual void ProcessSourceAudio(const FAudioPluginSourceInputData& InputData,
		FAudioPluginSourceOutputData& OutputData) override;
	
private:
	int SamplingRate = 0;
	int FrameSize = 0;
	
	TArray<FFrequenSeeAudioReverbSource> Sources;
	
	TWeakObjectPtr<USoundSubmix> ReverbSubmix;

	FSoundEffectSubmixPtr ReverbSubmixEffect;
};

class FFrequenSeeAudioReverbPluginFactory : public IAudioReverbFactory
{
public:
	/**
	 * Inherited from IAudioPluginFactory
	 */

	/** Returns the name that should be shown in the platform settings. */
	virtual FString GetDisplayName() override;

	/** Returns true if the plugin supports the given platform. */
	virtual bool SupportsPlatform(const FString& PlatformName) override;
	
	/** Returns the class object for the reverb settings data. */
	virtual UClass* GetCustomReverbSettingsClass() const override;

	/** Instantiates the reverb plugin. */
	virtual TAudioReverbPtr CreateNewReverbPlugin(FAudioDevice* OwningDevice) override;
};

class FFrequenSeeAudioReverbSubmixPlugin : public FSoundEffectSubmix
{
public:
	FFrequenSeeAudioReverbSubmixPlugin();

	~FFrequenSeeAudioReverbSubmixPlugin();

	/** Returns the number of channels to use for input and output. */
	virtual uint32 GetDesiredInputChannelCountOverride() const override;

	/** Processes the audio flowing through the submix. */
	virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;

	/** Called to specify the singleton reverb plugin instance. */
	void SetReverbPlugin(FFrequenSeeAudioReverbPlugin* Plugin);
};

USTRUCT(BlueprintType)
struct FFrequenSeeAudioReverbSubmixPluginSettings
{
	GENERATED_USTRUCT_BODY()

	/** If true, listener-centric reverb will be applied to the audio received as input to this submix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubmixSettings)
	bool bApplyReverb;

	FFrequenSeeAudioReverbSubmixPluginSettings();
};

UCLASS()
class UFrequenSeeAudioReverbSubmixPluginPreset : public USoundEffectSubmixPreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS(FrequenSeeAudioReverbSubmixPlugin);

	UPROPERTY(EditAnywhere, Category = SubmixPreset)
	FFrequenSeeAudioReverbSubmixPluginSettings Settings;
};

