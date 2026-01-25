namespace Meganob

open System.Collections.Immutable
open System.Threading.Tasks

[<Interface>]
type ITargetContext =
    abstract member Reporter: IReporter
    abstract member GetDependency: IDependency -> obj

type ITargetResult = interface end

type Target =
    {
        Name: string
        Dependencies: ImmutableArray<IDependency>
        Execute: ITargetContext -> Task<ITargetResult>
    }

    interface IDependency with
        member this.CacheKey = failwith "todo"
        member this.DisplayName = this.Name
        member this.GetForStore(var0) = failwith "todo"
        member this.LoadFromCache(cacheDir) = failwith "todo"
        member this.StoreToCache(cacheDir, value) = failwith "todo"

    static member Define(
        name: string,
        dependencies: IDependency seq,
        execute: ITargetContext -> Task<ITargetResult>
    ): Target =
        {
            Name = name
            Dependencies = dependencies.ToImmutableArray()
            Execute = execute
        }
