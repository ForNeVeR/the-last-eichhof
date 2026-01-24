namespace Meganob

open System.Threading.Tasks

[<Interface>]
type IProgressReporter =
    abstract member Report: int64 -> unit

[<Interface>]
type IReporter =
    abstract member Status: newStatus: string -> unit
    abstract member WithProgress: header: string * total: int64 * action: (IProgressReporter -> Task<'a>) -> Task<'a>
    abstract member Log: string -> unit

module ProgressReporter =
    let Null: IProgressReporter =
        { new IProgressReporter with
            member _.Report _ = () }

type internal RootReporter() =
    interface IReporter with
        member this.Log(var0) = failwith "todo"
        member this.Status(newStatus) = failwith "todo"
        member this.WithProgress(header, total, action) = failwith "todo"
