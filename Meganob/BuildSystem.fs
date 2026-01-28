module Meganob.BuildSystem

open System
open System.Collections.Concurrent
open System.Collections.Generic
open System.Threading.Channels
open System.Threading.Tasks
open Meganob
open Spectre.Console

module ExitCodes =
    let Success = 0
    let ExecutionError = 1
    let InvalidArguments = 2
    let TargetNotFound = 3

let DefaultTarget: string = "build"

/// Recursively collects all tasks in the dependency graph.
let private CollectAllTasks(root: BuildTask): Dictionary<Guid, BuildTask> =
    let visited = Dictionary<Guid, BuildTask>()
    let rec walk (task: BuildTask) =
        if not (visited.ContainsKey task.Id) then
            visited[task.Id] <- task
            for input in task.Inputs do
                walk input
    walk root
    visited

/// Builds a reverse dependency map: task ID -> list of task IDs that depend on it.
let private BuildDependentsMap(tasks: Dictionary<Guid, BuildTask>): Dictionary<Guid, ResizeArray<Guid>> =
    let dependents = Dictionary<Guid, ResizeArray<Guid>>()
    for kvp in tasks do
        if not (dependents.ContainsKey kvp.Key) then
            dependents[kvp.Key] <- ResizeArray()
    for kvp in tasks do
        for input in kvp.Value.Inputs do
            dependents[input.Id].Add kvp.Key
    dependents

/// Executes a BuildTask and all its dependencies with maximum parallelism.
/// This implements Kahn's algorithm.
let internal ExecuteTask(rootTask: BuildTask, reporter: IReporter): Task<ITaskOutput> =
    let allTasks = CollectAllTasks rootTask
    let dependents = BuildDependentsMap allTasks

    // Build initial indegree map (number of unfinished dependencies)
    let inDegree = Dictionary<Guid, int>()
    for kvp in allTasks do
        inDegree[kvp.Key] <- kvp.Value.Inputs.Length

    // State for parallel execution
    let outputs = ConcurrentDictionary<Guid, ITaskOutput>()
    let readyChannel = Channel.CreateUnbounded<BuildTask>()
    let mutable completedCount = 0
    let inDegreeLock = obj()
    let tcs = TaskCompletionSource<ITaskOutput>(TaskCreationOptions.RunContinuationsAsynchronously)

    let enqueueResult task =
        let success = readyChannel.Writer.TryWrite task
        if not success then failwithf "Unexpected rejection from channel."

    // Initialize ready queue with tasks that have no dependencies
    for kvp in allTasks do
        if inDegree[kvp.Key] = 0 then
            enqueueResult kvp.Value

    let processTask (buildTask: BuildTask): Task =
        task {
            try
                let inputValues = buildTask.Inputs |> Seq.map (fun input -> outputs[input.Id])
                let! output = buildTask.Execute inputValues
                outputs[buildTask.Id] <- output

                lock inDegreeLock (fun () ->
                    completedCount <- completedCount + 1

                    // Decrement indegree of dependents and enqueue newly ready tasks
                    for dependentId in dependents[buildTask.Id] do
                        inDegree[dependentId] <- inDegree[dependentId] - 1
                        if inDegree[dependentId] = 0 then
                            enqueueResult allTasks[dependentId]

                    // Check if all tasks are done
                    if completedCount = allTasks.Count then
                        tcs.TrySetResult outputs[rootTask.Id] |> ignore
                        readyChannel.Writer.Complete()

                )
            with
            | ex ->
                tcs.TrySetException ex |> ignore
                readyChannel.Writer.Complete()
        }

    reporter.WithProgress(
        $"Executing {rootTask.DisplayName}",
        int64 allTasks.Count,
        fun progress -> task {
            // Process tasks as they become ready
            let rec processLoop () = task {
                while not tcs.Task.IsCompleted do
                    let reader = readyChannel.Reader
                    let! status = reader.WaitToReadAsync()
                    if status then
                        let! readyTask = reader.ReadAsync()

                        // Spawn in parallel:
                        Task.Run(Func<Task>(fun () -> task {
                            do! processTask readyTask
                            lock inDegreeLock (fun () -> progress.Report (int64 completedCount))
                        })) |> ignore
                    else
                        return ()
            }

            do! processLoop ()
            return! tcs.Task
        }
    )

let private ExecuteTarget(context: IBuildContext, target: Target): Task<int> =
    let dependencyContext = DependencyContext.Of context
    context.Reporter.WithProgress(
        $"Dependencies of {target.Name}",
        int64 target.Dependencies.Length,
        fun progress -> task {
            let mutable counter = 0L
            for dependency in target.Dependencies do
                do! context.StoreDependency(dependency, dependencyContext)
                counter <- counter + 1L
                progress.Report counter
            return ExitCodes.Success
        }
    )

let private FindAndExecuteTarget(
    targets: IReadOnlyDictionary<string, Target>,
    name: string,
    cacheManager: CacheManager
) =
    match targets.TryGetValue name with
    | true, target ->
        AnsiConsole.Progress().StartAsync(fun ctx -> task {
            let buildContext = BuildContext(ctx, cacheManager)
            return! ExecuteTarget(buildContext, target)
        })
    | false, _ -> Task.FromResult ExitCodes.TargetNotFound

let private PrintUsage() =
    printfn "Usage: [task] [options...]"
    printfn ""
    printfn "Arguments:"
    printfn $"  task      The build task to execute. Default: \"%s{DefaultTarget}\"."
    printfn ""
    printfn "Options:"
    printfn "  --verbose   Print detailed cache cleanup information."

let private FormatBytes(bytes: int64): string =
    if bytes >= 1024L * 1024L * 1024L then
        $"%.2f{float bytes / float (1024L * 1024L * 1024L)} GB"
    elif bytes >= 1024L * 1024L then
        $"%.2f{float bytes / float (1024L * 1024L)} MB"
    elif bytes >= 1024L then
        $"%.2f{float bytes / float 1024L} KB"
    else
        $"%d{bytes} bytes"

let private RunThrowing(targets: IReadOnlyDictionary<string, Target>, args: string[], cacheManager: CacheManager) =
    match args with
    | [||] -> FindAndExecuteTarget(targets, DefaultTarget, cacheManager)
    | [|targetName|] -> FindAndExecuteTarget(targets, targetName, cacheManager)
    | _ -> PrintUsage(); Task.FromResult ExitCodes.InvalidArguments

let private ReportError(e: Exception) =
    eprintfn $"{e}"

let Run(targets: IReadOnlyDictionary<string, Target>, args: string seq, cacheConfig: CacheConfig option): Task<int> = task {
    try
        let args = Array.ofSeq args
        let verbose = args |> Array.contains "--verbose"
        let args = args |> Array.filter (fun arg -> arg <> "--verbose")

        let cacheManager = CacheManager(cacheConfig |> Option.defaultValue CacheConfig.Default)
        let! cleanupResult = cacheManager.Cleanup(verbose)
        if cleanupResult.EntriesRemoved > 0 then
            printfn $"Cache cleanup: removed %d{cleanupResult.EntriesRemoved} entries, freed %s{FormatBytes cleanupResult.BytesFreed}"

        return! RunThrowing(targets, args, cacheManager)
    with
    | e ->
        ReportError e
        return ExitCodes.ExecutionError
}
