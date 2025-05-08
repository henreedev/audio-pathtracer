using System.IO;
using UnrealBuildTool;

public class FrequenSee : ModuleRules
{
	public FrequenSee(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"Projects",
			"Landscape",
			"AudioMixer",
			"AudioExtensions",
			"Synthesis", 
			// "CPPTest"
		});
				
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("UnrealEd");
		}
	}
}