﻿#include "AudioRayTracingSubsystem.h"

#include <unordered_map>

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

}

void UAudioRayTracingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Warning, TEXT("Initializing."));
    Super::Initialize(Collection);
}

void UAudioRayTracingSubsystem::Deinitialize()
{
    Super::Deinitialize();
    UE_LOG(LogTemp, Warning, TEXT("Deinitializing."));
}


void UAudioRayTracingSubsystem::RegisterSource(UFrequenSeeAudioComponent* InComp)
{
    ActiveSources.Add({ InComp });
}

void UAudioRayTracingSubsystem::UnRegisterSource(UFrequenSeeAudioComponent* InComp)
{
    ActiveSources.Remove({ InComp });
}

void UAudioRayTracingSubsystem::Tick(float DeltaTime)
{
    static float TimeBeforeFirstTick = 1.0f;
    if (TimeBeforeFirstTick >= 0.0f)
    {
        TimeBeforeFirstTick -= DeltaTime;
        return;
    }
    if (ActiveSources.Num() == 0) return;

    // 1) Get listener position
 

        // 2) Iterate sources (compact array each frame)
        ActiveSources.RemoveAllSwap([](const FActiveSource& S){ return !S.AudioComp.IsValid(); });
        // UE_LOG(LogTemp, Warning, TEXT("Active sources: %d"), ActiveSources.Num());

    // for (FActiveSource& Src : ActiveSources)
    // {
        // TraceAndApply(nullptr /*AudioDevice*/, ListenerLoc, Src);
        
    // }

    if (!PlayerPawn.IsValid())
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0); // returns APawn*
        if (!PlayerPawn.IsValid()) return; // still not spawned – just wait until next tick
    }

    UpdateSources(DeltaTime);
}

void UAudioRayTracingSubsystem::TraceAndApply(FAudioDevice* /*Device*/, const FVector& Listener, const FActiveSource& Src) const
{
    if (!Src.AudioComp.IsValid()) return;

    const FVector SourceLoc = Src.AudioComp->GetComponentLocation();

    int32 BlockedRays = 0;
    FCollisionQueryParams QParams(TEXT("AudioRay"), /*bTraceComplex*/ false);
    QParams.AddIgnoredActor(Src.AudioComp->GetOwner());

    TArray<FHitResult> Hits;
}

void UAudioRayTracingSubsystem::UpdateSources(float DeltaTime, bool bForceUpdate)
{
    if (TickVisualization)
    {
        if (VisualizeTimer <= 0.0f)
        {
            for (FActiveSource& Src : ActiveSources)
            {
                UpdateSource(Src);
                
                // Reset timer
                VisualizeTimer = VISUALIZE_DURATION;
            }
        } else
        {
            // Decrement timer
            VisualizeTimer -= DeltaTime;
        }
    }
    if (bForceUpdate)
    {
        for (FActiveSource& Src : ActiveSources)
        {
            UpdateSource(Src);
        }
    }
}

