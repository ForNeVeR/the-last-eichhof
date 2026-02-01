module internal Meganob.TaskExecutor

open System
open System.Collections.Concurrent
open System.Collections.Generic
open System.Threading.Channels
open System.Threading.Tasks

/// Recursively collects all tasks in the dependency graph.
let private CollectAllTasks(root: BuildTask): HashSet<BuildTask> =
    let visited = HashSet<BuildTask>()
    let rec walk (task: BuildTask) =
        if visited.Add task then
            for input in task.Inputs do
                walk input
    walk root
    visited

/// Builds a reverse dependency map: task ID -> list of task IDs that depend on it.
let private BuildDependentsMap(tasks: HashSet<BuildTask>): Dictionary<BuildTask, ResizeArray<BuildTask>> =
    let dependents = Dictionary<BuildTask, ResizeArray<BuildTask>>()
    for task in tasks do
        if not (dependents.ContainsKey task) then
            dependents[task] <- ResizeArray()
    for task in tasks do
        for input in task.Inputs do
            dependents[input].Add task
    dependents

/// Executes a BuildTask and all its dependencies with maximum parallelism.
/// This implements Kahn's algorithm.
let Execute(context: BuildContext, rootTask: BuildTask): Task<IArtifact> =
    let allTasks = CollectAllTasks rootTask
    let dependents = BuildDependentsMap allTasks

    // Build initial indegree map (number of unfinished dependencies)
    let inDegree = Dictionary<BuildTask, int>()
    for task in allTasks do
        inDegree[task] <- task.Inputs.Length

    // State for parallel execution
    let outputs = ConcurrentDictionary<BuildTask, IArtifact>()
    let readyChannel = Channel.CreateUnbounded<BuildTask>()
    let mutable completedCount = 0
    let inDegreeLock = obj()
    let tcs = TaskCompletionSource<IArtifact>(TaskCreationOptions.RunContinuationsAsynchronously)

    let enqueueResult task =
        let success = readyChannel.Writer.TryWrite task
        if not success then failwithf "Unexpected rejection from channel."

    // Initialize ready queue with tasks that have no dependencies
    for task in allTasks do
        if inDegree[task] = 0 then
            enqueueResult task

    let processTask (buildTask: BuildTask): Task =
        task {
            try
                let inputValues = buildTask.Inputs |> Seq.map (fun input -> outputs[input]) |> Seq.toList

                // Skip cache for tasks without cache data
                let! cachedResult =
                    match buildTask.CacheData with
                    | None -> Task.FromResult None
                    | Some cacheData ->
                        if inputValues.IsEmpty then Task.FromResult None
                        else context.Cache.TryLoadCached(inputValues, cacheData)

                match cachedResult with
                | Some cached ->
                    outputs[buildTask] <- cached
                | None ->
                    let! output = buildTask.Execute(context, inputValues)
                    outputs[buildTask] <- output
                    match buildTask.CacheData with
                    | Some cacheData when not inputValues.IsEmpty ->
                        do! context.Cache.Store(inputValues, cacheData, output)
                    | _ -> ()

                lock inDegreeLock (fun () ->
                    completedCount <- completedCount + 1

                    // Decrement indegree of dependents and enqueue newly ready tasks
                    for dependent in dependents[buildTask] do
                        inDegree[dependent] <- inDegree[dependent] - 1
                        if inDegree[dependent] = 0 then
                            enqueueResult dependent

                    // Check if all tasks are done
                    if completedCount = allTasks.Count then
                        tcs.TrySetResult outputs[rootTask] |> ignore
                        readyChannel.Writer.Complete()

                )
            with
            | ex ->
                tcs.TrySetException ex |> ignore
                readyChannel.Writer.Complete()
        }

    context.Reporter.WithProgress(
        $"Executing {rootTask.Name}",
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
