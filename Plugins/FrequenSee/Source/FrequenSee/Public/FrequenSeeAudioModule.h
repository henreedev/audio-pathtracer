#pragma once

// class FFrequenSeeAudioSpatializationPluginFactory;
class FFrequenSeeAudioOcclusionPluginFactory;
class FFrequenSeeAudioReverbPluginFactory;

class FFrequenSeeAudioModule: public IModuleInterface
{
public:
	/** Called when the module is being loaded. */
	virtual void StartupModule() override;

private:
	/** Factory object used to instantiate the spatialization plugin. */
	// TUniquePtr<FFrequenSeeSpatializationPluginFactory> SpatializationPluginFactory;

	/** Factory object used to instantiate the occlusion plugin. */
	TUniquePtr<FFrequenSeeAudioOcclusionPluginFactory> OcclusionPluginFactory;

	/** Factory object used to instantiate the reverb plugin. */
	TUniquePtr<FFrequenSeeAudioReverbPluginFactory> ReverbPluginFactory;
};
