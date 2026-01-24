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

let private FindAndExecuteTarget(targets: IReadOnlyDictionary<string, Target>, name: string, cacheManager: CacheManager option) =
    match targets.TryGetValue name with
    | true, target ->
        AnsiConsole.Progress().StartAsync(fun ctx -> task {
            let buildContext =
                match cacheManager with
                | Some cm -> BuildContext(ctx, cm)
                | None -> BuildContext(ctx)
            return! ExecuteTarget(buildContext, target)
        })
    | false, _ -> Task.FromResult ExitCodes.TargetNotFound

let private PrintUsage() =
    printfn "Arguments:\n  [target] - execute the selected target. By default, the target is \"{DefaultTarget}\"."

let private RunThrowing(targets: IReadOnlyDictionary<string, Target>, args: string[], cacheManager: CacheManager option) =
    match args with
    | [||] -> FindAndExecuteTarget(targets, DefaultTarget, cacheManager)
    | [|targetName|] -> FindAndExecuteTarget(targets, targetName, cacheManager)
    | _ -> PrintUsage(); Task.FromResult ExitCodes.InvalidArguments

let private ReportError(e: Exception) =
    eprintfn $"{e}"

let Run(targets: IReadOnlyDictionary<string, Target>, args: string seq, cacheConfig: CacheConfig option): Task<int> = task {
    try
        let args = Array.ofSeq args
        let! cacheManager = task {
            match cacheConfig with
            | Some config ->
                let cm = CacheManager(config)
                do! cm.Cleanup()
                return Some cm
            | None -> return None
        }
        return! RunThrowing(targets, args, cacheManager)
    with
    | e ->
        ReportError e
        return ExitCodes.ExecutionError
}
