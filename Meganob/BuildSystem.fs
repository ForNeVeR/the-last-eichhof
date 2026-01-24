module Meganob.BuildSystem

open System
open System.Collections.Generic
open System.Threading.Tasks
open Meganob

module ExitCodes =
    let Success = 0
    let ExecutionError = 1
    let InvalidArguments = 2
    let TargetNotFound = 3

let DefaultTarget: string = "build"

let private ExecuteTarget(context: IBuildContext, target: Target) =
    let dependencyContext = DependencyContext.Of context
    context.Reporter.WithProgress($"Dependencies of {target.Name}", target.Dependencies.Length, fun progress -> task {
        let mutable counter = 0L
        for dependency in target.Dependencies do
            do! context.StoreDependency dependency
            counter <- counter + 1L
            progress.Report counter
    })

let private FindAndExecuteTarget(targets: IReadOnlyDictionary<string, Target>, name: string) =
    let buildContext = BuildContext()
    match targets.GetValueOrDefault name with
    | Null -> Task.FromResult ExitCodes.TargetNotFound
    | NonNull target -> ExecuteTarget buildContext target

let private PrintUsage() =
    printfn "Arguments:\n  [target] - execute the selected target. By default, the target is \"{DefaultTarget}\"."

let private RunThrowing(targets: IReadOnlyDictionary<string, Target>, args: string[]) =
    match args with
    | [||] -> FindAndExecuteTarget(targets, DefaultTarget)
    | [|targetName|] -> FindAndExecuteTarget(targets, targetName)
    | _ -> PrintUsage(); Task.FromResult ExitCodes.InvalidArguments

let private ReportError(e: Exception) =
    eprintfn $"{e}"

let Run(targets: IReadOnlyDictionary<string, Target>, args: string seq): Task<int> = task {
    try
        let args = Array.ofSeq args
        return! RunThrowing(targets, args)
    with
    | e ->
        ReportError e
        return ExitCodes.ExecutionError
}
