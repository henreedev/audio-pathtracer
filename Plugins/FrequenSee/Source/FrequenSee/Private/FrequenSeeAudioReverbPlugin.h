// #pragma once
// #include "FrequenSeeAudioComponent.h"
//
// class FrequenSeeAudioReverbPlugin : IAudioReverb
// {
// public:
// 	FrequenSeeAudioReverbPlugin();
//
// 	~FrequenSeeAudioReverbPlugin();
// 	
// 	virtual FSoundEffectSubmixPtr GetEffectSubmix() override;
// 	
// 	virtual USoundSubmix* GetSubmix() override;
// 	
// 	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
// 		
// 	virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels,
// 		UReverbPluginSourceSettingsBase* InSettings) override;
// 	
// 	virtual void OnReleaseSource(const uint32 SourceId) override;
// 	
// 	virtual void ProcessSourceAudio(const FAudioPluginSourceInputData& InputData,
// 		FAudioPluginSourceOutputData& OutputData) override;
// 	
// 	virtual void Shutdown() override;
//
// private:
// 	TArray<UFrequenSeeAudioComponent> Sources;
// };
//
// class FFrequenSeeAudioReverbPluginFactory : public IAudioReverbFactory
// {
// public:
// 	/**
// 	 * Inherited from IAudioPluginFactory
// 	 */
//
// 	/** Returns the name that should be shown in the platform settings. */
// 	virtual FString GetDisplayName() override;
//
// 	/** Returns true if the plugin supports the given platform. */
// 	virtual bool SupportsPlatform(const FString& PlatformName) override;
// 	
// 	/** Returns the class object for the reverb settings data. */
// 	virtual UClass* GetCustomReverbSettingsClass() const override;
//
// 	/** Instantiates the reverb plugin. */
// 	virtual TAudioReverbPtr CreateNewReverbPlugin(FAudioDevice* OwningDevice) override;
// };
//
