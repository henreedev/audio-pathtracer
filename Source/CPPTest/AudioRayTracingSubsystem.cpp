#include "AudioRayTracingSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "FrequenSeeAudioComponent.h"
#include "Engine/World.h"

float AverageArray(const TArray<float>& Values)
{
    if (Values.Num() == 0) return 0.0f;
    float Sum = 0.0f;
    for (const float Value : Values) Sum += Value;
    return Sum / Values.Num();
}

float AverageArray(const TArray<FAcousticBand>& Values)
{
    if (Values.Num() == 0) return 0.0f;
    float Sum = 0.0f;
    for (const FAcousticBand Value : Values) Sum += Value.Value;
    return Sum / Values.Num();
}

UAudioRayTracingSubsystem::UAudioRayTracingSubsystem()
{

    Super();
    Player = nullptr;
    for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    {
        Player = Cast<ADefaultPawn>(*It);
        break; // Only need the first one
    }
}

void UAudioRayTracingSubsystem::RegisterSource(UFrequenSeeAudioComponent* InComp)
{
    ActiveSources.Add({ InComp });
}

void UAudioRayTracingSubsystem::UnRegisterSource(UFrequenSeeAudioComponent* InComp)
{
    ActiveSources.Remove({ InComp });
}

void UAudioRayTracingSubsystem::Tick(float /*DeltaTime*/)
{
    if (ActiveSources.Num() == 0) return;

    // 1) Get listener position
    FVector ListenerLoc;
    FRotator ListenerRot;
    UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0)->GetCameraViewPoint(ListenerLoc, ListenerRot);

    // 2) Iterate sources (compact array each frame)
    ActiveSources.RemoveAllSwap([](const FActiveSource& S){ return !S.AudioComp.IsValid(); });

    // for (FActiveSource& Src : ActiveSources)
    // {
        // TraceAndApply(nullptr /*AudioDevice*/, ListenerLoc, Src);
        
    // }

    UpdateSources();
}

void UAudioRayTracingSubsystem::TraceAndApply(FAudioDevice* /*Device*/, const FVector& Listener, const FActiveSource& Src) const
{
    if (!Src.AudioComp.IsValid()) return;

    const FVector SourceLoc = Src.AudioComp->GetComponentLocation();

    int32 BlockedRays = 0;
    FCollisionQueryParams QParams(TEXT("AudioRay"), /*bTraceComplex*/ false);
    QParams.AddIgnoredActor(Src.AudioComp->GetOwner());

    TArray<FHitResult> Hits;
    for (int32 i = 0; i < NumRays; ++i)
    {
        // Uniformly jitter direction in a cone around the direct path (quick & dirty)
        const FVector Dir     = (SourceLoc - Listener).GetSafeNormal();
        const FVector Jitter  = FMath::VRandCone(Dir, FMath::DegreesToRadians(15.f));
        const FVector End     = Listener + Jitter * 5000.f;

        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(
                Hit, Listener, End, ECC_Visibility, QParams))
        {
            ++BlockedRays;
            if (Hit.bBlockingHit) Hits.Add(Hit);

#if WITH_EDITOR
            DrawDebugLine(GetWorld(), Listener, Hit.ImpactPoint, FColor::Red, false, 0.05f, 0, 0.5f);
#endif
        }
#if WITH_EDITOR
        else
        {
            DrawDebugLine(GetWorld(), Listener, End, FColor::Green, false, 0.05f, 0, 0.5f);
        }
#endif
    }
}

void UAudioRayTracingSubsystem::UpdateSources()
{
    for (FActiveSource& Src : ActiveSources)
    {
        TArray<FSoundPath> OutPaths;
        GenerateFullPaths(Src, OutPaths);
    }
}

/** -------------------------- BIDIRECTIONAL PATH TRACING --------------------------- */
void UAudioRayTracingSubsystem::GenerateFullPaths(const FActiveSource& Src, TArray<FSoundPath>& OutPaths)
{

    constexpr int iterations = 100;
    const auto PlayerLoc = Player->GetOwner();
    OutPaths.Reserve(iterations);
    for (int i = 0; i < iterations; ++i)
    {
        FSoundPath ForwardPath;
        GeneratePath(Src.AudioComp->GetOwner(), ForwardPath);
        FSoundPath BackwardPath;
        GeneratePath(PlayerLoc, BackwardPath);
        if (FSoundPath FullPath; ConnectSubpaths(ForwardPath, BackwardPath, FullPath))
            OutPaths.Add(FullPath);
    }
    
}