void UAudioRayTracingSubsystem::UpdateSource(FActiveSource& Src)
{
    // Visualize a few rays
    Visualize(Src);

    // Cast a lot more to update impulse response
    TArray<FSoundPath> ForwardPaths;
    TArray<FSoundPath> BackwardPaths;
    TArray<FSoundPath> ConnectedPaths;

    GenerateFullPaths(Src, ForwardPaths, BackwardPaths, ConnectedPaths);
        
    for (FSoundPath& Path : ForwardPaths)
    {
        EvaluatePath(Path);
    }
            
    for (FSoundPath& Path : BackwardPaths)
    {
        EvaluatePath(Path);
    }
    TArray<FPathEnergyResult> EnergyResults;
    for (FSoundPath& Path : ConnectedPaths)
    {
        EnergyResults.Add(EvaluatePath(Path));
    }

    // Place energy of connected paths into bins in Src's energy buffer
    // Flush it first
    if (Src.AudioComp.IsValid())
    {
        Src.AudioComp->FlushEnergyBuffer();
                    
    }
    // Place energy values
    // Normalize energy values based on total num rays
    float NormalizationFactor = 1.0f / (float) USED_RAY_COUNT;
    for (FPathEnergyResult& Result : EnergyResults)
    {
        if (Src.AudioComp.IsValid())
        {
            float Energy = Result.Gain;
            Energy *= NormalizationFactor;
            Src.AudioComp->AddEnergyAtDelay(Result.DelaySeconds, Energy);
        }
    }
    // Log contents of Src.AudioComp's EnergyBuffer 
    if (Src.AudioComp.IsValid())
    {
        // UE_LOG(LogTemp, Warning, TEXT("Energy Buffer contents:"));
        const TArray<float>& EnergyBuffer = Src.AudioComp->EnergyBuffer;
        for (int32 i = 0; i < EnergyBuffer.Num(); ++i)
        {
            if (EnergyBuffer[i] > 0.0f)
            {
                // UE_LOG(LogTemp, Warning, TEXT("Buffer[%d] = %f"), i, EnergyBuffer[i]);
            }
        }
    }

    // Update impulse response of audio component
    if (Src.AudioComp.IsValid())
    {
        Src.AudioComp->FlushEnergyBuffer();
        Src.AudioComp->ReconstructImpulseResponse();
    }
    
}


/** -------------------------- BIDIRECTIONAL PATH TRACING --------------------------- */


void UAudioRayTracingSubsystem::GenerateFullPaths(const FActiveSource& Src, TArray<FSoundPath>& ForwardPathsOut, TArray<FSoundPath>& BackwardPathsOut, TArray<FSoundPath>& ConnectedPathsOut, int NumRays)
{

    APawn* Listener = PlayerPawn.Get();
    if (!Listener)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player not found in GenerateFullPaths"));
        return;
    }

    AActor* PlayerPtr = PlayerPawn.Get();
    ForwardPathsOut.Reserve(NumRays);
    BackwardPathsOut.Reserve(NumRays);
    ConnectedPathsOut.Reserve(NumRays); 
    for (int i = 0; i < NumRays; ++i)
    {
        // Create and add forward path
        FSoundPath ForwardPath;
        GeneratePath(Src.AudioComp->GetOwner(), ForwardPath);
        ForwardPathsOut.Add(ForwardPath);

        // Create and add backward path
        FSoundPath BackwardPath;
        GeneratePath(PlayerPtr, BackwardPath);
        BackwardPathsOut.Add(BackwardPath);

        // Attempt connection, add if valid
        if (FSoundPath ConnectedPath; ConnectSubpaths(ForwardPath, BackwardPath, ConnectedPath))
            ConnectedPathsOut.Add(ConnectedPath);
    }
    // Print the number of connected paths
    UE_LOG(LogTemp, Warning, TEXT("%d paths connected out of %d"), ConnectedPathsOut.Num(), USED_RAY_COUNT);
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
        H, ForwardLastNode.Position, BackwardLastNode.Position - 0.1f * (BackwardLastNode.Position - ForwardLastNode.Position).GetSafeNormal(), ObjectParams
        ))
    {
        // Connect the paths
        
        // Reserve space
        OutPath.Nodes.Reset(ForwardPath.Nodes.Num() + BackwardPath.Nodes.Num());

        // Add nodes to out path
        OutPath.Nodes.Append(ForwardPath.Nodes);
        // Add nodes in reverse
        for (int i = BackwardPath.Nodes.Num( ) - 1; i >= 0; --i)
        {
            OutPath.Nodes.Add(BackwardPath.Nodes[i]);
        }
        OutPath.ForwardConnectionPos = ForwardLastNode.Position;
        OutPath.BackwardConnectionPos = BackwardLastNode.Position;
        
        // UE_LOG(LogTemp, Warning, TEXT("Connected subpaths!"));

        return true;
    }
    
    return false;
}

