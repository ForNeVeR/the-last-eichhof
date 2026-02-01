namespace Meganob

open System.Threading.Tasks
open Spectre.Console

[<Interface>]
type IProgressReporter =
    abstract member Report: int64 -> unit

[<Interface>]
type IReporter =
    abstract member Status: newStatus: string -> unit
    abstract member WithProgress: header: string * total: int64 * action: (IProgressReporter -> Task<'a>) -> Task<'a>
    abstract member Log: string -> unit

module ProgressReporter =
    let Null: IProgressReporter = {
        new IProgressReporter with
            member _.Report _ = ()
    }

type internal SpectreReporter(spectre: ProgressContext) =
    interface IReporter with
        member _.Log(message) =
            AnsiConsole.MarkupLine(message)
        member _.Status(newStatus) =
            AnsiConsole.MarkupLine($"[grey]Status:[/] {newStatus}")
        member _.WithProgress(header, total, action) =
            task {
                let task = spectre.AddTask(header, true, float total)
                let reporter = { new IProgressReporter with member _.Report value = task.Value <- float value }
                return! action reporter
            }
