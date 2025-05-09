#include "AcousticGeometryComponent.h"
#include "AudioRayTracingSubsystem.h"

void UAcousticGeometryComponent::OnRegister()
{
	Super::OnRegister();
	if (UWorld const* World = GetWorld())
	{
		if (UAudioRayTracingSubsystem* SubSys = World->GetSubsystem<UAudioRayTracingSubsystem>())
		{
			SubSys->RegisterGeometry(this);
		}
	}
}

void UAcousticGeometryComponent::OnUnregister()
{
	if (UWorld const* World = GetWorld())
	{
		if (UAudioRayTracingSubsystem* SubSys = World->GetSubsystem<UAudioRayTracingSubsystem>())
		{
			SubSys->UnregisterGeometry(this);
		}
	}
	Super::OnUnregister();
}