void UAudioRayTracingSubsystem::GeneratePath(const AActor* ActorToIgnore, FSoundPath& OutPath) const
{
    // Chance for a path to NOT terminate
    constexpr float RUSSIAN_ROULETTE_PROB = 0.9f;
    // Maximum distance of a single raycast
    constexpr float MAX_RAYCAST_DIST = 1000000.f;

    // State variables
    FVector CurrentPos = ActorToIgnore->GetActorLocation();
    auto Mesh = ActorToIgnore->GetComponentByClass<UStaticMeshComponent>();
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
        float RussianRoulette = FMath::FRand();
        if (RussianRoulette < RUSSIAN_ROULETTE_PROB)
        {
            // 2. Pick a random direction, calculate its probability FIXME assuming diffuse
            FVector Dir;
            if (CurrentNormal.IsNearlyZero())
            {
                Dir = FMath::VRand();
                float PDF = 1.0f / (4.0f * PI);
                CurrentProbability = PDF * RUSSIAN_ROULETTE_PROB;
            } else
            {
                Dir = FMath::VRandCone(CurrentNormal, FMath::DegreesToRadians(90.f));
                // Probability of an angle out of 2PI steradians (hemisphere)
                float CosTheta = FVector::DotProduct(Dir, CurrentNormal); // assumed normalized
                float PDF = CosTheta / PI;
                CurrentProbability = PDF * RUSSIAN_ROULETTE_PROB;
            }
            // 3. Shoot a ray, call GeneratePath recursively at impact point

            // Ignore self
            FCollisionQueryParams QParams(TEXT("AudioRay"), /*bTraceComplex*/ false);
            QParams.AddIgnoredActor(ActorToIgnore);
            if (Mesh)
            {
                QParams.AddIgnoredComponent(Mesh);
            }
            

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
                CurrentPos = H.ImpactPoint + 0.1 * H.ImpactNormal;
                CurrentNormal = H.ImpactNormal;
                CurrentMaterial = H.GetActor()->FindComponentByClass<UAcousticGeometryComponent>();
            }
        } else
        {
            // UE_LOG(LogTemp, Warning, TEXT("Russian roulette failed!"));
            break; // Failed russian roulette
        }
    }
}



// Also updates the FSoundPath's TotalLength field based on calculated distance. 
FPathEnergyResult UAudioRayTracingSubsystem::EvaluatePath(FSoundPath& Path) const
{
    constexpr float SoundSpeed = 343.0f;
    float Distance = 0.0f;
    float ScaledDistance = 0.0f;
    float Energy = 1.0f;
    float Probability = 1.0f;
    
    for (int i = 0; i < Path.Nodes.Num() - 1; ++i)
    {
        FSoundPathNode& Node = Path.Nodes[i];
        FSoundPathNode& NextNode = Path.Nodes[i + 1];
        Distance += FVector::Dist(Node.Position, NextNode.Position);
        float NodeDistance = FVector::Dist(Node.Position, NextNode.Position) / 1000.f;
        ScaledDistance += NodeDistance;
        if (NodeDistance < 1.0f)
        {
            continue; // Don't allow glitched collisions in-place to contribute
        }
        float NodeDistanceSqr = NodeDistance * NodeDistance;
        
        // diffuse = reflectivity / PI, where reflectivity is 0.0-1.0
        float BSDFFactor = 1.0f;
        if (Node.Material.IsValid() and Node.Material.Get()->Material)
        {
            BSDFFactor = Node.Material.Get()->Material->Absorption[2].Value / PI;
        }
        // Multiply cosines of angles , divide by squared distance
        // float GeometryTerm = (float) FMath::Cos(Node.Normal.X) * FMath::Cos(Node.Normal.X) / FMath::Square(Distance);
        // FVector Direction = NextNode.Position - Node.Position.GetSafeNormal();
        
        float GeometryTerm = 1.0f / (4 * PI * NodeDistanceSqr);
        Energy *= BSDFFactor;
        Energy *= GeometryTerm;
        // Apply media term (equation 3)
        constexpr float AIR_ABSORPTION_FACTOR = 0.05;
        float MediaAbsorption = exp(-AIR_ABSORPTION_FACTOR * NodeDistance);
        Energy *= MediaAbsorption;
        Energy /= powf(Node.Probability, 0.1);
        Probability *= Node.Probability;
        
        if (Energy > USED_RAY_COUNT)
        {
            int j = 1;
        }
        // Log Everything
        
    }

    // Clamp energy
    Energy = FMath::Min(Energy, 1.0f);

    // Un-normalize energy (FIXME)
    Energy *= 10.f;
    
    // Update path values
    Path.TotalLength = Distance;
    Path.EnergyContribution = Energy;

    return {ScaledDistance / SoundSpeed, Energy};
}





