#pragma once
#include "FrequenSeeAudioComponent.h"

class FFrequenSeeAudioOcclusionPlugin : public IAudioOcclusion
{
public:
	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;

	/** Called when a given source voice is assigned for rendering a given Audio Component. */
	virtual void OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels, UOcclusionPluginSourceSettingsBase* InSettings) override;

	/** Called when a given source voice will no longer be used to render an Audio Component. */
	virtual void OnReleaseSource(const uint32 SourceId) override;

	/** Called to process a single source. */
	virtual void ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;

private:
	/** Audio pipeline settings. */
	int SamplingRate = 0;
	int FrameSize = 0;

	/** Lazy-initialized state for as many sources as we can render simultaneously. */
	// TArray<UFrequenSeeAudioComponent> Sources;
};

class FFrequenSeeAudioOcclusionPluginFactory : public IAudioOcclusionFactory
{
public:
	/** Returns the name that should be shown in the platform settings. */
	virtual FString GetDisplayName() override;

	/** Returns true if the plugin supports the given platform. */
	virtual bool SupportsPlatform(const FString& PlatformName) override;

	/** Instantiates the occlusion plugin. */
	virtual TAudioOcclusionPtr CreateNewOcclusionPlugin(FAudioDevice* OwningDevice) override;
	
	/**
	* @return the UClass type of your settings for occlusion. This allows us to only pass in user settings for your plugin.
	*/
	virtual UClass* GetCustomOcclusionSettingsClass() const override;
};
