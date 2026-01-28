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

let private FindAndExecuteTask(
    tasks: IReadOnlyDictionary<string, BuildTask>,
    name: string,
    cacheManager: CacheManager
) =
    match tasks.TryGetValue name with
    | true, rootTask ->
        AnsiConsole.Progress().StartAsync(fun ctx -> task {
            let buildContext = { Reporter = SpectreReporter(ctx); Cache = cacheManager }
            return! TaskExecutor.Execute(buildContext, rootTask)
        })
    | false, _ -> Task.FromResult ExitCodes.TargetNotFound

let private PrintUsage() =
    printfn "Usage: [task] [options...]"
    printfn ""
    printfn "Arguments:"
    printfn $"  task      The build task to execute. Default: \"%s{BuildSystem.DefaultTask}\"."
    printfn ""
    printfn "Options:"
    printfn "  --verbose   Print detailed cache cleanup information."

let private RunThrowing(tasks: IReadOnlyDictionary<string, Task>, args: string[], cacheManager: CacheManager) =
    match args with
    | [||] -> FindAndExecuteTask(tasks, BuildSystem.DefaultTask, cacheManager)
    | [|targetName|] -> BuildSystem.FindAndExecuteTarget(tasks, targetName, cacheManager)
    | _ -> BuildSystem.PrintUsage(); Task.FromResult ExitCodes.InvalidArguments

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

        return! BuildSystem.RunThrowing(tasks, args, cacheManager)
    with
    | e ->
        BuildSystem.ReportError e
        return ExitCodes.ExecutionError
}
