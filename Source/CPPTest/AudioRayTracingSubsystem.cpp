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

    // Super();
    // Player = nullptr;
    // for (TActorIterator<APawn> It(GetWorld()); It; ++It)
    // {
    //     Player = Cast<ADefaultPawn>(*It);
    //     break; // Only need the first one
    // }
}

void UAudioRayTracingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Warning, TEXT("Initializing."));
    Super::Initialize(Collection);
    
    // Find the first DefaultPawn in the world
    // if (UWorld* World = GetWorld())
    // {
    //     UE_LOG(LogTemp, Warning, TEXT("World found: %s"), *World->GetName());
    //     for (TActorIterator<APawn> It(World); It; ++It)
    //     {
    //         Player = Cast<ADefaultPawn>(UGameplayStatics::GetPlayerPawn(World, 0));
    //         Player = Cast<ADefaultPawn>(*It);
    //         if (Player)
    //         {
    //             UE_LOG(LogTemp, Log, TEXT("Player pawn found: %s"), *Player->GetName());
    //             break;
    //         }
    //     }
    // }
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
    UE_LOG(LogTemp, Warning, TEXT("Ticking!"));

    // 1) Get listener position
 

    // 2) Iterate sources (compact array each frame)
    ActiveSources.RemoveAllSwap([](const FActiveSource& S){ return !S.AudioComp.IsValid(); });

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

void UAudioRayTracingSubsystem::UpdateSources(float DeltaTime)
{
    for (FActiveSource& Src : ActiveSources)
    {
        TArray<FSoundPath> ForwardPaths;
        TArray<FSoundPath> BackwardPaths;
        TArray<FSoundPath> ConnectedPaths;
        GenerateFullPaths(Src, ForwardPaths, BackwardPaths, ConnectedPaths);
        for (FSoundPath& Path : ConnectedPaths)
        {
            EvaluatePath(Path);
        }
        if (VisualizeTimer <= 0.0f)
        {
            VisualizeBDPT(ForwardPaths, BackwardPaths, ConnectedPaths, VISUALIZE_DURATION);

            // Reset timer
            VisualizeTimer = VISUALIZE_DURATION;
        } else
        {
            // Decrement timer
            VisualizeTimer -= DeltaTime;
        }
    }
}

/** -------------------------- BIDIRECTIONAL PATH TRACING --------------------------- */
void UAudioRayTracingSubsystem::GenerateFullPaths(const FActiveSource& Src, TArray<FSoundPath>& ForwardPathsOut, TArray<FSoundPath>& BackwardPathsOut, TArray<FSoundPath>& ConnectedPathsOut)
{

    APawn* Listener = PlayerPawn.Get();
    if (!Listener)
    {
        UE_LOG(LogTemp, Warning, TEXT("Player not found in GenerateFullPaths"));
        return;
    }
    constexpr int iterations = 100;
    
    // if (!Player)
    // {
    //     // Find the first DefaultPawn in the world
    //     if (UWorld* World = GetWorld())
    //     {
    //         UE_LOG(LogTemp, Warning, TEXT("World found: %s"), *World->GetName());
    //         for (TActorIterator<APawn> It(World); It; ++It)
    //         {
    //             UE_LOG(LogTemp, Log, TEXT("Actor found: %s"), *It->GetName());
    //             Player = Cast<ADefaultPawn>(*It);
    //             Player = Cast<ADefaultPawn>(UGameplayStatics::GetPlayerPawn(World, 0));
    //             if (Player)
    //             {
    //                 UE_LOG(LogTemp, Log, TEXT("Player pawn found: %s"), *Player->GetName());
    //                 break;
    //             }
    //         }
    //     }
    // };

    AActor* PlayerPtr = PlayerPawn.Get();
    ForwardPathsOut.Reserve(iterations);
    BackwardPathsOut.Reserve(iterations);
    ConnectedPathsOut.Reserve(iterations); 
    for (int i = 0; i < iterations; ++i)
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
    UE_LOG(LogTemp, Warning, TEXT("%d paths connected out of %d"), ConnectedPathsOut.Num(), iterations);
    // UE_LOG(LogTemp, Warning, TEXT("Generated full paths!"));
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
        OutPath.ConnectionNodeForward = &ForwardLastNode;
        OutPath.ConnectionNodeBackward = &BackwardLastNode;
        
        // UE_LOG(LogTemp, Warning, TEXT("Connected subpaths!"));

        return true;
    }
    
    return false;
}