/** -------------------------- ADDITIONS BY IS --------------------------- */
void UAudioRayTracingSubsystem::Is_GeneratePath(const AActor* ActorToIgnore, FSoundPath& OutPath, int bounces, bool fwd)
{

    // Maximum distance of a single raycast
    constexpr float MAX_RAYCAST_DIST = 1000000.f;

    // State variables
    FVector CurrentPos = ActorToIgnore->GetActorLocation();
    auto Mesh = ActorToIgnore->GetComponentByClass<UStaticMeshComponent>();
    FVector CurrentNormal = FVector::ZeroVector;
    auto CurrentMaterial = TWeakObjectPtr<UAcousticGeometryComponent>(nullptr);
    float CurrentProbability = 1.0f;
    
    // REPEAT:  
    for (int i=0; i <= bounces; i++)
    {
        // 0. Add current position as a node in the path
        FSoundPathNode Node(CurrentPos, CurrentNormal, CurrentMaterial, CurrentProbability);
        OutPath.Nodes.Add(Node);
        
            // 1. Pick a random direction, calculate its probability FIXME assuming diffuse
            FVector Dir;
            if (CurrentNormal.IsNearlyZero())
            {
                Dir = FMath::VRand();
                float PDF = 1.0f / (4.0f * PI);
                CurrentProbability = PDF;
            } else
            {
                Dir = FMath::VRandCone(CurrentNormal, FMath::DegreesToRadians(90.f));
                // Probability of an angle out of 2PI steradians (hemisphere)
                float CosTheta = FVector::DotProduct(Dir, CurrentNormal); // assumed normalized
                float PDF = CosTheta / PI;
                CurrentProbability = PDF;
            }
            // 3. Shoot a ray, call GeneratePath recursively at impact point

            // Ignore self
            FCollisionQueryParams QParams(TEXT("AudioRay"), /*bTraceComplex*/ false);
            QParams.AddIgnoredActor(ActorToIgnore);
            if (Mesh)
            {
                QParams.AddIgnoredComponent(Mesh);
            }
        
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
                CurrentPos = H.ImpactPoint + 0.1 * H.ImpactNormal;
                CurrentNormal = H.ImpactNormal;
                CurrentMaterial = H.GetActor()->FindComponentByClass<UAcousticGeometryComponent>();
            }
    }

    //add path to array of paths corresponding to this number of bounces
    if (fwd)
    {
        subpathsOfBounceFwd[bounces].emplace_back(OutPath);
    }
    else
    {
        subpathsOfBounceBwd[bounces].emplace_back(OutPath);
    }
}

float getPathGenProbability(FSoundPath& Path)
{
    float prod = 1.f;
    for (auto node : Path.Nodes)
    {
        prod *= node.Probability;
    }
    return prod;
}

/*Connect every bounce of every possible sample in forward dir with every bounce of every possible sample in backward dir (see Equation 12)*/
void UAudioRayTracingSubsystem::Is_NaiveConnections()
{
    //for each bounce in the forward dir 
    for (int i=0; i < maxBounces; i++)
    {
        // for each i-bounce path in forward dir
        for (int pathsF = 0; pathsF < bounceToSamples[i]; pathsF++)
        {
            auto pathF = subpathsOfBounceFwd[i][pathsF];
            //for each bounce in backward dir
            for (int j=0; j < maxBounces; j++)
            {
                // for each i-bounce path in forward dir
                for (int pathsB = 0; pathsB < bounceToSamples[j]; pathsB++)
                {
                    auto pathB = subpathsOfBounceFwd[j][pathsB];
                    FSoundPath connectedPath;
                        //if connection is successful
                    if (ConnectSubpaths(pathF, pathB, connectedPath))
                    {
                        samples[i][j].emplace_back(connectedPath);
                        totalSamples++;
                    }
                }
            }
        }
    }
}

