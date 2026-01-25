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

let DefaultTarget: string = "build"

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
