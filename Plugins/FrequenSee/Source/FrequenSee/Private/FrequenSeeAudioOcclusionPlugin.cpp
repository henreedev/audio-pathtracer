#include "FrequenSeeAudioOcclusionPlugin.h"

#include "FrequenSeeAudioOcclusionSettings.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "HAL/UnrealMemory.h"


void FFrequenSeeAudioOcclusionPlugin::Initialize(const FAudioPluginInitializationParams InitializationParams)
{
    
    SamplingRate = InitializationParams.SampleRate;
    FrameSize = InitializationParams.BufferLength;

    // log
    UE_LOG(LogTemp, Warning, TEXT("Initializing occlusion plugin"));

    // Sources.AddDefaulted(InitializationParams.NumSources);
}

void FFrequenSeeAudioOcclusionPlugin::OnInitSource(const uint32 SourceId, const FName& AudioComponentUserId, const uint32 NumChannels, UOcclusionPluginSourceSettingsBase* InSettings)
{
    // log
    UE_LOG(LogTemp, Warning, TEXT("Initializing source %d"), SourceId);

}

void FFrequenSeeAudioOcclusionPlugin::OnReleaseSource(const uint32 SourceId)
{
    
}

void FFrequenSeeAudioOcclusionPlugin::ProcessAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData)
{
    // UFrequenSeeAudioComponent& Source = Sources[InputData.SourceId];
    // UE_LOG(LogTemp, Warning, TEXT("OCCLUSION PLUGIN PROCESSING"));
    const UAudioComponent* AudioComponent = UAudioComponent::GetAudioComponentFromID(InputData.AudioComponentId);
    const UFrequenSeeAudioComponent* FrequenSeeSourceComponent = AudioComponent->GetOwner()->FindComponentByClass<UFrequenSeeAudioComponent>();

    const float* InBufferData = InputData.AudioBuffer->GetData();
    float* OutBufferData = OutputData.AudioBuffer.GetData();

    const float OcclusionAttenuation = FrequenSeeSourceComponent->GetOcclusionAttenuation();
    // UE_LOG(LogTemp, Warning, TEXT("OCCLUSION ATTENUATION: %f"), OcclusionAttenuation);

    for (int SampleIndex = 0; SampleIndex < InputData.AudioBuffer->Num(); ++SampleIndex)
    {
        // OutBufferData[SampleIndex] = InBufferData[SampleIndex] * OcclusionAttenuation;
    }
}


FString FFrequenSeeAudioOcclusionPluginFactory::GetDisplayName()
{
    static FString DisplayName = FString(TEXT("FrequenSee Occlusion"));
    return DisplayName;
}

bool FFrequenSeeAudioOcclusionPluginFactory::SupportsPlatform(const FString& PlatformName)
{
    return true;
}

TAudioOcclusionPtr FFrequenSeeAudioOcclusionPluginFactory::CreateNewOcclusionPlugin(FAudioDevice* OwningDevice)
{
    // log
    UE_LOG(LogTemp, Warning, TEXT("Creating new occlusion plugin"));
    
    return MakeShared<FFrequenSeeAudioOcclusionPlugin>();
}

UClass* FFrequenSeeAudioOcclusionPluginFactory::GetCustomOcclusionSettingsClass() const
{
    return UFrequenSeeAudioOcclusionSettings::StaticClass();    
}