float getPdf(FSoundPathNode a, FSoundPathNode b)
{
    return 0.9;     //hardcoding for now
}

float UAudioRayTracingSubsystem::getExpectedWeight(int fwdNode, int bwdNode)
{
    float sum = 0.f;
    for (int i=0; i < fwdNode; i++)
    {
        for (int j=0; j < bwdNode; j++)
        {
            float Cj = samples[i][j].size()/totalSamples;
            float avgProb = 0.f;
            for (FSoundPath path : samples[i][j])
            {
                avgProb += getPathGenProbability(path);
            }
            avgProb /= 3.f;
            sum += Cj * avgProb;
        }
    }

    return sum;
}



//The weight considers other strategies that could have produced the same path.
float UAudioRayTracingSubsystem::MISEnergy(int i)
{
    int N = totalSamples;
    std::unordered_map<int,int> weightsMap{};
    
    float outerSum = 0.f;
    for (int j = 0; j < i; ++j)
    {
        float Cj = samples[i][j].size()/totalSamples;
        float expectedWeight = getExpectedWeight(i, j);

        float innerSum = 0.f;
        
        for (int k = 0; k < samples[i][j].size(); ++k)
        {
            FSoundPath sample = samples[i][j][k];
            float prob = getPathGenProbability(sample);
            float estimator = EvaluatePath(sample).Gain/prob;
            float weight = Cj * prob;       //not totally confident in using Cj here
            innerSum += weight * estimator;
        }
        outerSum += innerSum/Cj;
    }
    return outerSum/(float) N;
}

/** -------------------------- PATH VISUALIZATION --------------------------- */
/**
 * Draws a segmented line from Start to End at the given speed. 
 * Line segments need to be manually cleaned up later if bPersistent is true.
 * GetWorld()->FlushPersistentDebugLines();
 */

float UAudioRayTracingSubsystem::DrawSegmentedLineAdvanced(
        const FVector& Start, const FVector& End,
        float Speed, FColor BaseColor,
        float DrawDelay, bool bPersistent) 
{
    const FVector Diff  = End - Start;
    const float   Dist  = Diff.Length();
    const FVector Dir   = Diff / Dist;

    const float DeltaT      = 1.f / DEBUG_RAY_FPS;
    const float Step        = Speed * DeltaT;          // world units per “tick”
    float       travelled   = 0.f;                     // distance already covered
    float       timePassed  = 0.f;                     // accumulates the delay

    UWorld* World = GetWorld();

    while (travelled < Dist)
    {
        const float segmentLen = FMath::Min(Step, Dist - travelled);
        const FVector CurrentPos = Start + Dir * travelled;
        const FVector NextPos    = CurrentPos + Dir * segmentLen;

        /* -------- gradient colour (dark→full colour) -------- */
        const float tSegmentEnd = (travelled + segmentLen) / Dist;      // 0‥1
        FLinearColor linA       = FLinearColor(BaseColor);
        linA *= tSegmentEnd;                                            // dark‑to‑bright
        FColor SegColor          = linA.ToFColor(/*bSRGB*/ true);
        /* ------------------------------------------------------- */

        FTimerHandle Dummy;   // we don’t need to keep the handle if we never cancel the timer
        World->GetTimerManager().SetTimer(
            Dummy,
            FTimerDelegate::CreateLambda([=, this]()
            {
                DrawDebugLine(
                    GetWorld(),
                    CurrentPos,
                    NextPos,
                    SegColor,
                    bPersistent,
                    /*LifeTime*/ 1000.f,
                    /*Depth*/    0,
                    /*Thickness*/1.f);
            }),
            (timePassed == 0.f) ? 1e-6f : timePassed,
            /*bLoop*/ false);

        travelled  += segmentLen;
        timePassed += DeltaT;
    }

    return timePassed;
}

