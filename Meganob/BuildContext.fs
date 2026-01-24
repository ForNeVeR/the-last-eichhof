namespace Meganob

open System.Collections.Concurrent

type BuildContext() =
    let resolvedDependencies = ConcurrentDictionary<IDependency, obj>()
    interface IBuildContext with
        member this.Reporter = RootReporter()
        member this.StoreDependency(dependency, context: IDependencyContext) = task {
            let! result = dependency.GetForStore context
            let success = resolvedDependencies.TryAdd(dependency, result)
            if not success then failwithf $"Dependency %A{dependency} has been calculated twice."
        }

