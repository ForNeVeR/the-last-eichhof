module internal Meganob.TaskExecutor

open System
open System.Collections.Concurrent
open System.Collections.Generic
open System.Threading.Channels
open System.Threading.Tasks

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
let Execute(context: BuildContext, rootTask: BuildTask): Task<IArtifact> =
    let allTasks = CollectAllTasks rootTask
    let dependents = BuildDependentsMap allTasks

    // Build initial indegree map (number of unfinished dependencies)
    let inDegree = Dictionary<Guid, int>()
    for kvp in allTasks do
        inDegree[kvp.Key] <- kvp.Value.Inputs.Length

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
    for kvp in allTasks do
        if inDegree[kvp.Key] = 0 then
            enqueueResult kvp.Value

    let processTask (buildTask: BuildTask): Task =
        task {
            try
                let inputValues = buildTask.Inputs |> Seq.map (fun input -> outputs[input]) |> Seq.toList

                // Skip cache for tasks with no inputs (non-cacheable)
                let! cachedResult =
                    if inputValues.IsEmpty then Task.FromResult None
                    else context.Cache.TryLoadCached(inputValues) // TODO: At this point, we should know the task output type. Make loading from cache a task's responsibility? Or store a type designator in cache.json?

                match cachedResult with
                | Some cached ->
                    outputs[buildTask] <- cached
                | None ->
                    let! output = buildTask.Execute(context, inputValues)
                    outputs[buildTask] <- output
                    if not inputValues.IsEmpty then
                        do! context.Cache.Store(inputValues, output)

                lock inDegreeLock (fun () ->
                    completedCount <- completedCount + 1

                    // Decrement indegree of dependents and enqueue newly ready tasks
                    for dependentId in dependents[buildTask.Id] do
                        inDegree[dependentId] <- inDegree[dependentId] - 1
                        if inDegree[dependentId] = 0 then
                            enqueueResult allTasks[dependentId]

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
