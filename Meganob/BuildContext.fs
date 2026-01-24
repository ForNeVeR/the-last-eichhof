namespace Meganob

open System.Collections.Concurrent
open Spectre.Console

type BuildContext(spectre: ProgressContext) =
    let resolvedDependencies = ConcurrentDictionary<IDependency, obj>()
    interface IBuildContext with
        member this.Reporter = SpectreReporter spectre
        member this.StoreDependency(dependency, context: IDependencyContext) = task {
            let! result = dependency.GetForStore context
            let success = resolvedDependencies.TryAdd(dependency, result)
            if not success then failwithf $"Dependency %A{dependency} has been calculated twice."
        }