bool UAudioRayTracingSubsystem::ConnectSubpaths(FSoundPath& ForwardPath, FSoundPath& BackwardPath, FSoundPath& OutPath)
{
    if (ForwardPath.Nodes.IsEmpty() || BackwardPath.Nodes.IsEmpty()) return false;
    FSoundPathNode& ForwardLastNode = ForwardPath.Nodes.Last();
    FSoundPathNode& BackwardLastNode = BackwardPath.Nodes.Last();

    // RAYCAST BETWEEN LAST NODES
    // Collide with various channels
    FCollisionObjectQueryParams ObjectParams;
    ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic); // covers walls/floors
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic); // covers dynamic props

    // Store hit result
    FHitResult H;

    // Raycast in chosen direction -- CHECK IF IT **DOESN'T** HIT
    if (not GetWorld()->LineTraceSingleByObjectType(
        H, ForwardLastNode.Position, BackwardLastNode.Position, ObjectParams
        ))
    {
        // Connect the paths
        
        // Reserve space
        OutPath.Nodes.Reserve(ForwardPath.Nodes.Num() + BackwardPath.Nodes.Num());

        // Add nodes to out path
        OutPath.Nodes.Append(ForwardPath.Nodes);
        // Add nodes in reverse
        for (int i = BackwardPath.Nodes.Num() - 1; i >= 0; --i)
        {
            OutPath.Nodes.Add(BackwardPath.Nodes[i]);
        }
        UE_LOG(LogTemp, Warning, TEXT("Connected subpaths!"));
        return true;
    }


    return false;
}

void UAudioRayTracingSubsystem::GeneratePath(const AActor* ActorToIgnore, FSoundPath& OutPath) const
{
    // Chance for a path to NOT terminate
    const float RUSSIAN_ROULETTE_PROB = 0.95f;
    // Maximum distance of a single raycast
    const float MAX_RAYCAST_DIST = 1000000.f;

    // State variables
    FVector CurrentPos = ActorToIgnore->GetActorLocation();
    FVector CurrentNormal = FVector::ZeroVector;
    auto CurrentMaterial = TWeakObjectPtr<UAcousticGeometryComponent>(nullptr);
    float CurrentProbability = 1.0f;
    
    // REPEAT:  
    while (true)
    {
        // 0. Add current position as a node in the path
        FSoundPathNode Node(CurrentPos, CurrentNormal, CurrentMaterial, CurrentProbability);
        OutPath.Nodes.Add(Node);
        
        // 1. Check russian roulette probability -- if successful:
        if (FMath::FRand() > RUSSIAN_ROULETTE_PROB)
        {
            // 2. Pick a random direction, calculate its probability FIXME assuming diffuse
            FVector Dir;
            if (CurrentNormal.IsNearlyZero())
            {
                // Pick a random direction in sphere
                Dir = FMath::VRandCone(FVector::UpVector, FMath::DegreesToRadians(180.f));
                // Probability of an angle out of 4PI steradians
                float Prob = 1.0f / (4.0f * PI);
                CurrentProbability = Prob * RUSSIAN_ROULETTE_PROB;
            } else
            {
                Dir = FMath::VRandCone(CurrentNormal, FMath::DegreesToRadians(90.f));
                // Probability of an angle out of 2PI steradians (hemisphere)
                float Prob = 1.0f / (2.0f * PI);
                CurrentProbability = Prob * RUSSIAN_ROULETTE_PROB;
            }
            // 3. Shoot a ray, call GeneratePath recursively at impact point

            // Ignore self
            FCollisionQueryParams QParams(TEXT("AudioRay"), /*bTraceComplex*/ false);
            QParams.AddIgnoredActor(ActorToIgnore);

            // Collide with various channels
            FCollisionObjectQueryParams ObjectParams;
            ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
            ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic); // covers walls/floors
            ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic); // covers dynamic props

            // Store hit result
            FHitResult H;

            // Raycast in chosen direction
            if (GetWorld()->LineTraceSingleByObjectType(
                H, CurrentPos, CurrentPos + Dir * MAX_RAYCAST_DIST, ObjectParams, QParams
                ))
            {
                // 4. If hit, update current position and normal
                CurrentPos = H.ImpactPoint;
                CurrentNormal = H.ImpactNormal;
                CurrentMaterial = H.GetActor()->FindComponentByClass<UAcousticGeometryComponent>();
            }
        } else
        {
            break; // Failed russian roulette
        }
    }
}

FPathEnergyResult UAudioRayTracingSubsystem::EvaluatePath(FSoundPath& Path) const
{
    constexpr float SoundSpeed = 343.0f;
    float Distance = 0.0f;
    float Energy = 1.0f;

    for (int i = 0; i < Path.Nodes.Num() - 1; ++i)
    {
        FSoundPathNode& Node = Path.Nodes[i];
        FSoundPathNode& NextNode = Path.Nodes[i + 1];
        Distance += FVector::Dist(Node.Position, NextNode.Position);

        // diffuse = reflectivity / PI, where reflectivity is 0.0-1.0
        float BSDFFactor = 1.0f;
        if (Node.Material.IsValid())
        {
            BSDFFactor = Node.Material.Get()->Material->Absorption[2].Value / PI;
        }
        // TODO Multiply cosines of angles , divide by squared distance
        float GeometryTerm = (float) FMath::Cos(Node.Normal.Y) * FMath::Cos(Node.Normal.Y) / FMath::Square(Distance);

        Energy *= BSDFFactor;
        Energy *= GeometryTerm;
    }
    // Apply media term (equation 3)
    constexpr float AIR_ABSORPTION_FACTOR = 0.005;
    float MediaAbsorption = exp(-AIR_ABSORPTION_FACTOR * Distance);
    Energy *= MediaAbsorption;
    
    return {Distance / SoundSpeed, Energy};
}
