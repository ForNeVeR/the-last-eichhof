
module Meganob.BuildSystem

open System
open System.Collections.Generic
open System.Threading.Tasks
open Meganob
open Spectre.Console

module ExitCodes =
    let Success = 0
    let ExecutionError = 1
    let InvalidArguments = 2
    let TargetNotFound = 3

let private FormatBytes(bytes: int64): string =
    if bytes >= 1024L * 1024L * 1024L then
        $"%.2f{float bytes / float (1024L * 1024L * 1024L)} GB"
    elif bytes >= 1024L * 1024L then
        $"%.2f{float bytes / float (1024L * 1024L)} MB"
    elif bytes >= 1024L then
        $"%.2f{float bytes / float 1024L} KB"
    else
        $"%d{bytes} bytes"

let private DefaultTask: string = "build"

let ExecuteTask(rootTask: BuildTask, reporter: IReporter): Task<IArtifact> =
    let nullCache = { new ICacheManager with
        member _.TryLoadCached _ = Task.FromResult None
        member _.Store(_, _) = Task.CompletedTask
    }
    let context = { Reporter = reporter; Cache = nullCache }
    TaskExecutor.Execute(context, rootTask)

let private FindAndExecuteTask(
    tasks: IReadOnlyDictionary<string, BuildTask>,
    name: string,
    cacheManager: CacheManager
): Task<int> =
    match tasks.TryGetValue name with
    | true, rootTask ->
        AnsiConsole.Progress().StartAsync(fun ctx -> task {
            let buildContext = { Reporter = SpectreReporter(ctx); Cache = cacheManager }
            let! _ = TaskExecutor.Execute(buildContext, rootTask)
            return ExitCodes.Success
        })
    | false, _ -> Task.FromResult ExitCodes.TargetNotFound

let private PrintUsage() =
    printfn "Usage: [task] [options...]"
    printfn ""
    printfn "Arguments:"
    printfn $"  task      The build task to execute. Default: \"%s{DefaultTask}\"."
    printfn ""
    printfn "Options:"
    printfn "  --verbose   Print detailed cache cleanup information."

let private RunThrowing(tasks: IReadOnlyDictionary<string, BuildTask>, args: string[], cacheManager: CacheManager): Task<int> =
    match args with
    | [||] -> FindAndExecuteTask(tasks, DefaultTask, cacheManager)
    | [|targetName|] -> FindAndExecuteTask(tasks, targetName, cacheManager)
    | _ -> PrintUsage(); Task.FromResult ExitCodes.InvalidArguments

let private ReportError(e: Exception) =
    eprintfn $"{e}"

let Run(tasks: BuildTask seq, args: string seq, cacheConfig: CacheConfig option): Task<int> = task {
    try
        let tasks = Dictionary(tasks |> Seq.map(fun t -> KeyValuePair(t.Name, t)))
        let args = Array.ofSeq args
        let verbose = args |> Array.contains "--verbose"
        let args = args |> Array.filter (fun arg -> arg <> "--verbose")

        let cacheManager = CacheManager(cacheConfig |> Option.defaultValue CacheConfig.Default)
        let! cleanupResult = cacheManager.Cleanup(verbose)
        if cleanupResult.EntriesRemoved > 0 then
            printfn $"Cache cleanup: removed %d{cleanupResult.EntriesRemoved} entries, freed %s{FormatBytes cleanupResult.BytesFreed}"

        return! RunThrowing(tasks, args, cacheManager)
    with
    | e ->
        ReportError e
        return ExitCodes.ExecutionError
}