void UAudioRayTracingSubsystem::GeneratePath(const AActor* ActorToIgnore, FSoundPath& OutPath) const
{
    // Chance for a path to NOT terminate
    constexpr float RUSSIAN_ROULETTE_PROB = 0.8f;
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
                CurrentPos = H.ImpactPoint;
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
    float Energy = 1.0f;
    float Probability = 1.0f;
    
    for (int i = 0; i < Path.Nodes.Num() - 1; ++i)
    {
        FSoundPathNode& Node = Path.Nodes[i];
        FSoundPathNode& NextNode = Path.Nodes[i + 1];
        Distance += FVector::Dist(Node.Position, NextNode.Position);
        
        // diffuse = reflectivity / PI, where reflectivity is 0.0-1.0
        float BSDFFactor = 1.0f;
        if (Node.Material.IsValid() and Node.Material.Get()->Material)
        {
            BSDFFactor = Node.Material.Get()->Material->Absorption[2].Value / PI;
        }
        // TODO Multiply cosines of angles , divide by squared distance
        float GeometryTerm = (float) FMath::Cos(Node.Normal.Y) * FMath::Cos(Node.Normal.Y) / FMath::Square(Distance);

        Energy *= BSDFFactor;
        Energy *= GeometryTerm;
 
        Probability *= Node.Probability;
    }

    
    // Apply media term (equation 3)
    constexpr float AIR_ABSORPTION_FACTOR = 0.005;
    float MediaAbsorption = exp(-AIR_ABSORPTION_FACTOR * Distance);
    Energy *= MediaAbsorption;
    Energy /= Probability;
    
    // Update path values
    Path.TotalLength = Distance;
    Path.EnergyContribution = Energy;
    
    return {Distance / SoundSpeed, Energy};
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
    // Difference vector
    FVector Diff = End - Start;
    FVector Dir = Diff.GetSafeNormal();
    float Dist = Diff.Length();
    FVector CurrentPos = Start;

    const float DELTATIME = 1.0f / DEBUG_RAY_FPS;
    FVector Increment = Dir * Speed * DELTATIME;
    float DistIncrement = Increment.Length();
    UE_LOG(LogTemp, Warning, TEXT("Total Launches: %f"), (Dist / DistIncrement));

    float TimePassed = 0.0f;
    float DistTravelled = 0.f;   
    auto World = GetWorld();
    UE_LOG(LogTemp, Warning, TEXT("Start: %s"), *Start.ToString());
    // Simulate ticks with fixed delta time, drawing a ray segment on each tick with delay and length based on time passed
    while (DistTravelled < Dist)
    {
        // Find end of ray segment
        FVector NextPos = CurrentPos + Increment;
        DistTravelled += DistIncrement;
        // if ((NextPos - Start).Length() >= Dist)
        // {
        //     NextPos = End;
        // }
        
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
                    1.0f
                );
        	}),
        	Delay == 0 ? 0.0000001f : Delay,
        	false
        );

        // Increment current position
        CurrentPos = NextPos;

        // Increment time passed
        TimePassed += DELTATIME;
    }
    return TimePassed;
}

// float UAudioRayTracingSubsystem::DrawSegmentedLine(const FVector& Start, const FVector& End, float Speed, FColor Color,
//                                                    float DrawDelay, bool bPersistent) const
// {
//     // Difference vector
//     const FVector Diff = End - Start;
//     const FVector Dir = Diff.GetSafeNormal();
//     const float Dist = Diff.Length();
//     FVector CurrentPos = Start;
//
//     const float DELTATIME = 1.0f / DEBUG_RAY_FPS;
//     FVector Increment = Dir * Speed * DELTATIME;
//
//     float TimePassed = 0.0f;
//     auto World = GetWorld();
//
//     DrawDebugLine(
//         World,
//         Start,
//         End,
//         Color,
//         bPersistent,
//             1000.f,
//         0,
//         3.0f
//     );
//     
//     // // Simulate ticks with fixed delta time, drawing a ray segment on each tick with delay and length based on time passed
//     // while ((CurrentPos - Start).Length() < Dist)
//     // {
//     //     // Find end of ray segment
//     //     FVector NextPos = CurrentPos + Increment;
//     //     if (NextPos.Length() >= Dist)
//     //     {
//     //         NextPos = End;
//     //     }
//     //     
//     //     // Draw ray segment with delay equal to total time passed 
//     //     // FTimerHandle TimerHandle;
//     //     // float Delay = TimePassed;
// 	   //
//     //     // DrawDebugLine(
//     //     //     World,
//     //     //     CurrentPos,
//     //     //     NextPos,
//     //     //     Color,
//     //     //     bPersistent,
//     //     //         1000.f,
//     //     //     0,
//     //     //     1.0f
//     //     // );
//     //
//     //     // Increment current position
//     //     CurrentPos = NextPos;
//     //
//     //     // Increment time passed
//     //     TimePassed += DELTATIME;
//     // }
//     return TimePassed;
// }

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
    
    // 2. After doing above, Draw the connection line gradually while deleting
    FTimerHandle ConnectedPathsTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        ConnectedPathsTimerHandle,
        FTimerDelegate::CreateLambda([&, this]()
        {
            if (!IsValid(this)) return;
            // FlushPersistentDebugLines(GetWorld());
            // Redraw
            UE_LOG(LogTemp, Warning, TEXT("%d"), ConnectedPaths.Num());
            for (const FSoundPath& Path : ConnectedPaths)
            {
                FVector A = Path.ConnectionNodeForward->Position;
                FVector B = Path.ConnectionNodeBackward->Position;
                float Distance = FVector::Dist(A, B);
                DrawSegmentedLine(
                    Path.ConnectionNodeForward->Position,
                    Path.ConnectionNodeBackward->Position,
                    Distance / ShowConnectionDuration,
                    FColor::Blue,
                    0.0f,
                    true);
            }

            
        }),
        DrawPathsDuration,
        false);
    
    // 3. After doing above, erase existing debug lines and draw the connected paths BASED ON THEIR ENERGY
    FTimerHandle ContributionTimerHandle;
    GetWorld()->GetTimerManager().SetTimer(
        ContributionTimerHandle,
        FTimerDelegate::CreateLambda([=, this]()
        {
            if (!IsValid(this)) return;
            
            FlushPersistentDebugLines(GetWorld());
            
            for (const FSoundPath& Path : ConnectedPaths)
            {
                // Get path energy
                float Energy = Path.EnergyContribution;

                // Lerp its color between bright green and dark red based on this energy
                // Start and end colors
                FLinearColor DarkRed = FLinearColor(FColor(200, 0, 0));
                FLinearColor Green = FLinearColor(FColor::Green);

                // Lerp in HSV space
                FColor EnergyColor = FLinearColor::LerpUsingHSV(DarkRed, Green, Energy).ToFColor(true);

                // Draw using energy color
                VisualizePath(Path, 0.2f, EnergyColor, true);
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