float UAudioRayTracingSubsystem::DrawSegmentedLine(FVector& Start, FVector& End, float Speed, FColor Color,
                                                   float DrawDelay, bool bPersistent)
{
    if (Speed <= 0.f)
    {
        return 0.f;
    }
    
    // Difference vector
    FVector Diff = End - Start;
    FVector Dir = Diff.GetSafeNormal();
    float Dist = Diff.Length();

    if (Dist < 0.01f)
    {
        return 0.f;
    }
    FVector CurrentPos = Start;

    
    const float DELTATIME = 1.0f / DEBUG_RAY_FPS;
    FVector Increment = Dir * Speed * DELTATIME;
    float DistIncrement = Increment.Length();
    
    float TimePassed = 0.0f;
    float DistTravelled = 0.f;   
    auto World = GetWorld();
    // Simulate ticks with fixed delta time, drawing a ray segment on each tick with delay and length based on time passed
    while (DistTravelled < Dist)
    {
        // Find end of ray segment
        FVector NextPos = CurrentPos + Increment;
        DistTravelled += DistIncrement;
        if (DistTravelled >= Dist)
        {
            NextPos = End;
        }
        
        // Draw ray segment with delay equal to total time passed 
        FTimerHandle TimerHandle;
        float Delay = TimePassed;
        UE_LOG(LogTemp, Warning, TEXT("Delay: %f"), Delay);
        World->GetTimerManager().SetTimer(
        	TimerHandle,
        	FTimerDelegate::CreateLambda([=, this]()
        	{
                DrawDebugLine(
                    GetWorld(),
                    CurrentPos,
                    NextPos,
                    Color,
                    bPersistent,
                        1000.f,
                    0,
                    10.0f
                );
        	}),
        	(DrawDelay + Delay) == 0 ? 0.0000001f : DrawDelay + Delay,
        	false
        );

        // Increment current position
        CurrentPos = NextPos;

        // Increment time passed
        TimePassed += DELTATIME;

        if (TimePassed > 5.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("TimePassed: %f"), TimePassed);
            break;
        }
    }
    return TimePassed;
}


/**
 * Given an FSoundPath, visualizes its entire travel over the course of the given duration.
 * Does so by calling Unreal's DrawDebugLine for each segment travelled along the path as time passes.
 * Line segments need to be manually cleaned up later if bPersistent is true.
 */
void UAudioRayTracingSubsystem::VisualizePath(const FSoundPath& Path, float Duration, FColor Color, bool bPersistent)
{
    float Speed = Path.TotalLength / Duration;
    float TotalTimePassed = 0.0f;
    // Draw each line segment, adding the time it takes to the 
    for (int i = 0; i < Path.Nodes.Num() - 1; ++i)
    {
        // Get node pair
        auto CurrentNode = Path.Nodes[i];
        auto NextNode = Path.Nodes[i + 1];

        // Draw line between them at a fixed speed, returning how long it takes to draw the line at that pace
        // Delay the drawing by time taken for previous lines so far
        // UE_LOG(LogTemp, Warning, TEXT("Speed: %f"), Speed);
        float TimePassed = DrawSegmentedLine(CurrentNode.Position, NextNode.Position, Speed, Color, TotalTimePassed, bPersistent);

        // Add time taken to total count for future delays
        TotalTimePassed += TimePassed;
    }
}

/**
 * Given forward, backward, and connected paths, shows the complete process of:
 * 1. Sending out forward and backward paths
 * 2. Connecting them at their endpoints -- add a new, different-colored debug line between endpoints
 * 3. Evaluating their contribution -- redraw paths colored by their energy contribution
 * Apportions some of the total duration to each step. 
 */
