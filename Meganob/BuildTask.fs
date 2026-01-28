namespace Meganob

open System
open System.Collections.Immutable
open System.Threading.Tasks

type ITaskOutput = interface end

/// A build task with dependencies (inputs) that must complete before execution.
/// The Inputs array uses ImmutableArray which captures references at creation time.
/// This makes dependency cycles structurally impossible — when a task is created,
/// all its dependencies must already exist as fully-formed objects, so they cannot reference it.
[<CustomComparison; CustomEquality>]
type BuildTask =
    {
        Id: Guid // TODO: Consider dropping this and removing any structural identity — it still allows to break the DAG in case of id reuse (malicious user).
        DisplayName: string
        Inputs: ImmutableArray<BuildTask>
        Execute: ITaskOutput seq -> Task<ITaskOutput>
    }

    override this.Equals(obj) =
        match obj with
        | :? BuildTask as other -> other.Id = this.Id
        | _ -> false

    override this.GetHashCode() =
        this.Id.GetHashCode()

    interface IComparable with
        member this.CompareTo(obj) =
            match obj with
            | :? BuildTask as other -> this.Id.CompareTo other.Id
            | _ -> failwithf $"Cannot compare %A{this} with %A{obj}."
