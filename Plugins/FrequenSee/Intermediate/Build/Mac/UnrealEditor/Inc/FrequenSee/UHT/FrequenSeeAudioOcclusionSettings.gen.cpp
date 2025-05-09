// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "FrequenSee/Public/FrequenSeeAudioOcclusionSettings.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeFrequenSeeAudioOcclusionSettings() {}

// Begin Cross Module References
AUDIOEXTENSIONS_API UClass* Z_Construct_UClass_UOcclusionPluginSourceSettingsBase();
FREQUENSEE_API UClass* Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings();
FREQUENSEE_API UClass* Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_NoRegister();
UPackage* Z_Construct_UPackage__Script_FrequenSee();
// End Cross Module References

// Begin Class UFrequenSeeAudioOcclusionSettings
void UFrequenSeeAudioOcclusionSettings::StaticRegisterNativesUFrequenSeeAudioOcclusionSettings()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UFrequenSeeAudioOcclusionSettings);
UClass* Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_NoRegister()
{
	return UFrequenSeeAudioOcclusionSettings::StaticClass();
}
struct Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "FrequenSeeAudioOcclusionSettings.h" },
		{ "ModuleRelativePath", "Public/FrequenSeeAudioOcclusionSettings.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UFrequenSeeAudioOcclusionSettings>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UOcclusionPluginSourceSettingsBase,
	(UObject* (*)())Z_Construct_UPackage__Script_FrequenSee,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics::ClassParams = {
	&UFrequenSeeAudioOcclusionSettings::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	nullptr,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	0,
	0,
	0x003030A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics::Class_MetaDataParams), Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings()
{
	if (!Z_Registration_Info_UClass_UFrequenSeeAudioOcclusionSettings.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UFrequenSeeAudioOcclusionSettings.OuterSingleton, Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UFrequenSeeAudioOcclusionSettings.OuterSingleton;
}
template<> FREQUENSEE_API UClass* StaticClass<UFrequenSeeAudioOcclusionSettings>()
{
	return UFrequenSeeAudioOcclusionSettings::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UFrequenSeeAudioOcclusionSettings);
UFrequenSeeAudioOcclusionSettings::~UFrequenSeeAudioOcclusionSettings() {}
// End Class UFrequenSeeAudioOcclusionSettings

// Begin Registration
struct Z_CompiledInDeferFile_FID_emrearslan_Desktop_hw_cs2240_audio_pathtracer_Plugins_FrequenSee_Source_FrequenSee_Public_FrequenSeeAudioOcclusionSettings_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UFrequenSeeAudioOcclusionSettings, UFrequenSeeAudioOcclusionSettings::StaticClass, TEXT("UFrequenSeeAudioOcclusionSettings"), &Z_Registration_Info_UClass_UFrequenSeeAudioOcclusionSettings, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UFrequenSeeAudioOcclusionSettings), 2063645828U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_emrearslan_Desktop_hw_cs2240_audio_pathtracer_Plugins_FrequenSee_Source_FrequenSee_Public_FrequenSeeAudioOcclusionSettings_h_21311502(TEXT("/Script/FrequenSee"),
	Z_CompiledInDeferFile_FID_emrearslan_Desktop_hw_cs2240_audio_pathtracer_Plugins_FrequenSee_Source_FrequenSee_Public_FrequenSeeAudioOcclusionSettings_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_emrearslan_Desktop_hw_cs2240_audio_pathtracer_Plugins_FrequenSee_Source_FrequenSee_Public_FrequenSeeAudioOcclusionSettings_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