void UAudioRayTracingSubsystem::VisualizeBDPT(const TArray<FSoundPath>& ForwardPaths, const TArray<FSoundPath>& BackwardPaths, const TArray<FSoundPath>& ConnectedPaths, float TotalDuration) 
{
    TotalDuration -= 0.1f; // 100ms buffer
    float DrawPathsDuration = TotalDuration * 0.4f;
    float ShowConnectionDuration = TotalDuration * 0.3f;
    float ShowContributionDuration = TotalDuration * 0.3f;
    
    // 1. Draw forward + backward paths at the same time
    // Draw forward paths
    for (const FSoundPath& Path : ForwardPaths)
    {
        VisualizePath(Path, DrawPathsDuration, FColor::Green, true);
    }
    // Draw backward paths
    for (const FSoundPath& Path : BackwardPaths)
    {
        VisualizePath(Path, DrawPathsDuration, FColor::Orange, true);
    }
    
    // 2. After doing above, Draw the connection line gradually
    FTimerHandle ConnectedPathsTimerHandle;
    TArray<FSoundPath> ConnectedPathsCopy = ConnectedPaths;
    GetWorld()->GetTimerManager().SetTimer(
        ConnectedPathsTimerHandle,
        FTimerDelegate::CreateLambda([this, ConnectedPathsCopy]() mutable
        {
            if (!IsValid(this)) return;
            for (const FSoundPath& Path : ConnectedPathsCopy)
            {
                // if (!Path.ForwardConnectionPos || !Path.BackwardConnectionPos) continue;
                FVector A = Path.ForwardConnectionPos;
                FVector B = Path.BackwardConnectionPos;
                float Distance = FVector::Dist(A, B);
                DrawSegmentedLine(A, B, Distance / 0.15f, FColor::Blue, 0.0f, true);
            }
        }),
        DrawPathsDuration,
        false
    );
    
    // 3. After doing above, erase existing debug lines and draw the connected paths BASED ON THEIR ENERGY
    FTimerHandle ContributionTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        ContributionTimerHandle,
        FTimerDelegate::CreateLambda([=, this]()
        {
            if (!IsValid(this)) return;
            
            FlushPersistentDebugLines(GetWorld());
            float MaxEnergy = 0.0f;
            for (const FSoundPath& Path : ConnectedPaths)
            {
                MaxEnergy = FMath::Max(MaxEnergy, Path.EnergyContribution);
            }
            for (const FSoundPath& Path : ConnectedPaths)
            {
                // Get path energy
                float Energy = Path.EnergyContribution / MaxEnergy; 

                // Lerp its color between bright green and dark red based on this energy
                // Start and end colors
                FLinearColor DarkRed = FLinearColor(FColor(200, 0, 0));
                FLinearColor Green = FLinearColor(FColor::Green);

                // Lerp in HSV space
                FColor EnergyColor = FLinearColor::LerpUsingHSV(DarkRed, Green, Energy).ToFColor(true);

                // Draw using energy color
                VisualizePath(Path, 1.0f, EnergyColor, true);
            }
        }),
        DrawPathsDuration + ShowConnectionDuration,
        false);

    // 4. Delete debug lines after duration ends
    FTimerHandle DeleteTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        DeleteTimerHandle,
        FTimerDelegate::CreateLambda([=, this]()
        {
            if (IsValid(this))
            {
                FlushPersistentDebugLines(GetWorld());
            }
        }),
        DrawPathsDuration + ShowConnectionDuration + ShowContributionDuration, false
    );
}

// Visualizes the given number of forward/backward ray pairs
void UAudioRayTracingSubsystem::Visualize(FActiveSource& Src, int RayCount, float Duration)
{
    TArray<FSoundPath> ForwardPaths;
    TArray<FSoundPath> BackwardPaths;
    TArray<FSoundPath> ConnectedPaths;
    GenerateFullPaths(Src, ForwardPaths, BackwardPaths, ConnectedPaths, RayCount);
    
    for (FSoundPath& Path : ForwardPaths)
    {
        EvaluatePath(Path);
    }
    for (FSoundPath& Path : BackwardPaths)
    {
        EvaluatePath(Path);
    }
    for (FSoundPath& Path : ConnectedPaths)
    {
        EvaluatePath(Path);
    }
    VisualizeBDPT(ForwardPaths, BackwardPaths, ConnectedPaths, Duration);
}

// Calls UpdateSources with bForceUpdate = true
void UAudioRayTracingSubsystem::ForceUpdateSources()
{
    UpdateSources(0.0f, true);
}

