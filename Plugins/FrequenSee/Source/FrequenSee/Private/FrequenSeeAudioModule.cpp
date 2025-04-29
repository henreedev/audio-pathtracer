// #include "FrequenSeeAudioModule.h"
#include "FrequenSeeAudioModule.h"

#include "FrequenSeeAudioOcclusionPlugin.h"
#include "Interfaces/IPluginManager.h"
// #include "FrequenSeeAudioReverb.h"
// #include "FrequenSeeAudioSpatialization.h"

IMPLEMENT_MODULE(FFrequenSeeAudioModule, FrequenSee);


void FFrequenSeeAudioModule::StartupModule()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin("FrequenSee");
	check(Plugin);
	
	OcclusionPluginFactory = MakeUnique<FFrequenSeeAudioOcclusionPluginFactory>();
	IModularFeatures::Get().RegisterModularFeature(FFrequenSeeAudioOcclusionPluginFactory::GetModularFeatureName(), OcclusionPluginFactory.Get());
}
